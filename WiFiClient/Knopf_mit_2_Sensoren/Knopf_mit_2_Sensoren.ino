struct TeamSensor {

  int pin;

  int tore = 0;

  bool imEvent = false;
  bool letzterZustand = HIGH;

  unsigned long startZeit = 0;
};



TeamSensor gelb = {4};
TeamSensor rot  = {5};

const int startButton = 2;



const int MAXTORE = 6;

const unsigned long minBlockZeit = 80;
const unsigned long maxBlockZeit = 700;


const unsigned long torPause = 5000;

unsigned long letztesTorGlobal = 0;



enum SpielStatus {

  WARTET_AUF_START,
  SPIELT
};

SpielStatus status = WARTET_AUF_START;



void resetSpiel() {

  gelb.tore = 0;
  rot.tore = 0;

  gelb.imEvent = false;
  rot.imEvent = false;

  letztesTorGlobal = 0;

  Serial.println("Neues Spiel gestartet!");
}



void pruefeSensor(TeamSensor &team,
                  const char* name) {

  unsigned long jetzt = millis();

  int wert = digitalRead(team.pin);


  if (wert == LOW &&
      team.letzterZustand == HIGH) {

    team.startZeit = jetzt;
    team.imEvent = true;
  }


  if (team.imEvent &&
      jetzt - team.startZeit > maxBlockZeit) {

    team.imEvent = false;
  }


  if (wert == HIGH &&
      team.letzterZustand == LOW &&
      team.imEvent) {

    unsigned long dauer =
      jetzt - team.startZeit;

    
    if (dauer >= minBlockZeit &&
        dauer <= maxBlockZeit &&
        jetzt - letztesTorGlobal >= torPause) {

      team.tore++;
      letztesTorGlobal = jetzt;

      Serial.print("TOR ");
      Serial.print(name);

      Serial.print(" | Gelb: ");
      Serial.print(gelb.tore);

      Serial.print(" Rot: ");
      Serial.println(rot.tore);

    
      if (team.tore >= MAXTORE) {

        Serial.print(name);
        Serial.println(" GEWINNT!");

        status = WARTET_AUF_START;

        Serial.println("Startknopf drücken für neues Spiel.");
      }
    }

    team.imEvent = false;
  }

  team.letzterZustand = wert;
}



void setup() {

  Serial.begin(115200);

  pinMode(gelb.pin, INPUT_PULLUP);
  pinMode(rot.pin, INPUT_PULLUP);

  pinMode(startButton, INPUT_PULLUP);

  Serial.println("Startknopf drücken...");
}



void loop() {



  if (status == WARTET_AUF_START) {

    if (digitalRead(startButton) == LOW) {

      delay(50);

      if (digitalRead(startButton) == LOW) {

        resetSpiel();
        status = SPIELT;

        Serial.println("SPIEL STARTET!");
      }
    }

    return;
  }



  pruefeSensor(gelb, "GELB");
  pruefeSensor(rot, "ROT");
}