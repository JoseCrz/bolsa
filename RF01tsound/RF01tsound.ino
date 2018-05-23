//unidad pantalla
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
int addr = 0;
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define MainPeriod 1000
#define Watsdog 4500
#define XPOS 0
#define YPOS 1
#define UsbDebug Serial 
#define RFHC11 Serial1
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
#define speakerOut 8 // Set up speaker on a PWM pin (digital 8... )
// Do we want debugging on serial out? 1 for yes, 0 for no
int DEBUG = 0;

#define ButtonEnable A2// Set up sicro button V source on  pin A2 (Analog 2)
#define Button 7// Set up sicro button sens on  pin 7 (Analog 7)
#define BtnDelete 6
#define Motor 5// Set up Motor on  pin 5 (digital 5)
#define SSD1306_LCDHEIGHT 64  

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
#define RFctrl A3
int extract,RXLED = 17;
boolean u2sync = 0, u1sync = 0;
String ID = "rf00";
void setup() {
  pinMode(RFctrl, OUTPUT);
  digitalWrite(RFctrl, HIGH);
  
  pinMode(speakerOut, OUTPUT);
  pinMode(Button, INPUT);
  pinMode(BtnDelete, INPUT);
  pinMode(ButtonEnable, OUTPUT);
  digitalWrite(ButtonEnable,HIGH);
  pinMode(Motor, OUTPUT);
  pinMode(RXLED, OUTPUT);  // Set RX LED as an output
  UsbDebug.begin(9600); // Set serial out if we want debugging
  RFHC11.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  // init done
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("012|OK");
  display.println("013|Error");
  display.println("016|Error");
  display.setTextSize(2);
  display.println(" ID|ESTADO");
  //display.setCursor(0,14);
  
  //display.drawLine(0, 15, display.width()-1, 14, WHITE);
  display.display();
  //delay(2000);
  display.clearDisplay();
  extract = EEPROM.read(2); //por ser id2
  if (extract == 1) {
	  u2sync = 1;
  }
  else {
	  u2sync = 0;
  }
  int extract = EEPROM.read(1); //por ser id2
  if (extract == 1) {
	  u1sync = 1;
  }
  else {
	  u1sync = 0;
  }

  //logo();
}

// MELODY and TIMING  =======================================
//  melody[] is an array of notes, accompanied by beats[],
//  which sets each note's relative length (higher #, longer note)
int melody[] = {  C,  b,  g,  C,  b,   e,  R,  C,  c,  g, a, C };
int beats[]  = { 16, 16, 16,  8,  8,  16, 32, 16, 16, 16, 8, 8 };
int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.
// Set overall tempo
long tempo = 10000;
// Set length of pause between notes
int pause = 500;
// Loop variable to increase Rest length
int rest_count = 50; //<-BLETCHEROUS HACK; See NOTES
long previousMillis = 0; // will store last time of the cycle end
long previousMillis2 = 0; // will store last time of the cycle end
long previousMillis3 = 0; // will store last time of the cycle end
// Initialize core variables
int tone_ = 0;
int idtosync,beat = 0;
long duration  = 0;
String valor,inString = "";
boolean alert,remote1 = 0, remote2 = 0,borrar,motorstat,emparejar,stringcomplete = 0;
byte value;

void meloda(void);
void playtone(void);
void motorvib(int temp);
void parser(String received );
void Alarma(void);
void pantalla(void);
// PLAY TONE  ==============================================
// Pulse the speaker to play a tone for a particular duration

// ============================= MAIN =============================
void loop() {
	
	timmer1();
  if(u1sync){ timmer2();}
  if (u2sync) { timmer3();}
  if (alert) { Alarma(); 
				motorvib(200);
			}

  if(!digitalRead(Button)){
	  if (alert) {
		  alert = 0;
		  delay(500);
	  }
	  else {
		  //display.clearDisplay();
		  display.setCursor(5, 45);
		  display.println("EMPAREJA");
		  display.display();
		  display.clearDisplay();
		  emparejar = 1;
		  meloda();
	  }
  }
  
  if(!digitalRead(BtnDelete)){
	  //display.clearDisplay();
	  display.setCursor(5, 45);
	  display.println("BORRA ID");
	  display.display();
	  display.clearDisplay();
	  borrar = 1;
	  meloda();
  }
	while(RFHC11.available() > 0){ // Si hay datos por modulo RF   
		char inChar = RFHC11.read();
		inString += (char)inChar;
		//UsbDebug.println(inChar);
		if (inChar == '\n') {
			UsbDebug.println(inString);
			stringcomplete=1;
			while(RFHC11.read() > -1);
		}
	}
	if (stringcomplete){
		parser(inString);
		stringcomplete=0;
		inString ="";    
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
      digitalWrite(speakerOut, LOW);
      delayMicroseconds(tone_ / 2);
      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { 
      delayMicroseconds(duration);  
    }                                
  }                                
}
void meloda(){
  // contador para  melody[] and beats[]
  for (int i=0; i<MAX_COUNT; i++) {
    tone_ = melody[i];
    beat = beats[i];
    duration = beat * tempo; // tiempos
    playTone();
    // pausa entre notas...
    delayMicroseconds(pause);

    if (DEBUG) { 
      UsbDebug.print(i);
      UsbDebug.print(":");
      UsbDebug.print(beat);
      UsbDebug.print(" ");    
      UsbDebug.print(tone_);
      UsbDebug.print(" ");
      UsbDebug.println(duration);
    }
  }
}
void timmer1() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= MainPeriod) { //Uptade every X miliseconds, this will be equal to reading frecuency (Hz).
    previousMillis = currentMillis;     
	pantalla();
    // read the input on analog pin 0:
    int sensorValue = analogRead(A0);
    if(sensorValue <= 500){     //nivel de bateria , 4.5v aproximados.
      digitalWrite(RXLED, LOW);   // set the RX LED ON (LED SIGNAL INVERTED)
        UsbDebug.print("Low Batt: ");
      }else{
        digitalWrite(RXLED, HIGH);   // set the RX LED off (LED SIGNAL INVERTED)
      }
        UsbDebug.print("Batt Lvl: ");
		UsbDebug.println(sensorValue);
        //UsbDebug.println(sensorValue);    
    }
}
void parser( String received ){
  if(received.startsWith("s"))
   {
    UsbDebug.println("pARSING..");
    int pos=received.indexOf("f");  // CADENA ES syncIDrf01 o syncIDrf02 Devuleve pos de 'f' 
    //Serial.println(pos);
    idtosync = received.substring(pos+1).toInt();
	UsbDebug.print("ID ");
	UsbDebug.println(idtosync);
	//Funcion de emparejamiento
	if(emparejar){
		addr = idtosync;
		int extract= EEPROM.read(addr);
		if (extract > -1) { //si No se ha emparejado ese No 'addr' de remoto 
			int data = 1; //bit de emparejado 2^0
			EEPROM.update(addr, data);
			delay(100);
			UsbDebug.println("Emparejado");
			RFHC11.println("sync");
			display.setCursor(0, 0);
			display.println("EMPAREJADO");
			display.print("ID: ");
			display.println(addr);
			display.display();
			display.clearDisplay();
			if (addr == 1) { u1sync = 1; }
			if (addr == 2) { u2sync = 1; }
		}
      value = EEPROM.read(addr); //si no existe escribe el valor en la direccion 
	  emparejar=0;
	  UsbDebug.println("---EEPROM---");
	  for (int i = 0; i <= 4; i++) {
		  extract = EEPROM.read(i);
		  UsbDebug.print(i);
		  UsbDebug.print(": ");
		  UsbDebug.println(extract);
	  }
	  UsbDebug.println("-----------");
     }//emparejar
	UsbDebug.print("Borrarval");
	UsbDebug.println(borrar);
	if (borrar) {
		addr = idtosync;
		int extract = EEPROM.read(addr);
		if (extract > -1) { //si esta emparejado 
			int data = 255; //bit de emparejado -1
			EEPROM.update(addr, data);
			delay(100);
			UsbDebug.println("Borrado");
			RFHC11.println("borrado");
			display.setCursor(0, 0);
			display.println("BORRADO");
			display.print("ID: ");
			display.println(addr);
			display.display();
			display.clearDisplay();
			if (addr == 1) { u1sync = 0; }
			if (addr == 2) { u2sync = 0; }
		}
		value = EEPROM.read(addr); //si no existe escribe el valor en la direccion 
		borrar = 0;
		UsbDebug.println("---EEPROM---");
		for (int i = 0; i <= 4; i++) {
			extract = EEPROM.read(i);
			UsbDebug.print(i);
			UsbDebug.print(": ");
			UsbDebug.println(extract);
		}
		UsbDebug.println("-----------");
	}
   }
	  if (received.startsWith("I")) {
		  int pos = received.indexOf("f");  // CADENA ES syncIDrf01 o syncIDrf02 Devuleve pos de 'f' 
											//Serial.println(pos);
		  idtosync = received.substring(pos + 1).toInt();
		  if (idtosync==1){
			  previousMillis2 = millis();
			  UsbDebug.println("reinicio U1");
			  RFHC11.println("ping");
			  
		  }
		  if (idtosync==2) {
			  previousMillis3 = millis();
			  UsbDebug.println("reinicio U2");
			  RFHC11.println("ping");
			  
		  }
	  }
  }

void timmer2() {
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis2 >= Watsdog) { //Uptade every X miliseconds, this will be equal to reading frecuency (Hz).
		
		UsbDebug.println("Tiempo vencido UNIDAD 1");		
		alert = 1;
		previousMillis2 = currentMillis;
	}
}
void timmer3() {
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis3 >= Watsdog) { //Uptade every X miliseconds, this will be equal to reading frecuency (Hz).
		
		UsbDebug.println("Tiempo vencido UNIDAD 2");
		alert = 1;
		previousMillis3 = currentMillis;
	}
}

void Alarma() {
	//while (digitalRead(Button)) {
		meloda();
	//}
}

void pantalla() {
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	if (u1sync && !alert) { display.println("001| OK"); }
	else if(u1sync && alert) {
		display.println("001|ERROR");
	}
	if (u2sync && !alert) { display.println("002| OK"); }
	else if(u2sync && alert){
		display.println("002|ERROR");
	}
	display.println(" ID|ESTADO");
	display.display();
	display.clearDisplay();
}

//CÓDIGO AGREGADO POR JOSÉ CUEVAS PARA LA INTERFAZ GRÁFICA:

void logo (void) {

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(50,8);
  display.clearDisplay();
  display.println("iLab");
  display.display();
  delay(250);
 
  display.startscrollleft(0x00, 0x0F);
  delay(1500);
  display.stopscroll();

}




