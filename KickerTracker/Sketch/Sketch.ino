#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#define NUM_LEDS 6


// ================= WIFI ====================
WiFiClient client;
const char* sophosLoginUrl ="https://10.10.75.1:4502/index.cgi?action=login";


// ================= FASTLED =================
CRGB ledsRed[NUM_LEDS];
CRGB ledsYellow[NUM_LEDS];

// ================= SPIEL STATUS =================
enum SpielStatus {
  START = 0,
  SPIELT = 1
};
SpielStatus status = START;

int rotTore = 0;
int gelbTore = 0;
const int MAXTORE = 6;

// ================= PINS =================
const int rotPin = D1;
const int gelbPin = D2;
const int startButton = D4;
const int ledRedPin = 12
const int ledYellowPin = 13

// ================= BLOCKZEIT SENSOR =================
const unsigned long minBlockZeit = 20;
const unsigned long maxBlockZeit = 300;

// ================= BLOCKZEIT TORE =================
unsigned long torPause = 6000;
unsigned long letztesTor = 0;


// ================= THINGSPEAK =================
unsigned long spielStartZeit = 0;
unsigned long erstesTorZeit = 0;
bool erstesTorGefallen = false;
String letzteMannschaft = "";
int aktuelleSerie = 0;
int besteSerie = 0;
int maxRueckstand = 0;


// ================= SENSOR =================
struct SensorState {
  int lastState = HIGH;
  unsigned long changeTime = 0;
};

SensorState rotState;
SensorState gelbState;


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  connectWiFi();

  // init sensor pins
  pinMode(rotPin, INPUT_PULLUP);
  pinMode(gelbPin, INPUT_PULLUP);

  // init button pins
  pinMode(startButton, INPUT_PULLUP);

  // init fastLed
  FastLED.addLeds<WS2812B, ledRedPin, GRB>(ledsRed, NUM_LEDS);
  FastLED.addLeds<WS2812B, ledYellowPin, GRB>(ledsYellow, NUM_LEDS);
  FastLED.setBrightness(80);

  clearAllLeds();
}

// =====================================================
// GAME LOOP
// =====================================================

void loop() {

  if (status == START) {

    if (digitalRead(startButton) == LOW) {
      delay(50);
      if (digitalRead(startButton) == LOW) {

        Serial.println("SPIEL STARTET");

        // RESET ALLER WERTE
        rotTore = 0;
        gelbTore = 0;
        letztesTor = 0;
        spielStartZeit = millis();
        erstesTorGefallen = false;
        erstesTorZeit = 0;
        aktuelleSerie = 0;
        besteSerie = 0;
        letzteMannschaft = "";
        maxRueckstand = 0;
        clearAllLeds();
        delay(50);

        // startanimation  
        startAnimation();

        // wechsel zu status spielt nach startanimation
        status = SPIELT;
      }
    }

    return;
  }

  if (status == SPIELT) {
    checkSensor(rotPin, "ROT", rotState, rotTore);
    checkSensor(gelbPin, "GELB", gelbState, gelbTore);
  }
}


// =====================================================
// SCORE LEDS
// =====================================================

void updateLEDs() {

  clearAllLeds();

  for (int i = 0; i < rotTore; i++) {
    ledsRed[i] = CRGB::Red;
  }

  for (int i = 0; i < gelbTore; i++) {
    ledsYellow[i] = CRGB::Yellow;
  }

  FastLED.show();
}

// =====================================================
// START ANIMATION
// =====================================================

void startAnimation() {

  clearAllLeds();

  CRGB colors[] = {
    CRGB::Blue,
    CRGB::Red,
    CRGB::Green
  };

  for (int cycle = 0; cycle < 3; cycle++) {

    for (int i = 0; i < 3; i++) {

      fill_solid(ledsRed, NUM_LEDS, colors[i]);
      fill_solid(ledsYellow, NUM_LEDS, colors[i]);

      FastLED.show();

      delay(250);
    }
  }

  clearAllLeds();

  FastLED.show();
}

// =====================================================
// TOR BLINKEN
// =====================================================

void blinkGoal(CRGB* strip, int score, CRGB color) {

  unsigned long start = millis();
  while (millis() - start < 3500) {

    bool on = ((millis() / 350) % 2 == 0);

    if (on) {
      for (int i = 0; i < NUM_LEDS; i++) {
        strip[i] = (i < score) ? color : CRGB::Black;
      }
    } else {
      fill_solid(strip, NUM_LEDS, CRGB::Black);
    }

    FastLED.show();
    delay(40);
  }

  for (int i = 0; i < NUM_LEDS; i++) {

    strip[i] = (i < score) ? color : CRGB::Black;
  }

  FastLED.show();
}

// =====================================================
// GEWINNER BLINKT
// =====================================================

void winnerBlinkEnd(CRGB color, bool rotGewonnen) {

  unsigned long start = millis();

  while (millis() - start < 10000) {

    bool on = ((millis() / 600) % 2 == 0);

    if (on) {
      if (rotGewonnen) {
        fill_solid(ledsRed, NUM_LEDS, color);
        fill_solid(ledsYellow,  NUM_LEDS, CRGB::Black);
      } else {
        fill_solid(ledsRed, NUM_LEDS, CRGB::Black);
        fill_solid(ledsYellow, NUM_LEDS, color);
      }
    } else {
      clearAllLeds();
    }

    FastLED.show();
    delay(30);
  }

  clearAllLeds();

  // RESET STATUS
  status = START;
}

// =====================================================
// SERIEN
// =====================================================

void updateSerie(const char* team) {

  if (letzteMannschaft == team) {
    aktuelleSerie++;
  } else {
    aktuelleSerie = 1;
    letzteMannschaft = team;
  }

  if (aktuelleSerie > besteSerie) {
    besteSerie = aktuelleSerie;
  }
}

// =====================================================
// RÜCKSTAND
// =====================================================

void updateRueckstand() {

  int differenz = abs(rotTore - gelbTore);

  if (differenz > maxRueckstand) {
    maxRueckstand = differenz;
  }
}

// =====================================================
// BLOCKZEIT
// =====================================================

bool isStableSignal(int pin, SensorState &s, int minTime, int maxTime) {

  int state = digitalRead(pin);
  unsigned long now = millis();

  if (state != s.lastState) {
    s.changeTime = now;
  }

  unsigned long stableTime =
      now - s.changeTime;

  if (stableTime < minTime)
    return false;

  if (stableTime > maxTime)
    return false;

  return true;
}


// =====================================================
// LEDS AUS
// =====================================================

void clearAllLeds() {

  fill_solid(ledsRed, NUM_LEDS, CRGB::Black);
  fill_solid(ledsYellow, NUM_LEDS, CRGB::Black);

  FastLED.show();
}


// =====================================================
// SENSOR
// =====================================================

void checkSensor(int pin, const char* team, SensorState &s, int &tore) {

  int state = digitalRead(pin);

  unsigned long now = millis();

  bool stable = isStableSignal(pin, s,  minBlockZeit, maxBlockZeit);

  // FALLENDE FLANKE
  if (s.lastState == HIGH && state == LOW && stable) {

    if (now - letztesTor > torPause) {

      tore++;
      letztesTor = now;

      // ERSTES TOR
      if (!erstesTorGefallen) {
        erstesTorGefallen = true;
        erstesTorZeit = millis();
      }

      updateSerie(team);
      updateRueckstand();

      Serial.print("TOR ");
      Serial.print(team);
      Serial.print(" | Rot:");
      Serial.print(rotTore);
      Serial.print(" Gelb:");
      Serial.println(gelbTore);

      // BLINKEN
      if (strcmp(team, "ROT") == 0) {
        blinkGoal(ledsRed, rotTore, CRGB::Red);
      } else {
        blinkGoal(ledsYellow, gelbTore, CRGB::Yellow);
      }

      updateLEDs();

      // SPIEL ENDE
      if (rotTore >= MAXTORE || gelbTore >= MAXTORE) {

        Serial.println("SPIEL ENDE");
        sendToThingSpeak();

        if (rotTore > gelbTore) {
          Serial.println("GEWINNER: ROT");
          winnerBlinkEnd(CRGB::Red, true);

        } else {
          Serial.println("GEWINNER: GELB");
          winnerBlinkEnd(CRGB::Yellow, false);
        }
      }
    }
  }
  s.lastState = state;
}

// =====================================================
// WLAN
// =====================================================

void connectWiFi() {

  Serial.print("Verbinde WLAN");

  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WLAN verbunden");

  ThingSpeak.begin(client);

  Serial.println("ThingSpeak bereit");
}

// =====================================================
// THINGSPEAK SENDEN
// =====================================================

void sendToThingSpeak() {

  unsigned long spielLaenge = (millis() - spielStartZeit) / 1000;
  unsigned long sekBisErstesTor = 0;

  if (erstesTorGefallen) {
    sekBisErstesTor = (erstesTorZeit - spielStartZeit) / 1000;
  }

  ThingSpeak.setField(1, (long)spielLaenge);
  ThingSpeak.setField(2, (long)sekBisErstesTor);
  ThingSpeak.setField(3, gelbTore);
  ThingSpeak.setField(4, rotTore);
  ThingSpeak.setField(5, besteSerie);
  ThingSpeak.setField(6, maxRueckstand);

  int response = ThingSpeak.writeFields( SECRET_CH_ID, SECRET_WRITE_APIKEY);

  if (response == 200) {
    Serial.println("ThingSpeak erfolgreich gesendet");
  } else {
    Serial.print("ThingSpeak Fehler: ");
    Serial.println(response);
  }
}

// =====================================================
// GÄSTE LAN LOGIN
// =====================================================

void doSophosLogin() {

  std::unique_ptr<BearSSL::WiFiClientSecure> secureClient(new BearSSL::WiFiClientSecure);
  secureClient->setInsecure();

  HTTPClient https;

  if (!https.begin(*secureClient, sophosLoginUrl)) {
    Serial.println("Sophos begin failed");
    return;
  }

  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = https.POST("accept=true");

  Serial.print("Sophos Code: ");
  Serial.println(code);

  if (code > 0) {
    Serial.println(https.getString());
  }

  https.end();
}