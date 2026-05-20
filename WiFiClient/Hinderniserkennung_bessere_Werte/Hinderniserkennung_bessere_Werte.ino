const int sensorPin = 4;

int tore = 0;
const int MAXTORE = 6;

// Zeitfenster gegen Hand / echte Ballbewegung
bool imEvent = false;
unsigned long startZeit = 0;

const unsigned long minBlockZeit = 50;    // zu kurz = ignorieren
const unsigned long maxBlockZeit = 250;  // zu lang = Hand

bool letzterZustand = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);

  Serial.println("Spiel startet!");
}

void loop() {

  // Wenn gewonnen → stoppen
  if (tore >= MAXTORE) {
    Serial.println("🏆 GEWONNEN!");
    while (true) {
      // Spiel endet hier
    }
  }

  int wert = digitalRead(sensorPin);
  unsigned long jetzt = millis();

  // Start der Unterbrechung
  if (wert == LOW && letzterZustand == HIGH) {
    startZeit = jetzt;
    imEvent = true;
  }

  // Ende der Unterbrechung
  if (wert == HIGH && imEvent) {

    unsigned long dauer = jetzt - startZeit;

    // Nur gültiger Ball-Durchgang
    if (dauer >= minBlockZeit && dauer <= maxBlockZeit) {

      tore++;
      Serial.print("TOR! Stand: ");
      Serial.println(tore);
    }

    imEvent = false;
  }

  letzterZustand = wert;
}