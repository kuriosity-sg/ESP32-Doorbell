// include libraries
#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h"
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>

// declare user variables
#define LED_COUNT   5
#define BRIGHTNESS  150 // NeoPixel brightness, 0 (min) to 255 (max) 

// declare pins
const int buzzerPin = 14;
const int neopixelPin = 25;

// declare variables
uint8_t data;
char logArray[5][22];

// declare objects
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
RTC_DS3231  rtc;
Adafruit_NeoPixel strip(LED_COUNT, neopixelPin, NEO_GRB + NEO_KHZ800);

void drawStartScreen() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(FF18);
  tft.drawString("DOORBELL LOGGER", 5, 5, GFXFF);
  tft.drawRect(0, 40, 320, 160, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(FF17);
  tft.drawString("MAC Address :", 5, 220, GFXFF);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(WiFi.softAPmacAddress(), 135, 220, GFXFF);
}

void drawDoorbellLog() {
  char strBuffer[22];
  DateTime now = rtc.now();
  // format datetime to become hh:mm:ss (dd/mm/yyyy)
  sprintf(strBuffer, "%02i:%02i:%02i (%02i/%02i/%04i)", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());

  // shift array 1 position down
  memcpy(logArray, &logArray[1], sizeof(logArray) - sizeof(char) * 22);
  // update array with latest value
  memcpy(logArray[4], strBuffer, sizeof(char) * 22);

  tft.fillRect(1, 41, 318, 158, TFT_BLACK);
  tft.setFreeFont(FF17);
  // draw data in array to screen
  for (int i = 4; i >= 0; i--) {
    if (strcmp(logArray[i], "") != 0) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawNumber(5 - i, 5, ((4 - i) * 30) + 50, GFXFF);
      tft.drawString(". Doorbell @", 17, ((4 - i) * 30) + 50, GFXFF);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString(logArray[i], 125, ((4 - i) * 30) + 50, GFXFF);
    }
  }
}

void alarm() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(buzzerPin, LOW);
    strip.fill(strip.Color(255, 0, 0, strip.gamma8(255)));
    strip.show();   // Send the updated pixel colors to the hardware.
    delay(100);
    digitalWrite(buzzerPin, HIGH);
    strip.fill(strip.Color(0, 0, 0, strip.gamma8(255)));
    strip.show();   // Send the updated pixel colors to the hardware.
    delay(100);
  }
}

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);
  drawDoorbellLog();
  alarm();
}

void setup() {
  Serial.begin(115200);

  // initialize pins
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);

  // initialize clock module
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // initialize tft
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  drawStartScreen();

  // initialize neopixel
  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();   // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
}

void loop() {

}
