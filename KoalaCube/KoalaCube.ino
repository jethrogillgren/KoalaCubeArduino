/*
 * --------------------------------------------------------------------------------------------------------------------
 * Controls local LED from server messages, and lets server know if it has found a RFID Tag.
 * --------------------------------------------------------------------------------------------------------------------
 * 
 * This is the moving KoalaCube object code.
 */

#include <SPI.h>
#include <MFRC522.h>
#include <XBee.h>
#include <SoftwareSerial.h>
#include <Printers.h>
#include <elapsedMillis.h>

#include <Adafruit_NeoPixel.h> //1.1.6 by adafruit

//LEDS
bool PINK_MOD = false;
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            3
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      64


float lastFlickerBrightness = 0;
float currentFlickerBrightness = 0;
int flickerDelay = 0;
int lastFlickerDelay = 0;
bool flickerHeadingUp = false;

//Default max brightness (0-255)
#define DEFAULT_BRIGHTNESS 50
//Number of pixels to miss to save power.
#define PIXEL_STEP 3
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


//XBEE & COMMUNICATIONS
SoftwareSerial xbeeSerial(7, 6); // RX, TX

//Works with Series1 and 2
XBeeWithCallbacks xbee;

#define MSG_RFID_REMOVED 'z'

#define MSG_SET_COLOUR_BLUE   'B'
#define MSG_SET_COLOUR_RED    'R'
#define MSG_SET_COLOUR_GREEN  'G'
#define MSG_SET_COLOUR_WHITE  'W'
#define MSG_SET_COLOUR_YELLOW 'Y'
#define MSG_SET_COLOUR_NONE   'N'

unsigned int flickerState = 2; // 0:low power  1:medium power  2:full power

#define MSG_SET_FLICKER_0 '0'
#define MSG_SET_FLICKER_1 '1'
#define MSG_SET_FLICKER_2 '2'

#define MSG_SETUP_0 'a'
#define MSG_SETUP_1 'b'
#define MSG_SETUP_2 'c'
#define MSG_SETUP_3 'd'
//#define MSG_SETUP_4 'e'

// Build a reuseable message packet to send to the Co-Ordinator
XBeeAddress64 coordinatorAddr = XBeeAddress64(0x00000000, 0x00000000);

uint8_t placeMessagePayload[4] = {0};
ZBTxRequest placeMessage = ZBTxRequest(coordinatorAddr, placeMessagePayload, sizeof(placeMessagePayload));
uint8_t removedMessagePayload[1] = {0};
ZBTxRequest removedMessage = ZBTxRequest(coordinatorAddr, removedMessagePayload, sizeof(removedMessagePayload));

//RFID
constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

elapsedMillis timeElapsed; //declare global if you don't want it reset every time loop runs
unsigned int sendInterval = 10000; // delay in milliseconds 

bool isPlaced = false;
int isPlacedAttempts = 0;
bool previous;
bool current;

int errorCnt = 0;
    
void setup() {
  Serial.begin(9600);   // Initialize serial communications with the PC
  SPI.begin();      // Init SPI bus

  //RFID Reader
  InitRFIDReader(); //Will continiously retry (blocking) if init fails.

  //RGB LED
  pixels.begin(); // This initializes the NeoPixel library.
  SetColourWhite();
  delay(1000);
  SetColourNone();
  
  //LightLEDPosition(27,  DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS, true);
  /*delay(100);
  LightNLEDs(1);
  delay(1000);
  LightNLEDs(2);
  delay(1000);
  LightNLEDs(3);
delay(1000);
  LightNLEDs(4);
  delay(1000);
  LightNLEDs(5);
  delay(1000);
  SetColourNone();*/

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
  //xbee.onResponse(printRawResponseCb, (uintptr_t)(Print*)&Serial);

  if(PINK_MOD)
    Serial.println("Setup (PINK MOD) Completed OK");
  else
    Serial.println("Setup Completed OK");
}

void loop() {

  //Serial.println("Check XBee");

  
  //XBEE
  // Continuously let xbee read packets and call callbacks.
  xbee.loop();


  //LEDS flickering
  //0:low flickering power   1:medium flickering power   2:full stable power
  if( flickerState < 2 )
  {
    //Serial.println("flicker");
    //Count down until we start a new flash.
    flickerDelay--;
    
    //Time to start a new flash
    if(flickerDelay <= 0)
    { 
      //We flash at semi-random intervals
      if(lastFlickerDelay < 10)
        flickerHeadingUp = true;
      else if (lastFlickerDelay > 40)
        flickerHeadingUp = false;

      if(flickerHeadingUp)
        flickerDelay = random(lastFlickerDelay, 50);
      else
        flickerDelay = random(0, lastFlickerDelay);

      lastFlickerDelay = flickerDelay;
        
      if(flickerDelay == 0)
        lastFlickerBrightness = random(0, DEFAULT_BRIGHTNESS/3);
      else if(flickerState == 1);
        lastFlickerBrightness = random(0, DEFAULT_BRIGHTNESS);
        
      SetColourWhite( lastFlickerBrightness );
      currentFlickerBrightness = lastFlickerBrightness;
    }
    else if( currentFlickerBrightness > 1 ) //Process the current flash.
    {
      if(flickerState==0)
        currentFlickerBrightness=currentFlickerBrightness*0.5;
      else
        currentFlickerBrightness=currentFlickerBrightness*0.8;
        
      SetColourWhite( currentFlickerBrightness );
    }
    //else we are in a pause.

    if(flickerState==0)
      delay(30);
    else
      delay(5);
  }

  
  //// RFID
  if(isPlaced)
  {
    //Serial.println("check for remove");
    //Poll the card twice because when placed it toggles
    //betwen 1 and 0, and sticks to 0 only when removed.
    previous = !mfrc522.PICC_IsNewCardPresent();
    current =!mfrc522.PICC_IsNewCardPresent();
    
    if(current && previous)
      isPlacedAttempts++; //We think we were 'removed'
//    else //We toggled, indicating we are placed
//      delay(200); //Throttle the next check to save power

    //Check we're sure we were removed.
    if(isPlacedAttempts >3 )
    {
      Serial.println("Removed from Pos");
      isPlaced = false;
      SendRemovedPacket();
      //LightLEDPosition(28,  0,0,100, false); //BLue IDLE indicator
    }
    else if (timeElapsed > sendInterval) 
    {
      SendCurrentPlacedPacket(); //Includes resetting timeElapsed.
      //LightLEDPosition(28,  0,100,0, false); //Green Placed indicator
    }
  }
  else //Search for any new card
  {
    //Serial.println("check for place");

    //Replicate mfrc522.PICC_IsNewCardPresent() and PICC_ReadCardSerial() but with extra debug

    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
  
    // Reset baud rates
    mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
    mfrc522.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
    // Reset ModWidthReg
    mfrc522.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);

    //Check for card.
    MFRC522::StatusCode reqAResult = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);

    if(reqAResult == MFRC522::STATUS_OK)
    {
      //Serial.println("RequestA got STATUS_OK");
      
      MFRC522::StatusCode selectResult = mfrc522.PICC_Select(&mfrc522.uid);
      if (selectResult == MFRC522::STATUS_OK)
      {
        // Check for compatibility
        MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
        if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K)
        {
          Serial.println(F("Bad Card - needs MIFARE Classic cards."));
          //LightLEDPosition(28,  100,0,0, false); //Red error indicator
          CheckResetRFID();
          return;
        }
        else //We read OK!
        {
          Serial.println("Placed");
          isPlaced = true;
          isPlacedAttempts = 0;
          errorCnt = 0;
          //LightLEDPosition(28,  0,100,0, false); //Green OK Led.
    
          
          //Check for RESET RFID
          if(mfrc522.uid.size == 4 && mfrc522.uid.uidByte[0] == 0x46 && mfrc522.uid.uidByte[1] == 0x7E
              && mfrc522.uid.uidByte[2] == 0xC6 && mfrc522.uid.uidByte[3] == 0x96)
          {
            Serial.println("RESET RFID Activated");
            delay(100);
            software_Reset();
          }
          else if (mfrc522.uid.size == 4 && mfrc522.uid.uidByte[0] == 0xA6 && mfrc522.uid.uidByte[1] == 0x9C
              && mfrc522.uid.uidByte[2] == 0x1F && mfrc522.uid.uidByte[3] == 0xB9)
          {
            Serial.println("LED TEST Activated");
            delay(100);
            SetColour(255,255,255,false);
            delay(2000);
            SetColourNone();
          }
          else  //Normal RFID, let's send it
          {
            SendPlacedPacket(mfrc522.uid.uidByte, mfrc522.uid.size);
            //Resends are handled in the 'checking for removal' loop.
          }
        }
      }
      else //Failed to select card data.
      {
        Serial.print("PICC_Select failed with:");
        Serial.println( mfrc522.GetStatusCodeName(selectResult) );
        //LightLEDPosition(28,  100,0,0, false); //Red warning LED

        CheckResetRFID();
      }
    
    }
    else if(reqAResult != MFRC522::STATUS_TIMEOUT ) //Failed to find card.
    {
      Serial.print("RequestA failed with:");
      Serial.println( mfrc522.GetStatusCodeName(reqAResult) );
      //LightLEDPosition(28,  100,0,0, false); //Red warning LED

      CheckResetRFID();
    }
    /*else
    {
      Serial.println("No card - RequestA Timed out");
    }*/
  }

  //Serial.println("loop done");
}


void CheckResetRFID()
{
  errorCnt++;
  if( errorCnt > 6 )
  {
    errorCnt = 0;
    Serial.println("RESETTING RFID");
    mfrc522.PCD_Reset();
    InitRFIDReader(); //Will continiously retry (blocking) if init fails.
    delay(2000);
  }
}

void InitRFIDReader()
{
  Serial.println("Initializing the RFID Reader.");

  mfrc522.PCD_SetAntennaGain( 0x07 << 4 ); //PCD_RxGain.RxGain_max
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_SetAntennaGain( 0x07 << 4 ); //PCD_RxGain.RxGain_max
  timeElapsed = sendInterval;
  //mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details


  // Check it went OK.
  byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("Firmware Version: 0x"));
  Serial.print(v, HEX);
  // Lookup which version
  switch(v) {
    case 0x88: Serial.println(F(" = (clone)"));  break;
    case 0x90: Serial.println(F(" = v0.0"));     break;
    case 0x91: Serial.println(F(" = v1.0"));     break;
    case 0x92: Serial.println(F(" = v2.0"));     break;
    default:   Serial.println(F(" = (unknown)"));
  }
  
  if ((v == 0x00) || (v == 0xFF) || (v == 0x80)) //Bad reads, or 'UNKNOWN'
  {
    Serial.println(F("WARNING: Communication failure when Initializing the RFID reader.  Will sleep for 2000ms and retry."));
    delay(2000);
    InitRFIDReader();
  }
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
      
      
  } else {
      // we got it (obviously) but sender didn't get an ACK
      Serial.println("ERROR 1");
  }
}
void SendPlacedPacket( byte *buffer, byte bufferSize )
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
  
    SendCurrentPlacedPacket();
}
void SendCurrentPlacedPacket()
{
    placeMessage.setFrameId(xbee.getNextFrameId());
    
    Serial.println("SENDING 'Placed' Message to Co-ordinator");
    //xbee.send(placeMessage);
    
    // Send the command and wait up to N ms for a response.  xbee loop continues during this time.
    uint8_t status = xbee.sendAndWait(placeMessage, 1000);
    if (status == 0)
    {
      //Serial.println(F("SEND ACKNOWLEDGED"));
      timeElapsed = 0;       // reset the counter to 0 so the counting starts over...

    } else { //Complain, but do not reset timeElapsed - so that a new packet comes in and tried again immedietly.
      Serial.print(F("SEND FAILED: "));
      printHex(status, 2);
      Serial.println();
    }
}
void SendRemovedPacket()
{
  removedMessagePayload[0] = MSG_RFID_REMOVED;

  removedMessage.setFrameId(xbee.getNextFrameId());
     
  Serial.println("SENDING 'Removed' Message to Co-ordinator");
  //xbee.send(removedMessage);
  
  // Send the command and wait up to N ms for a response.  xbee loop continues during this time.
  uint8_t status = xbee.sendAndWait(removedMessage, 1000);
  if (status != 0){
    Serial.print(F("REMOVE PACKET SEND FAILED: "));
    printHex(status, 2);
    Serial.println();
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

  case MSG_SET_FLICKER_0:
    flickerState = 0;
    break;

  case MSG_SET_FLICKER_1:
    flickerState = 1;
    break;

  case MSG_SET_FLICKER_2:
    flickerState = 2;
    break;

  case MSG_SETUP_0:
    SetColourNone();
    LightNLEDs(1);
    break;
  case MSG_SETUP_1:
    SetColourNone();
    LightNLEDs(2);
    break;
  case MSG_SETUP_2:
    SetColourNone();
    LightNLEDs(3);
    break;
  case MSG_SETUP_3:
    SetColourNone();
    LightNLEDs(4);
    break;
  /*case MSG_SETUP_4:
    LightNLEDs(5);
    break;*/

  default: // If an invalid character, do nothing
    Serial.print("Unable to parse command: ");
    Serial.println(cmd);
    break;
  }
}






////LED FUNCTIONS 
void SetColourRed() {
  SetColour(DEFAULT_BRIGHTNESS,0,0);
}
void SetColourGreen() {
  SetColour(0,DEFAULT_BRIGHTNESS,0);
}
void SetColourYellow() {
  SetColour(DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS,0);
}
void SetColourWhite() {
  if(PINK_MOD)
    SetColour(DEFAULT_BRIGHTNESS*0.75,DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS);
  else
    SetColour(DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS);
}
void SetColourWhite(float brightness) {
  if(PINK_MOD)
    SetColour(brightness*0.75,brightness,brightness);
  else
    SetColour(brightness,brightness,brightness);
}
void SetColourBlue() {
  SetColour(0,0,DEFAULT_BRIGHTNESS);
}
void SetColourNone() {
  SetColour(0,0,0);
}

void SetColour(int red, int green, int blue)
{
  SetColour(red, green, blue, true);
}

void SetColour(int red, int green, int blue, bool usePixelStep)
{
  for(int i=0;i<NUMPIXELS;i++){
    
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    if( usePixelStep == false || i%PIXEL_STEP == 0)
      pixels.setPixelColor(i, pixels.Color(red, green, blue));
    else
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
     
    pixels.show(); // This sends the updated pixel color to the hardware.

  }

}

void LightNLEDs(int num)
{
  if(num>=1)
    LightLEDPosition(0, DEFAULT_BRIGHTNESS,0,0, false);
  if(num>=2)
    LightLEDPosition(63, 0,DEFAULT_BRIGHTNESS,0, false);
  if(num>=3)
    LightLEDPosition(7, 0,0,DEFAULT_BRIGHTNESS, false);
  if(num>=4)
    LightLEDPosition(56, DEFAULT_BRIGHTNESS,DEFAULT_BRIGHTNESS,0, false);
  if(num>=5)
    LightLEDPosition(36,  255,255,255, false);

}
void LightLEDPosition(int pos, int red, int green, int blue, bool usePixelStep )
{
  if(usePixelStep)
  {
    while( pos % PIXEL_STEP != 0 )
    {
      Serial.print("Finding nearest pixel to ");
      Serial.print(pos);
      Serial.print(" which fits into NUMPIXELS: ");
      Serial.println(NUMPIXELS);
      if( pos> (NUMPIXELS/2) )
        pos--;
      else
        pos++;
    }
  }
  
  Serial.println("Lighting Pixel num " + pos );
  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  pixels.setPixelColor(pos, pixels.Color(red, green, blue));
  pixels.show(); // This sends the updated pixel color to the hardware.
}


// UTIL FUNCTIONS
void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

// Restarts program from beginning but does not reset the peripherals and registers
void software_Reset() 
{
  asm volatile ("  jmp 0");  
}  
