/*---------------------------------------------------------------------------------------------

  Controller for Unity
  Communication via OSC

  V1 AB/ECAL 2022


  --------------------------------------------------------------------------------------------- */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
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
IPAddress outIp(192, 168, 45, 28);          // remote IP of your computer
const unsigned int outPort = 8888;          // remote port to send OSC
const unsigned int localPort = 9999;        // local port to listen for OSC packets (actually not used for sending)



OSCErrorCode error;

String boardName; // to be used as identifier and MDNS

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
// Timing
unsigned long lastUserInteractionMillis = 0;
int standyDelay = 1  * 60 * 1000; // time in second to wait before standby

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
  /* --------- Timing and stanby */
  if (millis() - lastUserInteractionMillis > standyDelay) {
    // start deepSleep
    goToSleep();
  }
  /* --------- SEND OSC MSGS */
  // ENCODER
  // read the state of the Encoder
  encoderCount = encoder.getCount();

  if (encoderCount != encoderPrevCount) {
    sendValues();
    //Serial.println("Encoder count = " + String((int32_t)encoderCount));
    encoderPrevCount = encoderCount;
    lastUserInteractionMillis = millis(); // reset countdown for deepsleep
  }

  // BUTTON
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  if (buttonState != buttonLastState) {
    sendValues();
    buttonLastState = buttonState;
    lastUserInteractionMillis = millis(); // reset countdown for deepsleep
  }

  /* --------- CHECK INCOMMING OSC MSGS */
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {
      msg.dispatch("/arduino/motor/rt", motorRealtime);
      msg.dispatch("/arduino/motor/cmd", motorCommand);
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
  int tryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (tryCount > 30) {
      goToSleep(); // go to sleep in not connected after 15 sec
    }
    tryCount++;
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
  boardName = getBoardName();
  char mdns[30];
  boardName.toCharArray(mdns, 30);
  Serial.println(mdns);

  WiFi.setHostname(mdns);
  MDNS.begin(mdns);
  MDNS.addService("_osc", "_udp", localPort);
  MDNS.addServiceTxt("osc", "udp", "board", "ESP32Board");

  // UPDATE IP TABLE
  updateIpTable();
  /*if (!MDNS.begin(mdns)) {
    Serial.println("Error starting mDNS");
    return;
    }*/
}

String getBoardName() {
  String board_name = WiFi.macAddress();
  board_name.replace(":", "");
  board_name = "magic" + board_name.substring(0, 3);
  return board_name;
}

void motorRealtime(OSCMessage &msg) {
  int motorValue = msg.getInt(0);
  Serial.print("/arduino/motor/rt: ");
  Serial.println(motorValue);
  double motorInput = (float) motorValue / 100;
  playHapticRT(motorInput);
}

void motorCommand(OSCMessage &msg) {
  int motorCommand = msg.getInt(0);
  Serial.print("/arduino/motor/cmd: ");
  Serial.println(motorCommand);
  playHaptic(motorCommand);
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

void goToSleep() {
  Serial.println("************* going to sleep, bye! *************");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
  esp_deep_sleep_start();
}

void updateIpTable() {
  httpclient.begin("https://ecal-mid.ch/magicleap/ip.php?name=" + getBoardName() + "&ip=" + WiFi.localIP().toString() + "&wifi=" + WIFI_SSID);
  int httpResponseCode = httpclient.GET();
  if (httpResponseCode > 0) {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    String payload = httpclient.getString();
    Serial.println(payload);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  httpclient.end();
}
