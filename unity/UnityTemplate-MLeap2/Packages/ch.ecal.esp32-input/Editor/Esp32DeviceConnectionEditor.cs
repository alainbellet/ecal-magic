using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

[CustomEditor(typeof(Esp32DeviceConnection))]
public class Esp32DeviceConnectionEditor : Editor
{
    private int hapticEvent;
    private float motorSpeed;
    
    public override bool RequiresConstantRepaint()
    {
        return true;
    }
    
    public override void OnInspectorGUI()
    {
        base.OnInspectorGUI();
        var oscEsp32Manager = (target as Esp32DeviceConnection);
        
        EditorGUILayout.LabelField("Local IP",oscEsp32Manager.serverAddress);


        var state = "";
        var isActive = oscEsp32Manager.timeSinceLastEvent >= 0;
        var color = Color.white;
        if (isActive)
        {
            color = new Color(.0f,1,.0f);
            state = "connected";
        }
        else
        {
            color = new Color(1f, .0f, .0f);
            state = "not connected";
        }

        GUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel("State");
        var prevColor = GUI.color;
        GUI.color = color;
        EditorGUILayout.LabelField(state,EditorStyles.boldLabel);
        GUILayout.EndHorizontal();
        GUI.color = prevColor;
        
        GUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel("Time since last event");
        EditorGUILayout.LabelField(oscEsp32Manager.timeSinceLastEvent >= 0 ? $"{oscEsp32Manager.timeSinceLastEvent:0.0}s" : "no events received");
        GUILayout.EndHorizontal();
            

        GUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel(" ");
        if (GUILayout.Button("Connect"))
        {
            oscEsp32Manager.gameObject.SetActive(false);
            oscEsp32Manager.gameObject.SetActive(true);
            oscEsp32Manager.SendIpNow();
        }

        GUILayout.EndHorizontal();
        
        EditorGUILayout.Space();
        EditorGUILayout.LabelField("","Debug ",EditorStyles.largeLabel);
        EditorGUILayout.Space();
        EditorGUILayout.LabelField("","Input",EditorStyles.boldLabel);
        EditorGUILayout.LabelField("Button:",oscEsp32Manager.currentState.button ? "down" : "up",EditorStyles.boldLabel);
        EditorGUILayout.LabelField("Encoder Value:",oscEsp32Manager.currentState.encoder.ToString(),EditorStyles.boldLabel);
        EditorGUILayout.Space();

        EditorGUILayout.LabelField("","Output",EditorStyles.boldLabel);
        hapticEvent = EditorGUILayout.IntSlider("Haptic Event", hapticEvent, 0, 123);
           
        GUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel(" ");
        if (GUILayout.Button("Send Haptic Event"))
        {
            oscEsp32Manager.inputDevice.SendHapticEvent(hapticEvent);
        }
 
        GUILayout.EndHorizontal();

       var newMotorSpeed = EditorGUILayout.Slider("Motor Speed", motorSpeed, 0f, 1f);
       
       if(Math.Abs(motorSpeed - newMotorSpeed) > 0.00001f)
       {
           oscEsp32Manager.inputDevice.SendMotorSpeed(motorSpeed);
           motorSpeed = newMotorSpeed;
       }
           
        GUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel(" ");
        if (GUILayout.Button("Stop Motor"))
        {
            oscEsp32Manager.inputDevice.SendMotorSpeed(0);
            motorSpeed = 0;
        }

        GUILayout.EndHorizontal();
        
        UnityEditor.EditorApplication.QueuePlayerLoopUpdate();
    }
}