#include <Arduino.h>
#include <Arduino_Modulino.h>
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <RTC.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <credentials.h>
#include <iostream>
#include <string>

ModulinoThermo thermo;
ModulinoBuzzer buzzer;
ArduinoLEDMatrix matrix;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// buzzer variables
int frequency = 840; // Frequency of the tone in Hz
int duration = 1000; // Duration of the tone in milliseconds

// cap. sensor constants
const int dry = 500; // value for dry sensor (0%)
const int wet = 230; // constant for wet sensor (100%)

// air humidity
int airHumidity;

// cap. sensors
int sensorVal1;
int sensorVal2;
int sensorVal3;
int soilHumidity1;
int soilHumidity2;
int soilHumidity3;

// Time
String getTime;
int getHours;

// Millis
unsigned long now;
bool switchPage;
bool refreshPage;
static bool active;
static unsigned long lastMillis;
static int pulseCount;
static unsigned long cooldownUntil;             // timestamp until which re-trigger is blocked
const unsigned long pulseInterval = 3000UL;     // time between pulse starts (ms)
const unsigned long cooldownDuration = 60000UL; // 1 minute cooldown after sequence (ms)

// Screen
String text;
int16_t tx1, ty1;
uint16_t w, h;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
int bestSize;

// WIFI
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASS;

// display toggle (non-blocking)
int displayPage = 0;
unsigned long lastDisplaySwitch = 0;
unsigned long lastDisplayRefresh = 0;
const unsigned long displayRefreshInterval = 500UL; // ms (refresh while page shown)
const unsigned long displayInterval = 5000UL;       // ms

void alarm(int airHumidity, int getHours);
void screen(const String &printDisplay1, const String &printDisplay2, const String &printDisplay3);
void updateReadings();

void setup()
{
  Serial.begin(9600);

  // led matrix
  matrix.begin();

  // modulino
  Modulino.begin();
  thermo.begin();
  buzzer.begin();

  // display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // WiFi
  screen("Connecting to WiFi:", ssid, "");
  WiFi.begin(ssid, password);
  screen("WiFi connected", "", "");
  delay(2000);

  // Time from WIFI
  timeClient.begin();
  timeClient.setTimeOffset(3600);
}

void loop()
{
  // update sensors and time once per loop (will also refresh again before drawing)
  updateReadings();
  getTime = timeClient.getFormattedTime();
  getHours = timeClient.getHours();

  // alarm RH > 60% and before 10pm-8am
  alarm(airHumidity, getHours);

  // non-blocking display: switch pages every `displayInterval` ms,
  // but refresh the currently visible content every `displayRefreshInterval` ms
  now = millis();

  // first-time init
  if (lastDisplaySwitch == 0)
  {
    lastDisplaySwitch = lastDisplayRefresh = now;
    displayPage = 0;
  }

  switchPage = (now - lastDisplaySwitch) >= displayInterval;
  refreshPage = (now - lastDisplayRefresh) >= displayRefreshInterval;

  if (switchPage)
  {
    lastDisplaySwitch = now;
    lastDisplayRefresh = now;
    // If currently showing an alarm page, only go back to page 0 when
    // that alarm condition is no longer true. Otherwise continue cycling
    // through the normal pages 0..4.
    if (displayPage >= 5)
    {
      bool alarmStillActive = false;
      if (displayPage == 5 && airHumidity > 60)
        alarmStillActive = true;
      if (displayPage == 6 && soilHumidity1 < 35)
        alarmStillActive = true;
      if (displayPage == 7 && soilHumidity2 < 25)
        alarmStillActive = true;
      if (displayPage == 8 && soilHumidity3 < 35)
        alarmStillActive = true;

      if (!alarmStillActive)
      {
        displayPage = 0;
      }
      // if still active, keep the alarm page
    }
    else
    {
      displayPage = displayPage + 1; // next regular page
      if (displayPage > 4)
        displayPage = 0;
    }
  }

  if (switchPage || refreshPage)
  {
    lastDisplayRefresh = now;
    updateReadings();

    // Alarm display
    // Air
    if (airHumidity > 60)
    {
      displayPage = 5;
    }
    // Ginseng Bonsai
    if (soilHumidity1 < 35)
    {
      displayPage = 6;
    }
    // Drachenbaum
    if (soilHumidity2 < 25)
    {
      displayPage = 7;
    }
    // Ficus Bonsai
    if (soilHumidity3 < 35)
    {
      displayPage = 8;
    }

    if (displayPage == 0)
    {
      screen("", getTime, "");
    }
    else if (displayPage == 1)
    {
      screen("Luftfeuchte: ", String(airHumidity), "%");
    }
    else if (displayPage == 2)
    {
      screen("Ginseng Bonsai: ", String(soilHumidity1), "%");
    }
    else if (displayPage == 3)
    {
      screen("Drachenbaum: ", String(soilHumidity2), "%");
    }
    else if (displayPage == 4)
    {
      screen("Ficus Bonsai: ", String(soilHumidity3), "%");
    }
    else if (displayPage == 5)
    {
      screen("ALARM! Air Humidity too high!: ", String(airHumidity), "%");
    }
    else if (displayPage == 6)
    {
      screen("ALARM! Ginsen Bonsai dry!: ", String(soilHumidity1), "%");
    }
    else if (displayPage == 7)
    {
      screen("ALARM! Drachenbaum dry!: ", String(soilHumidity2), "%");
    }
    else if (displayPage == 8)
    {
      screen("ALARM! Ficus Bonsai dry!: ", String(soilHumidity3), "%");
    }
  }
}

void alarm(int airHumidity, int getHours)
{
  // Non-blocking alarm using millis() to avoid delaying the main loop.
  // When air humidity > 60% and between 09:00 and 21:59 starts a sequence of 3 pulses
  // spaced by `pulseInterval`. Preserve state across calls (do not reset globals here).
  now = millis();

  // If we're in cooldown after a full sequence, don't start a new one
  if (now < cooldownUntil)
  {
    if (active)
    {
      active = false;
      buzzer.tone(0, 0);
    }
    return;
  }

  // Start alarm sequence when threshold exceeded
  if (airHumidity > 60 && getHours > 8 && getHours < 22)
  {
    if (!active)
    {
      active = true;
      pulseCount = 0;
      lastMillis = now - pulseInterval; // allow immediate first pulse
    }
  }
  else
  {
    // Stop alarm immediately when humidity drops
    if (active)
    {
      active = false;
      buzzer.tone(0, 0);
      pulseCount = 0;
    }
    return;
  }

  if (!active)
    return;

  if (pulseCount < 3 && (now - lastMillis) >= pulseInterval)
  {
    buzzer.tone(frequency, duration);
    lastMillis = now;
    ++pulseCount;
  }

  if (pulseCount >= 3)
  {
    // finished sequence: stop buzzer and set cooldown so alarm won't restart for 1 minute
    active = false;
    buzzer.tone(0, 0);
    cooldownUntil = now + cooldownDuration;
    pulseCount = 0;
  }
}

void screen(const String &printDisplay1, const String &printDisplay2, const String &printDisplay3)
{
  text = printDisplay1 + printDisplay2 + printDisplay3;
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Choose the largest text size that fits within the screen bounds.
  bestSize = 1;
  // Try sizes from 6 down to 1 (adjust max if you need larger fonts on bigger displays)
  for (int sz = 6; sz >= 1; --sz)
  {
    display.setTextSize(sz);
    display.getTextBounds(text, 0, 0, &tx1, &ty1, &w, &h);
    if (w <= SCREEN_WIDTH && h <= SCREEN_HEIGHT)
    {
      bestSize = sz;
      break;
    }
  }

  display.setTextSize(bestSize);
  display.getTextBounds(text, 0, 0, &tx1, &ty1, &w, &h);
  // center text
  display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
  display.print(text);
  display.display();
}

void updateReadings()
{
  timeClient.update();
  airHumidity = thermo.getHumidity();
  sensorVal1 = analogRead(A0);
  sensorVal2 = analogRead(A1);
  sensorVal3 = analogRead(A2);
  soilHumidity1 = map(sensorVal1, wet, dry, 100, 0);
  soilHumidity2 = map(sensorVal2, wet, dry, 100, 0);
  soilHumidity3 = map(sensorVal3, wet, dry, 100, 0);
}