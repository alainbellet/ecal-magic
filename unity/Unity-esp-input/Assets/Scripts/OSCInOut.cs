using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using UnityEngine;
using OscJack;
using Unity.Collections;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Layouts;

public class OSCInOut : MonoBehaviour
{
	public string clientAddress = "192.168.45.30";
	public int clientPort = 9999;

	string serverAddress;
	public int serverPort = 8888;

	OscServer _server; // IN
	OscClient _client; // OUT

	Esp32Device inputDevice;

	Queue<Esp32DeviceState> dataQueue = new Queue<Esp32DeviceState>();

	void OnEnable()
	{
		// IN
		_server = new OscServer(serverPort); // Port number
		_server.MessageDispatcher.AddCallback("/unity/state/", OnDataReceive);

		// OUT
		_client = new OscClient(clientAddress, clientPort);

		// start update ip loop
		serverAddress = GetLocalAddress();
		Debug.Log("Local IP: "+serverAddress);
		StartCoroutine(UpdateIPLoop());

		// init input device
		inputDevice = InputSystem.AddDevice(new InputDeviceDescription
		{
			interfaceName = "Poti",
			product = "Custom Product"
		}) as Esp32Device;
	}

	void OnDisable()
	{
		InputSystem.RemoveDevice(inputDevice);

		_server.Dispose();
		_server = null;
	}

	void Update()
	{
		lock (dataQueue)
		{
			while (dataQueue.Count > 0)
			{
				var state = dataQueue.Dequeue();

//				Debug.Log("state " + state);
				InputSystem.QueueStateEvent(inputDevice, state);
			}
		}
	}

	void OnDataReceive(string address, OscDataHandle data)
	{
		var state = new Esp32DeviceState();
		state.button = data.GetElementAsInt(0) == 1;
		state.poti = data.GetElementAsInt(1) / 4095f;

		lock (dataQueue)
		{
			dataQueue.Enqueue(state);
		}
	}


	public void Send(bool value)
	{
		_client.Send("/arduino/led", value ? 1 : 0); // Second element
	}


	IEnumerator UpdateIPLoop()
	{
		while (true)
		{
			// send regularly, in case the packet gets lost
			_client.Send("/arduino/updateip",serverAddress); 
			yield return new WaitForSeconds(5);
		}
	}
	static string GetLocalAddress()
	{
		IPHostEntry ipEntry = Dns.GetHostEntry(Dns.GetHostName());
		IPAddress[] addr = ipEntry.AddressList;

		var address = string.Empty;
		for (int i = 0; i < addr.Length; i++)
		{
			if (addr[i].AddressFamily == AddressFamily.InterNetwork)
			{
				//Debug.Log("IP Address {0}: {1} " + " "+  i + " "  +addr[i].ToString());
				address = addr[i].ToString();
				break;
			}
		}

		return address;
	}
}