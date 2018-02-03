

/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read data from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 * 
 * If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
 * So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
 * details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
 * keep the PICCs at reading distance until complete.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
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
 */

#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above

#define BLUE_PIN 3
#define GREEN_PIN 5
#define RED_PIN 6

#define MSG_SET_COLOUR_BLUE 'B'
#define MSG_SET_COLOUR_RED 'R'
#define MSG_SET_COLOUR_GREEN 'G'
#define MSG_SET_COLOUR_WHITE 'W'
#define MSG_SET_COLOUR_YELLOW 'Y'


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

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
      Serial.print(F("Card UID:"));
      send_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.println();
      
      delay(1000);                       // wait for a second
    }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}



// Parse serial input, take action if it's a valid character
void parseSerialInput(char c)
{
  
  switch (c)
  {
  case MSG_SET_COLOUR_RED: // Send a frame out next. 
    SetColourRed();
    break;

  case MSG_SET_COLOUR_BLUE: // Send a frame out next. 
    SetColourBlue();
    break;

  case MSG_SET_COLOUR_WHITE: // Send a frame out next. 
    SetColourWhite();
    break;

  case MSG_SET_COLOUR_GREEN: // Send a frame out next. 
    SetColourGreen();
    break;

  case MSG_SET_COLOUR_YELLOW: // Send a frame out next. 
    SetColourYellow();
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