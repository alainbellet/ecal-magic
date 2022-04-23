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
using UnityEngine.InputSystem.LowLevel;

[ExecuteAlways]
public class Esp32DeviceConnection : MonoBehaviour
{
    [Header("Client")]
    public string clientAddress = "192.168.45.30";
    public int clientPort = 9999;

    [Header("Server")]
    public int serverMinPort = 8880;
    public int serverMaxPort = 8899;
    public string serverAddress { get; private set; }
    public int serverPort { get; private set; }
    
    OscServer _server; // IN
    OscClient _client; // OUT

    Queue<Esp32DeviceState> dataQueue = new Queue<Esp32DeviceState>();

    public float timeSinceLastEvent { get; private set; } = -1;
    public float timeSinceLastIpSend { get; private set; } = 999;

    public bool initialized { get; private set; }

    public Esp32DeviceState currentState { get; private set; }

    public Esp32Device inputDevice { get; private set; }

    private bool firstEncoderValueReceived;
    private float zeroEncoderValue;

    void OnEnable()
    {
        // IN
        serverPort = serverMinPort;
        while (serverPort < serverMaxPort)
        {
            try
            {
                _server = new OscServer(serverPort); // Port number
                break;
            }
            catch
            {
            }
            serverPort++;
        }

        if (_server == null)
            throw new UnityException($"No free server port found in range {serverMinPort}-{serverMaxPort}");

        _server.MessageDispatcher.AddCallback("/unity/state/", OnDataReceiveState);
        _server.MessageDispatcher.AddCallback("/unity/ipupdated/", OnDataReceiveIp);

        // OUT
        _client = new OscClient(clientAddress, clientPort);

        // start update ip loop
        serverAddress = GetLocalAddress();

        inputDevice = InputSystem.AddDevice(new InputDeviceDescription
        {
            interfaceName = "ESP32",
            product = "Input Thing",
            version = "1",
            deviceClass = "box",
            manufacturer = "ecal",
            capabilities = "encoder,button,motor",
            serial = $"{clientAddress}:{clientPort}@{serverPort}",
        }) as Esp32Device;
        InputSystem.EnableDevice(inputDevice);

        AddCommandListener();

        initialized = true;
    }


    void OnDisable()
    {
        serverPort = -1;
        
        if (inputDevice != null)
        {
            InputSystem.RemoveDevice(inputDevice);
            inputDevice = null;
        }

        if (_server != null)
        {
            _server.Dispose();
            _server = null;
        }

        if (_client != null)
        {
            _client.Dispose();
            _client = null;
        }

        initialized = false;

        firstEncoderValueReceived = false;
        RemoveCommandListener();
    }


    void Update()
    {
        lock (dataQueue)
        {
            while (dataQueue.Count > 0)
            {
                var state = dataQueue.Dequeue();

                InputSystem.QueueStateEvent(inputDevice, state);

                currentState = state;
            }
        }

        if (timeSinceLastEvent >= 0)
            timeSinceLastEvent += Time.deltaTime;

        if (!Application.isEditor)
        {
            timeSinceLastIpSend += Time.deltaTime;
            if (timeSinceLastIpSend > 5)
            {
                SendIpNow();
                timeSinceLastIpSend = 0;
            }
        }
    }

    void OnDataReceiveState(string address, OscDataHandle data)
    {
        
        var state = new Esp32DeviceState();
        state.button = data.GetElementAsInt(0) == 0; // 0 = pressed, 1 = released
        state.encoder = data.GetElementAsInt(1) / 4095f;
        
        if (!firstEncoderValueReceived)
        {
            zeroEncoderValue = state.encoder;
            firstEncoderValueReceived = true;
        }

        state.encoder -= zeroEncoderValue;

        lock (dataQueue)
        {
            dataQueue.Enqueue(state);
        }

        timeSinceLastEvent = 0;
    }

    unsafe void AddCommandListener()
    {
        InputSystem.onDeviceCommand += OnDeviceCommand;
    }

    unsafe void RemoveCommandListener()
    {
        InputSystem.onDeviceCommand -= OnDeviceCommand;
    }

    private unsafe long? OnDeviceCommand(InputDevice commandDevice, InputDeviceCommand* command)
    {
        if (commandDevice == inputDevice)
        {
            if (command->type == Esp32HapticEventCommand.Type)
            {
                var cmd = (Esp32HapticEventCommand*)command;
                _client.Send("/arduino/motor/cmd", cmd->eventId);
            }

            if (command->type == Esp32HapticRealtimeCommand.Type)
            {
                var cmd = (Esp32HapticRealtimeCommand*)command;
                _client.Send("/arduino/motor/rt", Mathf.RoundToInt(cmd->speed * 100));
            }
        }


        return 0;
    }


    void OnDataReceiveIp(string address, OscDataHandle data)
    {
        timeSinceLastEvent = 0;
    }

    public void SendIpNow()
    {
        _client.Send("/arduino/updateip", $"{serverAddress}:{serverPort}");
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