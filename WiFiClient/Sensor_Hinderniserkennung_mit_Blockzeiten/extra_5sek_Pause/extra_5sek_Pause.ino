const int sensorPin = 4;

int tore = 0;
const int MAXTORE = 6;

// Zeitfenster für echte Ballbewegung
bool imEvent = false;
unsigned long startZeit = 0;

const unsigned long minBlockZeit = 80;     // zu kurz = ignorieren
const unsigned long maxBlockZeit = 1500;   // zu lang = Hand

bool letzterZustand = HIGH;

// Mindestpause zwischen Toren
unsigned long letztesTor = 0;
const unsigned long torPause = 5000; // 5 Sekunden

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);

  Serial.println("Spiel startet!");
}

void loop() {

  // ===== Gewonnen =====
  if (tore >= MAXTORE) {
    Serial.println("🏆 GEWONNEN!");
    while (true) {
      // Spiel stoppt
    }
  }

  int wert = digitalRead(sensorPin);
  unsigned long jetzt = millis();

  // ===== Start der Unterbrechung =====
  if (wert == LOW && letzterZustand == HIGH) {
    startZeit = jetzt;
    imEvent = true;
  }

  // ===== Ende der Unterbrechung =====
  if (wert == HIGH && imEvent) {

    unsigned long dauer = jetzt - startZeit;

    // Prüfen:
    // 1. echte Ballzeit
    // 2. 5 Sekunden seit letztem Tor vorbei
    if (dauer >= minBlockZeit &&
        dauer <= maxBlockZeit &&
        jetzt - letztesTor >= torPause) {

      tore++;
      letztesTor = jetzt;

      Serial.print("TOR! Stand: ");
      Serial.println(tore);
    }

    imEvent = false;
  }

  letzterZustand = wert;
}