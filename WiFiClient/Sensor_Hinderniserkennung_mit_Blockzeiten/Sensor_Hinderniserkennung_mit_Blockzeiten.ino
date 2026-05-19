const int sensorPin = 4;

int tore = 0;

bool imEvent = false;
unsigned long startZeit = 0;

const unsigned long minBlockZeit = 80;   // echte Ballunterbrechung
const unsigned long maxBlockZeit = 1000; // Hand wird ignoriert

bool letzterZustand = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);
}

void loop() {

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

    // Nur gültiger "Ball-Durchgang"
    if (dauer >= minBlockZeit && dauer <= maxBlockZeit) {

      tore++;
      Serial.print("TOR! Stand: ");
      Serial.println(tore);
    }

    imEvent = false;
  }

  letzterZustand = wert;
}