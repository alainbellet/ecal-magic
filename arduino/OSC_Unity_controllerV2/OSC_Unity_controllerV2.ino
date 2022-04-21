/*---------------------------------------------------------------------------------------------

  Controller for Unity
  Communication via OSC

  V1 AB/ECAL 2022


  --------------------------------------------------------------------------------------------- */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <ESPmDNS.h>
#include "your_secrets.h"
#include <Wire.h>
#include "Adafruit_DRV2605.h"
#include <ESP32Encoder.h>


char ssid[] = WIFI_SSID;                    // edit WIFI_SSID + WIFI_PASS constants in the your_secret.h tab (if not present create it)
char pass[] = WIFI_PASS;

WiFiUDP Udp;                                // A UDP instance to let us send and receive packets over UDP
IPAddress outIp(192, 168, 10, 56);          // remote IP of your computer
const unsigned int outPort = 8888;          // remote port to receive OSC
const unsigned int localPort = 9999;        // local port to listen for OSC packets (actually not used for sending)

OSCErrorCode error;


/* ------- Define pins ann vars for button + encoder */
// Button
const int buttonPin = 27;
int buttonState = 0;
int buttonLastState = -1;
// Encoder
ESP32Encoder encoder;
const int encoder_pin_1 = 13;
const int encoder_pin_2 = 15;
int32_t encoderPrevCount = -9999;
int32_t encoderCount;

/* ------- Adafruit_DRV2605 */
Adafruit_DRV2605 drv;
int8_t motorCurrentMode;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  // DRV2605
  Serial.println("Starting DRV2605");
  drv.begin();
  drv.selectLibrary(1);
  drv.useERM(); // ERM or LRA
  playHapticRT(0.0); // Stop the motor in case it hang running
  // BUTTON
  pinMode(buttonPin, INPUT_PULLUP);
  // ENCODER
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(encoder_pin_1, encoder_pin_2);
  encoder.setFilter(1023);
  encoder.setCount(0); // reset the counter
  // Start Wifi and UDP
  startWifiAndUdp();
}


void loop() {
  /* --------- SEND OSC MSGS */
  // ENCODER
  // read the state of the Encoder
  encoderCount = encoder.getCount();

  if (encoderCount != encoderPrevCount) {
    sendValues();
    //Serial.println("Encoder count = " + String((int32_t)encoderCount));
    encoderPrevCount = encoderCount;
  }

  // BUTTON
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  if (buttonState != buttonLastState) {
    sendValues();
    buttonLastState = buttonState;
  }

  /* --------- CHECK INCOMMING OSC MSGS */
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {
      msg.dispatch("/arduino/motor", motor);
      msg.dispatch("/arduino/updateip", updateIp);
    } else {
      error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}

/* --------- FUNCTIONS */

void startWifiAndUdp() {
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Start UDP
  Serial.println("Starting UDP");
  if (!Udp.begin(localPort)) {
    Serial.println("Error starting UDP");
    return;
  }
  Serial.print("Local port: ");
  Serial.println(localPort);
  Serial.print("Remote IP: ");
  Serial.println(outIp);
  // Start MDNS
  String unique_addr = WiFi.macAddress();
  unique_addr.replace(":", "");
  unique_addr = "encoder" + unique_addr.substring(0, 3);
  char mdns[30];
  unique_addr.toCharArray(mdns, 30);
  Serial.println(mdns);

  WiFi.setHostname(mdns);
  MDNS.begin(mdns);
  MDNS.addService("_osc", "_udp", localPort);
  MDNS.addServiceTxt("osc", "udp", "board", "ESP32Board");

  /*if (!MDNS.begin(mdns)) {
    Serial.println("Error starting mDNS");
    return;
    }*/
}

void motor(OSCMessage &msg) {
  Serial.println("/arduino/motor: ");
}

void sendValues() {
  OSCMessage msg("/unity/state/");
  //msg.add(buttonState);
  msg.add(encoderCount);
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
  delay (10);

}

void updateIp(OSCMessage &msg) {
  char newIp[15];
  int str_length = msg.getString(0, newIp, 15);
  String ipString = String(newIp);
  uint8_t IP_part_1 = ipString.substring(0, 3).toInt();
  uint8_t IP_part_2 = ipString.substring(4, 7).toInt();
  uint8_t IP_part_3 = ipString.substring(8, 10).toInt();
  uint8_t IP_part_4 = ipString.substring(11, 13).toInt();

  outIp[IP_part_1, IP_part_2, IP_part_3, IP_part_4];

  Serial.print("New remote IP: ");
  Serial.println(outIp);
}

void setMode(uint8_t mode) {
  if (mode == motorCurrentMode) {
    return;
  }
  drv.setMode(mode);
  motorCurrentMode = mode;
}
void playHaptic(uint8_t effect) {
  setMode(DRV2605_MODE_INTTRIG);

  drv.setWaveform(0, effect);  // play effect
  drv.setWaveform(1, 0);       // end waveform
  drv.go();
  //delay(200);
}

void playHapticRT(double val) { // 0 - 1
  setMode(DRV2605_MODE_REALTIME);

  int intV = min(1.0, max(0.0, val)) * 0x7F;
  //Serial.println(intV);
  drv.setRealtimeValue(intV);
}
