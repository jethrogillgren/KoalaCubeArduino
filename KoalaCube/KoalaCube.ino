

/*
 * --------------------------------------------------------------------------------------------------------------------
 * Controls local LED from server messages, and lets server know if it has found a Tag.
 * --------------------------------------------------------------------------------------------------------------------
 * 
 * This is the moving KoalaCube object code.
 * Mesh Network of XBees.  Messages are cubeId:char  with cubeId as the destination or the sender.
 * 
 * Pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 * 
 * TODO - KoalaCube full pins
 */

#include <SPI.h>
#include <MFRC522.h>

int cubeId = 0; //Manually set for each different cube build.

constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above

float sendHz = 1;

#define BLUE_PIN 3
#define GREEN_PIN 5
#define RED_PIN 6

String msgSeperator = ':'               //Message content is after this char.  Dest is before it.
#define MSG_SET_COLOUR_BLUE   'B'
#define MSG_SET_COLOUR_RED    'R'
#define MSG_SET_COLOUR_GREEN  'G'
#define MSG_SET_COLOUR_WHITE  'W'
#define MSG_SET_COLOUR_YELLOW 'Y'
#define MSG_SET_COLOUR_NONE   'N'



MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  
  //mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  //RGB LED
  pinMode(RED_PIN, OUTPUT); 
  pinMode(GREEN_PIN, OUTPUT); 
  pinMode(BLUE_PIN, OUTPUT); 
  SetColourGreen();

}

void loop() {

  // The loop constantly checks for new serial input:
  if ( Serial.available() )
  {
    // If new input is available on serial port
    parseSerialInput(Serial.read()); // parse it
  }
      
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Show some details of the PICC (that is: the tag/card)
    

    // Check for compatibility
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
        
    } else {
      //Serial.print(F("Card UID:"));
      send_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.println();
      
      delay(1000/sendHz);                       // wait for a second (makes L flash off)
    }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}



// Parse serial input, take action if it's a valid character
void parseSerialInput(char c)
{
  
  switch (c)
  {
  case cubeId+msgSeperator+MSG_SET_COLOUR_RED: 
    SetColourRed();
    break;

  case cubeId+msgSeperator+MSG_SET_COLOUR_BLUE: 
    SetColourBlue();
    break;

  case cubeId+msgSeperator+MSG_SET_COLOUR_WHITE: 
    SetColourWhite();
    break;

  case cubeId+msgSeperator+MSG_SET_COLOUR_GREEN: 
    SetColourGreen();
    break;

  case cubeId+msgSeperator+MSG_SET_COLOUR_YELLOW: 
    SetColourYellow();
    break;

  case cubeId+msgSeperator+MSG_SET_COLOUR_NONE:
    SetColourNone();
    break;

  default: // If an invalid character, do nothing
    break;
  }
}



/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void send_byte_array(byte *buffer, byte bufferSize) {

  for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
      Serial.print(buffer[i], HEX);
  }
    
}


void SetColourRed() {
  SetColour(255,0,0);
}
void SetColourGreen() {
  SetColour(0,255,0);
}
void SetColourYellow() {
  SetColour(255,255,0);
}
void SetColourWhite() {
  SetColour(255,255,255);
}
void SetColourBlue() {
  SetColour(0,0,255);
}
void SetColourNone() {
  SetColour(0,0,0);
}
void SetColour(int red, int green, int blue)
{
  #ifdef COMMON_ANODE
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  #endif
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);  
}
