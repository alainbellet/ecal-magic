using UnityEngine;
using UnityEngine.XR.MagicLeap;

public class TrackedImageObject : MonoBehaviour
{
    public ImageTrackingSystem imageTrackingSystem;
    public string trackingId;
    public MLImageTracker.Target.Result trackingState;
}