#include <IRremote.h> //Bibilothek für Infrarot Fernbedienung
#include <Servo.h>    //Bibilothek für die Steuerung des Servo
#include <Stepper.h>  //Bibilothek für die Steuerung des Schrittmotor
#include <SPI.h>      //Bibilothek für RFID Kid
#include <MFRC522.h>  //Bibilothek für RFID Kid

//Definition der Pins
#define FotoSensor A0
#define Infrarot 2
#define trigger 3
#define echo 4
#define Bremslicht 9
#define BlinkLichtLinks 10
#define RuckfahrLicht 11
#define SteuerungsServo 12
#define Nachtlicht 22
#define Piezo 24
#define RST 26
#define BlinkLichtRechts 28
#define RGBb 30
#define RGBg 31
#define RGBr 32
#define SS 53


//Initalisieren
MFRC522 mfrc522(SS,RST);
Servo myServo;
Stepper myStepper(32,5,6,7,8);
Stepper myStepperR(32,5,7,6,8);
IRrecv irrec(Infrarot);
bool AN=false; //bool Variable zur Angabe, ob Motor an
bool Nachtl[]={false,false,false}; //bool Array zur Angebe ob Tag oder Nacht
long dauer=0, entfernung=0; //Variablen zur Abstandsberechnung
volatile int Richtung=0; //-1 Rückwärts; 0 Stop; 1 Vorwärts; 2 Links; 3 Rechts
volatile decode_results res; //volatile um in Interrupt zu benutzen
int TagNachtSchwellwert=450; //Wert gibt Schwellwert zwischen Tag und Nacht an


void setup() {

  //RGB auf Standart Aus setzen
  analogWrite(RGBb,0);
  analogWrite(RGBg,0);
  analogWrite(RGBr,204);

  //Init
  myServo.attach(SteuerungsServo);
  irrec.enableIRIn();
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  myStepper.setSpeed(500);
  myStepperR.setSpeed(500);

  //INPUT festlegen
  pinMode(echo, INPUT);
  pinMode(Infrarot,INPUT);
  pinMode(FotoSensor,INPUT);

  //OUTPUT fetslegen
  pinMode(Nachtlicht,OUTPUT);
  pinMode(BlinkLichtRechts,OUTPUT);
  pinMode(BlinkLichtLinks,OUTPUT);
  pinMode(RuckfahrLicht,OUTPUT);
  pinMode(Bremslicht,OUTPUT);
  pinMode(trigger, OUTPUT);
  pinMode(Piezo,OUTPUT);
  pinMode(RGBb,OUTPUT);
  pinMode(RGBg,OUTPUT);
  pinMode(RGBr,OUTPUT);

  //Interrupt an Port 2 hängen
  //wenn Wert an Port 2 sich verändert, Methode InfrarotAbfrage ausführen
  attachInterrupt(0, InfrarotAbfrage, CHANGE);
}



void loop() {
  if(AN) //Wenn An
    Bewegen(); //Bewegung ausführen
  if(AN) //Wenn An
    NachtUberprufung(); //Überprüfen des Lichtverhältnises
  RFID();//Überprüfung des RFID-Chips
  delay(1000);
}


//Methode zum Auslesen des Befehl der Infrarot Fernbedienung
void InfrarotAbfrage(){
  if(!AN) return; //Wenn aus returnen
    while(irrec.decode(&res)){ //Befehl entschlüsseln
      int help=res.value;
      Serial.println(res.value,DEC);
      switch(help){ //Abhängig vom Befehl
        case 16718055:Serial.println("Vorwärts");Richtung=1; break; //Taste 2 -> Vorwärts
        case 16730805:Serial.println("Rückwärts");Richtung=-1;analogWrite(RuckfahrLicht,255); break //Taste 8 -> Rückwärts und Rückwärtslicht an
        case 16726215:Serial.println("Stop");Richtung=0; analogWrite(Bremslicht,255); break; //Taste 5 -> stop und Bremslicht an
        case 16716015:Serial.println("Links");Richtung=2; break; //Taste 4 -> links
        case 16734885:Serial.println("Rechts");Richtung=3; break; //Taste 6 -> rechts
        case 16743045:Serial.println("Hupe"); digitalWrite(Piezo,HIGH); delay(1000); digitalWrite(Piezo,LOW); break; //Taste 3 -> Hupe
      }
    irrec.resume();} //Auf nächsten Befehl warten
  }


void Links_Blinken(){ //Methode zum links blinken
  for(int f=0;f<3;f++){
    analogWrite(BlinkLichtLinks,255);
    delay(300);
    digitalWrite(BlinkLichtLinks,LOW);
    delay(300);
    }
  }
  void Rechts_Blinken(){ //Methode zum rechts blinken
  for(int f=0;f<3;f++){
    analogWrite(BlinkLichtRechts,255);
    delay(300);
    digitalWrite(BlinkLichtRechts,LOW);
    delay(300);
    }
  }


void Bewegen(){ //MEthode, die die Bewegung ausführt
  switch(Richtung){
    case 1: //Vorwärts
      if(GetServoPosition()!=90) //Wenn Servo nicht auf Position 90°
        myServo.write(90); //Servo auf 90° stellen
      digitalWrite(RuckfahrLicht, LOW); //Rückfahrlicht aus, falls an
      if(Entfernung()>30){ //Wenn in 30cm kein Gegenstand im Weg
        myStepperR.step(-2048);myStepper.step(0);//fahren
        break;//Vorwärts
      }
      else{ //Wenn Gegenstand im Weg
        analogWrite(Bremslicht, 255); //Bremslicht an
        Richtung=0; //stoppen
        break;
      }
    case -1://Rückwärts
      myStepper.step(2048); myStepperR.step(0);//fahren
      break;//Rückwärts
    case 0:
     myStepper.step(0);myStepperR.step(0); //stoppen
     digitalWrite(RuckfahrLicht, LOW);  //Rückfahrlicht aus, falls an
     digitalWrite(Bremslicht, LOW); //Bremslicht aus, falls an
     break; //STOOOOOOOOOOOOP
    case 2:
      Links_Blinken(); //Links blinken
      if(GetServoPosition()!=60)//Wenn Servo nicht auf Porsition 60°
        myServo.write(60);//Servo auf 60° stellen
      myStepperR.step(-2048);//fahren
      break; //Links

    case 3:
      Rechts_Blinken();  //Rechts blinken
      if(GetServoPosition()!=120) //Wenn Servo nicht auf Position 120°
        myServo.write(120);//Servo auf 180° stellen
      myStepperR.step(-2048); //fahren
    break; //Rechts
  }
}

//Methode zum Einlesen der Position des Server
int GetServoPosition(){
  int  val = analogRead(SteuerungsServo); // StromWert auf Pin des Servo einlesen (Wert zwischen 0 und 1023)
  val = map(val, 0, 1023, 0, 180);     // in bekannten Wertebereich umsetzen (Wert zwischen 0 und 180)
  return val;
}

//Methode zum Einlesung der Entfernung am vorderen Automobil
int Entfernung(){
  //return 1000;
  digitalWrite(trigger, LOW); //Trigger aus, falls an
  delay(5);
  digitalWrite(trigger,HIGH); //Trigger an
  delay(10);
  digitalWrite(trigger, LOW); //Trigger aus
  dauer=pulseIn(echo, HIGH); //Dauer des Ultraschall-Echo einlesen
  entfernung=dauer/58; //in cm umrechnen
  if(entfernung>=500){Serial.println("E:OVER");return 1000;} //Wenn Wert zu groß, 1000 zurückgeben
  else if(entfernung<=0) {Serial.println("E:Under");return 0;} //Wenn Wert zu klein oder falsch, 0 zurückgeben
  else{Serial.println("E:"+entfernung);return entfernung;} //Wert zurückzugeben
}

//Methode, die ausliest ob Tag oder Nacht ist
void NachtUberprufung(){
  int temp=analogRead(FotoSensor); //Auslesen des Fotosensor
  Nachtl[2]=Nachtl[1]; //Verschieben der Daten des Arrays
  Nachtl[1]=Nachtl[0];
  Nachtl[0]=(temp>TagNachtSchwellwert); //Eingabe der neuen Daten (Tag=false, Nacht=true)
  if(Nachtl[0]&&Nachtl[1]&&Nachtl[2]){ //Wenn alle Daten true, ist Nacht ->Licht an
    analogWrite(Nachtlicht,255);
  }
  else if((!Nachtl[0])&&(!Nachtl[1])&&(!Nachtl[2])){ //Wenn alle Daten aus, ist Tag ->Licht aus
    digitalWrite(Nachtlicht,LOW);
  }
  if(temp>TagNachtSchwellwert){Serial.println("Nacht");}//Nacht - 512
  else{Serial.println("Tag");}//Tag
}

//Methode, die RFID ausliest
void RFID(){
  if (!mfrc522.PICC_IsNewCardPresent()){ //Abfangen, wenn keine Karte da
    Serial.println("Keine Karte");
    return; }
  if (!mfrc522.PICC_ReadCardSerial()){ //Abfangen, wenn Karte nichts sendet
    Serial.println("RFID Sender fehlt");
    return;}

  long code=0; //Speichern des RFID-Code
  for (byte i = 0; i < mfrc522.uid.size; i++)//Einlesen des Codes
  {
    code=((code+mfrc522.uid.uidByte[i])*10); //Berechnung des Codes
  }

  if(code==2352090){//Wenn Code gleich ist
    if(!AN){ //Wenn aus
      AN=true; //an machen
      analogWrite(RGBb,102); //RGB grün setzen
      analogWrite(RGBg,255);
      analogWrite(RGBr,51);
      }
    else if(Richtung==0){ //Wenn Stop
      AN=false; //aus machen
      analogWrite(RGBb,0); //RGB rot setzen
      analogWrite(RGBg,0);
      analogWrite(RGBr,204);
      AUS();
    }
  }
}

//MEthode die übrige Lichter ausmacht, wenn Motor aus
void AUS(){
  digitalWrite(Nachtlicht,LOW);
  digitalWrite(RuckfahrLicht,LOW);
}
