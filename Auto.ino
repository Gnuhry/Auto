#include <IRremote.h>

#include <Stepper.h>
#include <Irremote.h>

#define FotoSensor A0
#define Infrarot 2
#define trigger 3
#define echo 4
#define Bremslicht 9
#define BlinkLichtLinks 10
#define RuckfahrLicht 11


volatile int Richtung=0;
volatile decode_results res;
Stepper myStepper(32,5,6,7,8);
Stepper myStepperR(32,5,7,6,8);
long dauer=0, entfernung=0;
 IRrecv irrec(Infrarot);

void setup() {
  
irrec.enableIRIn();
   Serial.begin(9600);
pinMode(trigger, OUTPUT);
pinMode(echo, INPUT);
pinMode(Infrarot,INPUT);
 attachInterrupt(0, InfrarotAbfrage, CHANGE);
myStepper.setSpeed(500);
myStepperR.setSpeed(500);
}

void loop() {
Bewegen();
NachtUberprufung();
delay(1000);
}

void InfrarotAbfrage(){
   while(irrec.decode(&res)){
   int help=res.value;
    Serial.println(res.value,DEC);
    switch(help){
      case 16718055:Serial.println("Vorwärts");Richtung=1; break;
      case 16730805:Serial.println("Rückwärts");Richtung=-1;analogWrite(RuckfahrLicht,255); break;
      case 16726215:Serial.println("Stop");Richtung=0; analogWrite(Bremslicht,255); break;
      case 16716015:Serial.println("Links");Richtung=2; break;
      case 16734885:Serial.println("Rechts");Richtung=0; break;
  }
    irrec.resume();}
  }
void Links_Blinken(){
  for(int f=0;f<3;f++){
    analogWrite(BlinkLichtLinks,255);
    delay(1000);
    digitalWrite(BlinkLichtLinks,LOW);
    delay(1000);
    }
  }


void Bewegen(){
  switch(Richtung){
    case 1: 
     digitalWrite(RuckfahrLicht, LOW);
    if(Entfernung()>30){
    myStepperR.step(0);myStepper.step(2048); break;//Vorwärts
    }else{
    analogWrite(Bremslicht, 255);
    Richtung=0;
    break;}
    case -1:myStepper.step(0); myStepperR.step(-2048); break;//Rückwärts
    case 0: myStepper.step(0);myStepperR.step(0); digitalWrite(RuckfahrLicht, LOW); digitalWrite(Bremslicht, LOW);break; //STOOOOOOOOOOOOP
    case 2: Links_Blinken(); //Links
    }
  }

  
int Entfernung(){
digitalWrite(trigger, LOW);
delay(5);
digitalWrite(trigger,HIGH);
delay(10);
digitalWrite(trigger, LOW);
dauer=pulseIn(echo, HIGH);
entfernung=dauer/58;
if(entfernung>=500){Serial.println("E:OVER");return 1000;}
else if(entfernung<=0) {Serial.println("E:Under");return 0;}
else{Serial.println("E:"+entfernung);return entfernung;}
  }
  void NachtUberprufung(){
    int temp=analogRead(FotoSensor);
    if(temp>550){Serial.println("Nacht");}//NAcht - 512
    else{Serial.println("Tag");}//Tag
    }
