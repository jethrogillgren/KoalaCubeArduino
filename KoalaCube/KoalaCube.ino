/*
 * --------------------------------------------------------------------------------------------------------------------
 * Controls local LED from server messages, and lets server know if it has found a Tag.
 * --------------------------------------------------------------------------------------------------------------------
 * 
 * This is the moving KoalaCube object code.
 * Mesh Network of XBees.  Messages are CUBE_ID:char  with CUBE_ID as the destination or the sender.
 */

#include <SPI.h>
#include <MFRC522.h>
#include <XBee.h>
#include <SoftwareSerial.h>
#include <Printers.h>
#include <elapsedMillis.h>

#include <Adafruit_NeoPixel.h>


//LEDS
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            3
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      64
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


//XBEE & COMMUNICATIONS
SoftwareSerial xbeeSerial(7, 6); // RX, TX

//Works with Series1 and 2
XBeeWithCallbacks xbee;

#define MSG_SET_COLOUR_BLUE   'B'
#define MSG_SET_COLOUR_RED    'R'
#define MSG_SET_COLOUR_GREEN  'G'
#define MSG_SET_COLOUR_WHITE  'W'
#define MSG_SET_COLOUR_YELLOW 'Y'
#define MSG_SET_COLOUR_NONE   'N'

// Build a reuseable message packet to send to the Co-Ordinator
XBeeAddress64 coordinatorAddr = XBeeAddress64(0x00000000, 0x00000000);

uint8_t placeMessagePayload[4] = {0};
ZBTxRequest placeMessage = ZBTxRequest(coordinatorAddr, placeMessagePayload, sizeof(placeMessagePayload));

//RFID
constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

elapsedMillis timeElapsed; //declare global if you don't want it reset every time loop runs
unsigned int sendInterval = 2000; // delay in milliseconds 



void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_SetAntennaGain( 0x07 << 4 );
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_SetAntennaGain( 0x07 << 4 );
  timeElapsed = sendInterval;
  //mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  //RGB LED
  pixels.begin(); // This initializes the NeoPixel library.
  SetColour(50,50,50);
  //SetColourGreen();

  // XBEE
  xbeeSerial.begin(9600);
  xbee.setSerial(xbeeSerial);

  // Make sure that any errors are logged to Serial. The address of
  // Serial is first cast to Print*, since that's what the callback
  // expects, and then to uintptr_t to fit it inside the data parameter.
  xbee.onPacketError(printErrorCb, (uintptr_t)(Print*)&Serial);
  xbee.onTxStatusResponse(printErrorCb, (uintptr_t)(Print*)&Serial);
  xbee.onZBTxStatusResponse(printErrorCb, (uintptr_t)(Print*)&Serial);

  // These are called when an actual packet received
  xbee.onZBRxResponse(zbReceive, (uintptr_t)(Print*)&Serial);

  // Print any unhandled response with proper formatting
  xbee.onOtherResponse(printResponseCb, (uintptr_t)(Print*)&Serial);

  // Enable this to print the raw bytes for _all_ responses before they
  // are handled
  xbee.onResponse(printRawResponseCb, (uintptr_t)(Print*)&Serial);

  Serial.println("Setup Completed OK");
}

void loop() {

  // Continuously let xbee read packets and call callbacks.
  xbee.loop();
     
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

//  Serial.println("A card is present");
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Could not read serial from card");
    return;
  }

  // Show some details of the PICC (that is: the tag/card)
    
  // Check for compatibility
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Bad Card - needs MIFARE Classic cards."));
    return;
      
  } else {
      SendPlacedPacket(mfrc522.uid.uidByte, mfrc522.uid.size);
  }
  
  // Dump debug info about the card; PICC_HaltA() is automatically called
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}





//// XBEE / COMMUNICATION FUNCTIONS
//FrameType:  0x90  recieved.
void zbReceive(ZBRxResponse& rx, uintptr_t data) {

  if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED
      || rx.getOption() == ZB_BROADCAST_PACKET ) {

      //Debug it out - copied from the lib
      Print *p = (Print*)data;
      if (!p) {
        Serial.println("ERROR 2");
        //flashSingleLed(LED_BUILTIN, 2, 500);
        return;
      }
      p->println(F("Recieved:"));
        p->print("  Payload: ");
        printHex(*p, rx.getFrameData() + rx.getDataOffset(), rx.getDataLength(), F(" "), F("\r\n    "), 8);
      p->println();
        p->print("  From: ");
        printHex(*p, rx.getRemoteAddress64() );
      p->println();

      //KoalaCubes only take 1 char commands
      printHex(rx.getData()[0], 2);
      parseCommand( (char) rx.getData()[0] );
      
      //flashSingleLed(LED_BUILTIN, 5, 50);
      
  } else {
      // we got it (obviously) but sender didn't get an ACK
      Serial.println("ERROR 1");
      //flashSingleLed(LED_BUILTIN, 1, 500);
  }
}
void SendPlacedPacket( byte *buffer, byte bufferSize )
{
  if (timeElapsed > sendInterval) 
  {

    //We assume 4 byte UIDs
    if( bufferSize != 4 )
    {
      Serial.print(F("ERROR 4 - Bad UUID Length: "));
      Serial.println(bufferSize);
      return;
    }
    
    Serial.println(F("SENDING UID:"));
    for ( uint8_t i = 0; i < 4; i++) {  //
      placeMessagePayload[i] = buffer[i];
      Serial.print(placeMessagePayload[i], HEX);
    }
    Serial.println();
  
    placeMessage.setFrameId(xbee.getNextFrameId());
    
    Serial.println("SENDING 'Placed' Message to Co-ordinator");
    //xbee.send(placeMessage);
    
    // Send the command and wait up to N ms for a response.  xbee loop continues during this time.
    uint8_t status = xbee.sendAndWait(placeMessage, 1000);
    if (status == 0)
    {
      Serial.println(F("SEND ACKNOWLEDGED"));
      timeElapsed = 0;       // reset the counter to 0 so the counting starts over...

    } else { //Complain, but do not reset timeElapsed - so that a new packet comes in and tried again immedietly.
      Serial.print(F("SEND FAILED: "));
      printHex(status, 2);
      Serial.println();
      //flashSingleLed(LED_BUILTIN, 3, 500);
    }
    
  }
}

// Parse serial input, take action if it's a valid character
void parseCommand( char cmd )
{
  Serial.print("Cmd:");
  Serial.println(cmd);
  switch (cmd)
  {
  case MSG_SET_COLOUR_RED: 
    SetColourRed();
    break;

  case MSG_SET_COLOUR_BLUE: 
    SetColourBlue();
    break;

  case MSG_SET_COLOUR_WHITE: 
    SetColourWhite();
    break;

  case MSG_SET_COLOUR_GREEN: 
    SetColourGreen();
    break;

  case MSG_SET_COLOUR_YELLOW: 
    SetColourYellow();
    break;

  case MSG_SET_COLOUR_NONE:
    SetColourNone();
    break;

  default: // If an invalid character, do nothing
    Serial.print("Unable to parse command: ");
    Serial.println(cmd);
    break;
  }
}






////LED FUNCTIONS 
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
  for(int i=0;i<NUMPIXELS;i++){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
      pixels.show(); // This sends the updated pixel color to the hardware.

  }

}

//Note, this blocks
void flashSingleLed(int pin, int times, int wait) {

  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(wait);
    digitalWrite(pin, LOW);

    if (i + 1 < times) {
      delay(wait);
    }
  }
}





// UTIL FUNCTIONS
void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}


