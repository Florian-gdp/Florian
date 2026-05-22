#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 6

CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];


enum SpielStatus { START_ANIMATION = 0, SPIELT = 1 };
SpielStatus status = START_ANIMATION;


const int rotPin = D1;
const int gelbPin = D2;
const int startButton = D4;


int rotTore = 0;
int gelbTore = 0;

const int MAXTORE = 6;


const unsigned long minBlockZeit = 20;
const unsigned long maxBlockZeit = 300;
unsigned long torPause = 6000;
unsigned long letztesTor = 0;


unsigned long endZeit = 0;
bool endAktiv = false;


struct SensorState {
  int lastState = HIGH;
  unsigned long startZeit = 0;
  bool imEvent = false;
};

SensorState rotState;
SensorState gelbState;


void updateLEDs() {

  for (int i = 0; i < NUM_LEDS; i++) {
    leds1[i] = (i < rotTore) ? CRGB::Red : CRGB::Black;
    leds2[i] = (i < gelbTore) ? CRGB::Yellow : CRGB::Black;
  }

  FastLED.show();
}


void startAnimation() {

  CRGB colors[] = { CRGB::Blue, CRGB::Red, CRGB::Green };

  for (int cycle = 0; cycle < 3; cycle++) {
    for (int i = 0; i < 3; i++) {

      fill_solid(leds1, NUM_LEDS, colors[i]);
      fill_solid(leds2, NUM_LEDS, colors[i]);

      FastLED.show();
      delay(250);
    }
  }

  fill_solid(leds1, NUM_LEDS, CRGB::Black);
  fill_solid(leds2, NUM_LEDS, CRGB::Black);
  FastLED.show();
}


void blinkGoal(CRGB* strip, int score, CRGB color) {

  unsigned long start = millis();

  while (millis() - start < 2000) {

    for (int i = 0; i < NUM_LEDS; i++) {
      strip[i] = (i < score) ? color : CRGB::Black;
    }
    FastLED.show();
    delay(200);

    fill_solid(strip, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(200);
  }
}


void winnerBlink(CRGB* strip, CRGB color) {

  unsigned long start = millis();

  while (millis() - start < 5000) {

    fill_solid(strip, NUM_LEDS, color);
    FastLED.show();
    delay(300);

    fill_solid(strip, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(300);
  }


  fill_solid(strip, NUM_LEDS, color);
  FastLED.show();


  endZeit = millis();
  endAktiv = true;
}


void resetSpiel() {

  rotTore = 0;
  gelbTore = 0;
  letztesTor = 0;

  rotState = SensorState();
  gelbState = SensorState();

  endAktiv = false;

  updateLEDs();

  Serial.println("Neues Spiel gestartet");
}


void checkSensor(int pin, const char* team, SensorState &s, int &tore) {

  int state = digitalRead(pin);
  unsigned long jetzt = millis();

  if (state == LOW && s.lastState == HIGH) {
    s.startZeit = jetzt;
    s.imEvent = true;
  }

  if (s.imEvent && (jetzt - s.startZeit > maxBlockZeit)) {
    s.imEvent = false;
  }

  if (state == HIGH && s.lastState == LOW && s.imEvent) {

    unsigned long dauer = jetzt - s.startZeit;

    if (dauer >= minBlockZeit &&
        dauer <= maxBlockZeit &&
        (jetzt - letztesTor >= torPause)) {

      tore++;
      letztesTor = jetzt;

      Serial.print("TOR ");
      Serial.print(team);
      Serial.print(" | Rot:");
      Serial.print(rotTore);
      Serial.print(" Gelb:");
      Serial.println(gelbTore);

      if (strcmp(team, "ROT") == 0) {
        blinkGoal(leds1, rotTore, CRGB::Red);
      } else {
        blinkGoal(leds2, gelbTore, CRGB::Yellow);
      }

      updateLEDs();

    
      if (rotTore >= MAXTORE || gelbTore >= MAXTORE) {

        status = START_ANIMATION;

        Serial.println("SPIEL ENDE");

        if (rotTore >= gelbTore) {
          Serial.println("GEWINNER: ROT");
          winnerBlink(leds1, CRGB::Red);
        } else {
          Serial.println("GEWINNER: GELB");
          winnerBlink(leds2, CRGB::Yellow);
        }
      }
    }

    s.imEvent = false;
  }

  s.lastState = state;
}


void setup() {
  Serial.begin(115200);

  pinMode(rotPin, INPUT_PULLUP);
  pinMode(gelbPin, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);

  FastLED.addLeds<WS2812B, 12, GRB>(leds1, NUM_LEDS);
  FastLED.addLeds<WS2812B, 13, GRB>(leds2, NUM_LEDS);
  FastLED.setBrightness(80);

  updateLEDs();
}


void loop() {


  if (endAktiv) {

    if (millis() - endZeit >= 10000) {

      fill_solid(leds1, NUM_LEDS, CRGB::Black);
      fill_solid(leds2, NUM_LEDS, CRGB::Black);
      FastLED.show();

      endAktiv = false;
      status = START_ANIMATION;

      Serial.println("SYSTEM AUS");
    }

    return;
  }


  if (status == START_ANIMATION) {

    if (digitalRead(startButton) == LOW) {
      delay(50);

      if (digitalRead(startButton) == LOW) {

        resetSpiel();
        status = SPIELT;

        Serial.println("SPIEL STARTET");

        startAnimation();
        updateLEDs();
      }
    }

    return;
  }


  checkSensor(rotPin, "ROT", rotState, rotTore);
  checkSensor(gelbPin, "GELB", gelbState, gelbTore);
}