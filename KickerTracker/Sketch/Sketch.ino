#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#define NUM_LEDS 6

CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];

WiFiClient client;

// ================= STATUS =================
enum SpielStatus {
  START_ANIMATION = 0,
  SPIELT = 1
};

SpielStatus status = START_ANIMATION;

// ================= PINS =================
const int rotPin = D1;
const int gelbPin = D2;
const int startButton = D4;

// ================= SPIEL =================
int rotTore = 0;
int gelbTore = 0;

const int MAXTORE = 6;

// ================= BLOCKZEIT =================
const unsigned long minBlockZeit = 20;
const unsigned long maxBlockZeit = 300;

unsigned long torPause = 6000;
unsigned long letztesTor = 0;

// ================= SPIEL ENDE =================
bool spielEnde = false;

// ================= THINGSPEAK =================
unsigned long spielStartZeit = 0;

unsigned long erstesTorZeit = 0;
bool erstesTorGefallen = false;

String letzteMannschaft = "";

int aktuelleSerie = 0;
int besteSerie = 0;

int maxRueckstand = 0;
const char* sophosLoginUrl ="https://10.10.75.1:4502/index.cgi?action=login";
// ================= SENSOR =================
struct SensorState {
  int lastState = HIGH;
  unsigned long changeTime = 0;
};

SensorState rotState;
SensorState gelbState;

// =====================================================
// LEDS AUS
// =====================================================

void clearAllLeds() {

  fill_solid(leds1, NUM_LEDS, CRGB::Black);
  fill_solid(leds2, NUM_LEDS, CRGB::Black);

  FastLED.show();
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

  unsigned long spielLaenge =
      (millis() - spielStartZeit) / 1000;
  Serial.print("Spiellänge");
    Serial.println(spielLaenge);
  unsigned long sekBisErstesTor = 0;

  if (erstesTorGefallen) {
    sekBisErstesTor =
        (erstesTorZeit - spielStartZeit) / 1000;
  }

  ThingSpeak.setField(1, (long)spielLaenge);
  ThingSpeak.setField(2, (long)sekBisErstesTor);
  ThingSpeak.setField(3, gelbTore);
  ThingSpeak.setField(4, rotTore);
  ThingSpeak.setField(5, besteSerie);
  ThingSpeak.setField(6, maxRueckstand);

  int response =
      ThingSpeak.writeFields(
          SECRET_CH_ID,
          SECRET_WRITE_APIKEY);

  if (response == 200) {

    Serial.println("ThingSpeak erfolgreich gesendet");

  } else {

    Serial.print("ThingSpeak Fehler: ");
    Serial.println(response);
  }
}

// =====================================================
// SCORE LEDS
// =====================================================

void updateLEDs() {

  clearAllLeds();

  for (int i = 0; i < rotTore; i++) {
    leds1[i] = CRGB::Red;
  }

  for (int i = 0; i < gelbTore; i++) {
    leds2[i] = CRGB::Yellow;
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

      fill_solid(leds1, NUM_LEDS, colors[i]);
      fill_solid(leds2, NUM_LEDS, colors[i]);

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

void blinkGoal(CRGB* strip,
               int score,
               CRGB color) {

  unsigned long start = millis();

  while (millis() - start < 3500) {

    bool on =
        ((millis() / 350) % 2 == 0);

    if (on) {

      for (int i = 0; i < NUM_LEDS; i++) {

        strip[i] =
            (i < score)
                ? color
                : CRGB::Black;
      }

    } else {

      fill_solid(strip,
                 NUM_LEDS,
                 CRGB::Black);
    }

    FastLED.show();

    delay(40);
  }

  for (int i = 0; i < NUM_LEDS; i++) {

    strip[i] =
        (i < score)
            ? color
            : CRGB::Black;
  }

  FastLED.show();
}

// =====================================================
// GEWINNER BLINKT
// =====================================================

void winnerBlinkEnd(CRGB color,
                    bool rotGewonnen) {

  unsigned long start = millis();

  while (millis() - start < 10000) {

    bool on =
        ((millis() / 600) % 2 == 0);

    if (on) {

      if (rotGewonnen) {

        fill_solid(leds1,
                   NUM_LEDS,
                   color);

        fill_solid(leds2,
                   NUM_LEDS,
                   CRGB::Black);

      } else {

        fill_solid(leds1,
                   NUM_LEDS,
                   CRGB::Black);

        fill_solid(leds2,
                   NUM_LEDS,
                   color);
      }

    } else {

      clearAllLeds();
    }

    FastLED.show();

    delay(30);
  }

  clearAllLeds();

  spielEnde = true;
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

  int differenz =
      abs(rotTore - gelbTore);

  if (differenz > maxRueckstand) {
    maxRueckstand = differenz;
  }
}

// =====================================================
// BLOCKZEIT
// =====================================================

bool isStableSignal(int pin,
                    SensorState &s,
                    int minTime,
                    int maxTime) {

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
// SENSOR
// =====================================================

void checkSensor(int pin,
                 const char* team,
                 SensorState &s,
                 int &tore) {

  if (spielEnde)
    return;

  int state = digitalRead(pin);

  unsigned long now = millis();

  bool stable =
      isStableSignal(pin,
                     s,
                     minBlockZeit,
                     maxBlockZeit);

  // FALLENDE FLANKE
  if (s.lastState == HIGH &&
      state == LOW &&
      stable) {

    if (now - letztesTor >
        torPause) {

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

        blinkGoal(leds1,
                  rotTore,
                  CRGB::Red);

      } else {

        blinkGoal(leds2,
                  gelbTore,
                  CRGB::Yellow);
      }

      updateLEDs();

      // SPIEL ENDE
      if (rotTore >= MAXTORE ||
          gelbTore >= MAXTORE) {

        Serial.println("SPIEL ENDE");

        sendToThingSpeak();

        if (rotTore > gelbTore) {

          Serial.println("GEWINNER: ROT");

          winnerBlinkEnd(
              CRGB::Red,
              true);

        } else {

          Serial.println("GEWINNER: GELB");

          winnerBlinkEnd(
              CRGB::Yellow,
              false);
        }
      }
    }
  }

  s.lastState = state;
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  connectWiFi();

  pinMode(rotPin, INPUT_PULLUP);
  pinMode(gelbPin, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);

  FastLED.addLeds<WS2812B,
                   12,
                   GRB>(
      leds1,
      NUM_LEDS);

  FastLED.addLeds<WS2812B,
                   13,
                   GRB>(
      leds2,
      NUM_LEDS);

  FastLED.setBrightness(80);

  clearAllLeds();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  if (spielEnde)
    return;

  // START
  if (status ==
      START_ANIMATION) {

    if (digitalRead(startButton)
        == LOW) {

      delay(50);

      if (digitalRead(startButton)
          == LOW) {

        Serial.println(
            "SPIEL STARTET");

        rotTore = 0;
        gelbTore = 0;

        letztesTor = 0;

        spielEnde = false;

        // THINGSPEAK RESET
        spielStartZeit = millis();

        erstesTorGefallen = false;
        erstesTorZeit = 0;

        aktuelleSerie = 0;
        besteSerie = 0;

        letzteMannschaft = "";

        maxRueckstand = 0;

        clearAllLeds();

        delay(50);

        startAnimation();

        status = SPIELT;
      }
    }

    return;
  }

  checkSensor(rotPin,
              "ROT",
              rotState,
              rotTore);

  checkSensor(gelbPin,
              "GELB",
              gelbState,
              gelbTore);
}
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