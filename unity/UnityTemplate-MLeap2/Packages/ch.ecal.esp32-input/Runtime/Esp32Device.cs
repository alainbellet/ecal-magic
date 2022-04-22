using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Controls;
using UnityEngine.InputSystem.Layouts;
#if UNITY_EDITOR
using UnityEditor;
#endif

// The input system stores a chunk of memory for each device. What that
// memory looks like we can determine ourselves. The easiest way is to just describe
// it as a struct.
//
// Each chunk of memory is tagged with a "format" identifier in the form
// of a "FourCC" (a 32-bit code comprised of four characters). Using
// IInputStateTypeInfo we allow the system to get to the FourCC specific
// to our struct. 

#if UNITY_EDITOR
[InitializeOnLoad] // Call static class constructor in editor.
#endif
[InputControlLayout(stateType = typeof(Esp32DeviceState))]
public class Esp32Device : InputDevice
{
    
#if UNITY_EDITOR
    static Esp32Device()
    {
        Initialize();
    }
#endif

    [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.BeforeSceneLoad)]
    private static void Initialize()
    {
        InputSystem.RegisterLayout<Esp32Device>(
            matches: new InputDeviceMatcher()
                .WithInterface("ESP32"));
    }

    public AxisControl encoder { get; private set; }
    public ButtonControl button { get; private set; }

    protected override void FinishSetup()
    {
        base.FinishSetup();
            
        
        encoder = GetChildControl<AxisControl>("encoder");
        button = GetChildControl<ButtonControl>("button");
    }


    public void SendMotorSpeed(float speed)
    {
        var cmd = Esp32HapticRealtimeCommand.Create(speed);
        device.ExecuteCommand(ref cmd);
    }


    public void SendHapticEvent(int eventId)
    {
        var cmd = Esp32HapticEventCommand.Create(eventId);
        device.ExecuteCommand(ref cmd);
    }
}