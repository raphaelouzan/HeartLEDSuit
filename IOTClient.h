#include <SPI.h>
#include <Adafruit_WINC1500.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WIFI *********************************/

char ssid[] = "Kandouni";     //  your network SSID (name)
char pass[] = "anyuni123";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "rapha2"
#define AIO_KEY         "3a273dd5867d4b87b395861ce9933e13"


// Define the WINC1500 board connections below.
// If you're following the Adafruit WINC1500 board
// guide you don't need to modify these:
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2     // or, tie EN to VCC and comment this out
// The SPI pins of the WINC1500 (SCK, MOSI, MISO) should be
// connected to the hardware SPI port of the Arduino.

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

int status = WL_IDLE_STATUS;

Adafruit_WINC1500Client client;

const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// Callbacks

void onNewBrightness(uint32_t brightness) { 
  PRINTX("Brightness update: ", brightness); 
  FastLED.setBrightness(brightness);
}

void onOnOffButton(char* buttonStatus, uint16_t len) { 
  PRINTX("ON/OFF button is ", buttonStatus);
  if (0 == strcmp((char *)buttonStatus, "OFF")) {
    FastLED.setBrightness(0);
  }

  if (0 == strcmp((char *)buttonStatus, "ON")) {
    FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  }
}

void onNewBPM(uint32_t bpm) { 
  PRINTX("Got new BPM", bpm); 
}

void onNextButton(uint32_t state) { 

  if (state == 1) { 
    PRINT("Moving to next animation");
    onClickFromIoT();
  }

}


// Feeds

const char BPM_FEED[] PROGMEM = AIO_USERNAME "/feeds/bpm";
Adafruit_MQTT_Subscribe bpmFeed = Adafruit_MQTT_Subscribe(&mqtt, BPM_FEED);

const char ONOFF_FEED[] PROGMEM = AIO_USERNAME "/feeds/onoff";
Adafruit_MQTT_Subscribe onoffbuttonFeed = Adafruit_MQTT_Subscribe(&mqtt, ONOFF_FEED);

const char NEXT_FEED[] PROGMEM = AIO_USERNAME "/feeds/next";
Adafruit_MQTT_Subscribe nextbuttonFeed = Adafruit_MQTT_Subscribe(&mqtt, NEXT_FEED);

const char BRIGHTNESS_FEED[] PROGMEM = AIO_USERNAME "/feeds/brightness";
Adafruit_MQTT_Subscribe brightnessFeed = Adafruit_MQTT_Subscribe(&mqtt, BRIGHTNESS_FEED);

// Publishers 

const char HEARTBUTTON_FEED[] PROGMEM = AIO_USERNAME "/feeds/heartbutton";
Adafruit_MQTT_Publish heartbuttonPublish = Adafruit_MQTT_Publish(&mqtt, HEARTBUTTON_FEED);

///////////


void printWifiStatus() {
  PRINTX("SSID: ", WiFi.SSID());
  PRINTX("IP Address: ", WiFi.localIP());
  PRINTX("Signal strength (RSSI in dBm): ", WiFi.RSSI());
}


void setupIoTConnection() { 

#ifdef WINC_EN
  pinMode(WINC_EN, OUTPUT);
  digitalWrite(WINC_EN, HIGH);
#endif
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    PRINT("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    PRINTX("Attempting to connect to SSID: ", ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }

  PRINT("Connected to wifi:");
  printWifiStatus();


  brightnessFeed.setCallback(onNewBrightness);
  bpmFeed.setCallback(onNewBPM);
  onoffbuttonFeed.setCallback(onOnOffButton);
  nextbuttonFeed.setCallback(onNextButton);

  mqtt.subscribe(&bpmFeed);
  mqtt.subscribe(&onoffbuttonFeed);
  mqtt.subscribe(&brightnessFeed);
  mqtt.subscribe(&nextbuttonFeed);
  
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }
  
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  } 
  
  PRINT("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    PRINTX("Error connecting ", mqtt.connectErrorString(ret));
    PRINT("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }

  PRINT("MQTT Connected!");

}
 

void loopMQTT(uint8_t requestedDelay) { 

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  mqtt.processPackets(requestedDelay); 

}

void publishButtonTap() { 

  PRINT_NOLN("Sending heart tap to AIO...");
  if (heartbuttonPublish.publish((uint32_t)1)) { 
    PRINT("SUCCESS")
  } else { 
    PRINT("FAILED!")
  }

}


