//import libs
#include <Wire.h>
#include <Adafruit_Si4713.h>
#include <SPI.h>
#include <Wire.h>
#include "U8glib.h"
#include <SPI.h>
#include <MFRC522.h>

#define RESETPIN 3
int FMSTATION = 8750;      // 87.50 mHz
int minVal[] = {0,1000} ; //var for finding start channel with min interference
constexpr uint8_t RST_PIN = 9;     
constexpr uint8_t SS_PIN = 10;

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0); //create display screen object
Adafruit_Si4713 radio = Adafruit_Si4713(RESETPIN);  //create radio object
MFRC522 mfrc522(SS_PIN, RST_PIN);       //create RFID reader object

int broadcastToggle = 0;

void setup() {
  Serial.begin(9600);

  SPI.begin();        // Init SPI bus (RFID Scanner uses SPI to communicate)
  mfrc522.PCD_Init(); // Init MFRC522 card (RFID Scanner)
  mfrc522.PCD_SetAntennaGain(0xFF); //maximize gain on the RFID scanner antenna

  //button inputs
  pinMode(6, INPUT);
  pinMode(7, INPUT);

  //LED eyeball output
  pinMode(A1, OUTPUT);

  //if radio couldn't be initialized then 
  //flash eyeball LEDS to indicate a reboot is needed
  if (! radio.begin()) {  
    Serial.println("Couldn't find radio?");
    while(true){
      digitalWrite(A1,HIGH);
      delay(250);
      digitalWrite(A1,LOW);
      delay(250);
    }
  }

  //iterates through all radio freqs to find a starting one with minimal interference
  for (uint16_t f  = 8750; f<10800; f+=10) {
   radio.readTuneMeasure(f);
   radio.readTuneStatus();
   if (radio.currNoiseLevel < minVal[1]){
      minVal[1] = radio.currNoiseLevel;
      minVal[0] = f;
    }
   }
   
  FMSTATION = minVal[0];
  Serial.print("TUNING INTO CLEAREST DETECTED FREQUENCY:  ");
  Serial.println(FMSTATION);
  radio.setTXpower(0);  // dBuV, 88-115 max
  radio.tuneFM(FMSTATION); 

  // This will tell you the status in case you want to read it from the chip
  radio.readTuneStatus();

  // begin the RDS/RDBS transmission
  radio.beginRDS();
  radio.setRDSstation("AdaRadio");
  radio.setRDSbuffer( "Adafruit g0th Radio!");
}

//draw function for little screen display. 
//Writes the FM frequency being used to broadcast
void draw(void) {
  u8g.setFont(u8g_font_unifont);
  u8g.setScale2x2();
  u8g.setPrintPos(10, 20); 
  u8g.print(String(FMSTATION/100));
  u8g.print(".");
  u8g.print(String(FMSTATION%100));
}


void loop() {

  //tell readio to read new signal data
  radio.readASQ();

  //draw the output frequency on the screen
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );

  //tune down button
  if (digitalRead(6)){
    FMSTATION-=10;
    if (FMSTATION < 8750){FMSTATION=10800;} //wraps the frequency so it stays 87.5-108
    radio.tuneFM(FMSTATION);
  }

  //tune up button
  if (digitalRead(7)){
    FMSTATION+=10; 
    if (FMSTATION > 10800){FMSTATION=8750;} //wraps the frequency so it stays 87.5-108
    radio.tuneFM(FMSTATION);
  } 

  //if the RFID at the end of the wand is detected as present
  if ( mfrc522.PICC_IsNewCardPresent()){

     Serial.println(broadcastToggle);
     
    //toggle var
    if (broadcastToggle == 0){
      broadcastToggle = 1;  
    } else {
      broadcastToggle = 0;
    }

    //toggle LEDs on, turn transmitter on
    if (broadcastToggle){
      digitalWrite(A1,HIGH);
      radio.setTXpower(115); 
    }

    //toggle LEDs off, turn transmitter off
    if (!broadcastToggle){
      digitalWrite(A1,LOW);
      radio.setTXpower(0); 
    }
    
  }
  
}
