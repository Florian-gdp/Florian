#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include "secrets.h"

// ---------------- WIFI ----------------
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient client;

// ---------------- THINGSPEAK ----------------
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

// ---------------- SPIEL ----------------
enum SpielStatus { WARTET_AUF_START = 0, SPIELT = 1 };
SpielStatus status = WARTET_AUF_START;

// ---------------- SENSOR ----------------
struct TeamSensor {
  int pin;
  int tore = 0;

  bool imEvent = false;
  bool letzterZustand = HIGH;

  unsigned long startZeit = 0;
};

TeamSensor gelb = {D2};
TeamSensor rot  = {D1};

const int startButton = D4;

// ---------------- REGELN ----------------
const int MAXTORE = 6;

const unsigned long minBlockZeit = 80;
const unsigned long maxBlockZeit = 1000;
unsigned long torPause = 4000;

unsigned long letztesTorGlobal = 0;

// ---------------- SPIELDATEN ----------------
unsigned long spielStartZeit = 0;

int finaleSpielzeit = 0;
int ersteTorZeit = 0;

// Serien
String letztesTeam = "";
int aktuelleSerie = 0;
int besteSerieRot = 0;
int besteSerieGelb = 0;

// Comeback
bool rotLagHinten = false;
bool gelbLagHinten = false;

bool rotComeback = false;
bool gelbComeback = false;

// ---------------- WIFI ----------------
void connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Serial.print("WLAN verbindet");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWLAN OK");
  Serial.println(WiFi.localIP());
}

// ---------------- RESET ----------------
void resetSpiel() {

  rot.tore = 0;
  gelb.tore = 0;

  rot.imEvent = false;
  gelb.imEvent = false;

  rot.letzterZustand = HIGH;
  gelb.letzterZustand = HIGH;

  letztesTorGlobal = 0;

  ersteTorZeit = 0;

  aktuelleSerie = 0;
  letztesTeam = "";

  besteSerieRot = 0;
  besteSerieGelb = 0;

  rotLagHinten = false;
  gelbLagHinten = false;

  rotComeback = false;
  gelbComeback = false;

  spielStartZeit = millis();

  Serial.println("Neues Spiel gestartet");
}

// ---------------- THINGSPEAK (NUR ENDE) ----------------
void uploadToThingSpeak() {

  ThingSpeak.setField(1, finaleSpielzeit);
  ThingSpeak.setField(2, ersteTorZeit);
  ThingSpeak.setField(3, gelb.tore);
  ThingSpeak.setField(4, rot.tore);
  ThingSpeak.setField(5, max(besteSerieRot, besteSerieGelb));
  ThingSpeak.setField(6, (rotComeback || gelbComeback) ? 1 : 0);

  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

// ---------------- SENSOR LOGIK ----------------
void pruefeSensor(TeamSensor &team, const char* name) {

  if (status != SPIELT) return;

  unsigned long jetzt = millis();
  int wert = digitalRead(team.pin);

  if (wert == LOW && team.letzterZustand == HIGH) {
    team.startZeit = jetzt;
    team.imEvent = true;
  }

  if (team.imEvent && (jetzt - team.startZeit > maxBlockZeit)) {
    team.imEvent = false;
  }

  if (wert == HIGH &&
      team.letzterZustand == LOW &&
      team.imEvent) {

    unsigned long dauer = jetzt - team.startZeit;

    if (dauer >= minBlockZeit &&
        dauer <= maxBlockZeit &&
        jetzt - letztesTorGlobal >= torPause) {

      team.tore++;
      letztesTorGlobal = jetzt;

      if (ersteTorZeit == 0) {
        ersteTorZeit = (jetzt - spielStartZeit) / 1000;
      }

      if (letztesTeam == name) {
        aktuelleSerie++;
      } else {
        aktuelleSerie = 1;
        letztesTeam = name;
      }

      if (String(name) == "ROT")
        besteSerieRot = max(besteSerieRot, aktuelleSerie);
      else
        besteSerieGelb = max(besteSerieGelb, aktuelleSerie);

      if (rot.tore > gelb.tore) gelbLagHinten = true;
      if (gelb.tore > rot.tore) rotLagHinten = true;

      Serial.print("TOR ");
      Serial.print(name);
      Serial.print(" | Rot:");
      Serial.print(rot.tore);
      Serial.print(" Gelb:");
      Serial.println(gelb.tore);

      // ---------------- SPIELENDE ----------------
      if (team.tore >= MAXTORE && status == SPIELT) {

        finaleSpielzeit = (millis() - spielStartZeit) / 1000;

        if (String(name) == "ROT" && rotLagHinten)
          rotComeback = true;

        if (String(name) == "GELB" && gelbLagHinten)
          gelbComeback = true;

        status = WARTET_AUF_START;

        Serial.println("SPIEL ENDE");

        uploadToThingSpeak();   // ✅ NUR HIER EINMAL

        rot.imEvent = false;
        gelb.imEvent = false;
      }
    }

    team.imEvent = false;
  }

  team.letzterZustand = wert;
}

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  pinMode(rot.pin, INPUT_PULLUP);
  pinMode(gelb.pin, INPUT_PULLUP);
  pinMode(startButton, INPUT_PULLUP);

  connectWiFi();
  ThingSpeak.begin(client);

  spielStartZeit = millis();
}

// ---------------- LOOP ----------------
void loop() {

  if (status == WARTET_AUF_START) {

    if (digitalRead(startButton) == LOW) {
      delay(50);

      if (digitalRead(startButton) == LOW) {
        resetSpiel();
        status = SPIELT;
        Serial.println("SPIEL STARTET");
      }
    }

    return;
  }

  pruefeSensor(rot, "ROT");
  pruefeSensor(gelb, "GELB");
}