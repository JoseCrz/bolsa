//unidad remota
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
// TONES  ==========================================
// Start by defining the relationship between
//       note, period, &  frequency.
#define  c     3830    // 261 Hz
#define  d     3400    // 294 Hz
#define  e     3038    // 329 Hz
#define  f     2864    // 349 Hz
#define  g     2550    // 392 Hz
#define  a     2272    // 440 Hz
#define  b     2028    // 493 Hz
#define  C     1912    // 523 Hz
// Define a special note, 'R', to represent a rest
#define  R     0
// SETUP ============================================
#define MainPeriod 2000
#define MainPeriod2 2000
#define Watsdog 4500

#define speakerOut 3 // Set up speaker on a PWM pin (digital 3... )
int DEBUG = 1; // Do we want debugging on serial out? 1 for yes, 0 for no
#define ButtonEnable A2 // Set up sicro button V source on  pin A2 (Analog 2)
#define Button 7 // Set up sicro button sens on  pin 7 (Analog 7)
#define Motor 5 // Set up Motor on  pin 5 (digital 5)
#define UsbDebug Serial 
#define RFHC11 Serial1
#define RFctrl A3
int RXLED = 17;
String ID = "rf02";
boolean emparejado = 0;
void setup() {
	pinMode(RFctrl, OUTPUT);
	digitalWrite(RFctrl, HIGH);
	pinMode(speakerOut, OUTPUT);
	pinMode(Button, INPUT);
	pinMode(ButtonEnable, OUTPUT);
	digitalWrite(ButtonEnable, HIGH);
	pinMode(RXLED, OUTPUT);  // Set RX LED as an output
	pinMode(Motor, OUTPUT);
	if (DEBUG) { UsbDebug.begin(9600); }
	RFHC11.begin(9600);
	int extract = EEPROM.read(0); //por ser id2
	if (extract == 1) {
		emparejado = 1;
	}	else {
		emparejado = 0;
	}
}

// MELODY and TIMING  =======================================
//  melody[] is an array of notes, accompanied by beats[],
//  which sets each note's relative length (higher #, longer note)
int melody[] = {  C,  b,  g,  C,  b,   e,  R,  C,  c,  g, a, C };
int beats[]  = { 16, 16, 16,  8,  8,  16, 32, 16, 16, 16, 8, 8 };
int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.
bool motorstat;
// Set overall tempo
long tempo = 10000;
// Set length of pause between notes
int pause = 500;
// Loop variable to increase Rest length
int rest_count = 50; //<-BLETCHEROUS HACK; See NOTES


// Initialize core variables
long previousMillis = 0; // will store last time of the cycle end
long previousMillis2 = 0; // will store last time of the cycle end
long previousMillis3 = 0; // will store last time of the cycle end
long previousMillis4 = 0; // will store last time of the cycle end
String valor, inString = "";
boolean alert,m2sync=0,borrar = 0, stringcomplete = 0;
int tone_ = 0;
int beat = 0;
long duration  = 0;
void meloda(void);
void playtone(void);
void motorvib(int temp);
void parser(String received);

// ============================= MAIN =============================
void loop() {
  timmer1();
  timmer2();
  if (emparejado) { timmer3(); }
  if (alert) { Alarma(); }
  if(!digitalRead(Button)){
	  if (alert) {
		  alert = 0;
		  delay(500);
	  }
	  else {
		  //motorvib(350);
		  meloda();
		  UsbDebug.println("Transmitiendo...");
		  RFHC11.print("syncID");
		  RFHC11.println(ID);
	  }
  
  }

  
  while(RFHC11.available() > 0){ // Si hay datos por modulo RF   
    char inChar = RFHC11.read();
	//UsbDebug.println(inChar);
    inString += (char)inChar;
    if (inChar == '\n') {
      UsbDebug.println(inString);
      // clear the string for new input:
	  stringcomplete = 1;
	  while (RFHC11.read() > -1);
    }
  }
  if (stringcomplete) {	
	  parser(inString);
	  stringcomplete = 0;
	  inString = "";
  }
}
// ============================= end MAIN =============================
void motorvib(int temp){
  digitalWrite(Motor,HIGH);
  delay(temp);
  digitalWrite(Motor,LOW);
  
  delay(200);
  digitalWrite(Motor,HIGH);
  delay(temp);
  digitalWrite(Motor,LOW);
  }

void playTone() {
  long elapsed_time = 0;
  if (tone_ > 0) { // if this isn't a Rest beat, while the tone has
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {
      digitalWrite(speakerOut,HIGH);
      delayMicroseconds(tone_ / 2);
      // DOWN
      digitalWrite(speakerOut, LOW);
      delayMicroseconds(tone_ / 2);
      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);  
      
    }                                
  }                                
}

void meloda(){
  // Set up a counter to pull from melody[] and beats[]
  for (int i=0; i<MAX_COUNT; i++) {
    tone_ = melody[i];
    beat = beats[i];
    duration = beat * tempo; // Set up timing
    playTone();
    // A pause between notes...
    delayMicroseconds(pause);
  }
}

void timmer1() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= MainPeriod) { //Uptade every X miliseconds, this will be equal to reading frecuency (Hz).
    previousMillis = currentMillis;
    int sensorValue = analogRead(A0);// read the input on analog pin 0: estatos de bateria
    if(sensorValue <= 500){     
        digitalWrite(RXLED, LOW);   // set the RX LED ON (LED SIGNAL INVERTED)
        UsbDebug.print("Low Batt: ");
      }else{
        digitalWrite(RXLED, HIGH);   // set the RX LED off (LED SIGNAL INVERTED)
      }
    UsbDebug.print("Batt Lvl: ");UsbDebug.println(sensorValue);  
  }//end funcion1    
}
void timmer2() {
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis2 >= MainPeriod2) { //Uptade every X miliseconds, this will be equal to reading frecuency (Hz).
		previousMillis2 = currentMillis;
		UsbDebug.print("val emparejado...");
		UsbDebug.println(emparejado);
		if (emparejado) {
			UsbDebug.println("Transmitiendo...");
			RFHC11.print("ID");
			RFHC11.println(ID);
		}
	
	}
}

void parser(String received) {
	if (received.startsWith("s"))
	{
		emparejado = 1;
		UsbDebug.println("Emparejado");
		EEPROM.update(0, 1);
	}
	if (received.startsWith("b"))
	{
		emparejado = 0;
		UsbDebug.println("Desemparejado");
		EEPROM.update(0, 0);
	}
	if (received.startsWith("p")) {
		previousMillis3 = millis();
		UsbDebug.println("reinicio Base ping");
	}
}


void timmer3() {
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis3 >= Watsdog) { //Uptade every X miliseconds, this will be equal to reading frecuency (Hz).
		previousMillis3 = currentMillis;
		UsbDebug.println("Tiempo vencido Base");
		alert = 1;
		previousMillis3 = millis();
	}
}


void Alarma() {
	//while (digitalRead(Button)) {
		meloda();
	//}
	
}