const int sensorPin = 4;

int tore = 0;
bool ausgelöst = false;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);
}

void loop() {

  int wert = digitalRead(sensorPin);

  // TOR erkannt (bei LOW)
  if (wert == LOW && !ausgelöst) {
    tore++;
    Serial.print("TOR! Stand: ");
    Serial.println(tore);

    ausgelöst = true;
  }

  // Reset wenn wieder frei
  if (wert == HIGH) {
    ausgelöst = false;
  }

  delay(100);
}