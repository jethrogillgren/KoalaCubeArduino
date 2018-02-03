// BreakoutBros RFID+LCD Tutorial

// 2016


// This tutorial uses the RC522 RFID Chip reader and its example

// Along with the 1602A LCD and its example

// Go to BreakoutBros.com to view wiring and step by step instructions

// RFID Setup

#include <AddicoreRFID.h> 
#include <SPI.h>

//LCD Setup

#include<LiquidCrystal.h>  
LiquidCrystal lcd(7, 8, 9, 6, 4, 3); //(RS,E,D4,D5,D6,D7)

#define uchar unsigned char

#define uint  unsigned int

uchar fifobytes;

uchar fifoValue;

AddicoreRFID myRFID; // create AddicoreRFID object to control the RFID module

/////////////////////////////////////////////////////////////////////

//set the pins

/////////////////////////////////////////////////////////////////////

const int chipSelectPin = 10;

const int NRSTPD = 5;

//Maximum length of the RFID array

#define MAX_LEN 16

  // Define All Key Codes – MAX 4 keys and 4 Names

uchar keys[16] = {

  32 , 32, 0, 0, //Jethro ID – These 4 digits are your 1st users ID Bytes
  10 , 89, 111, 133,   //Pickle ID – These 4 digits are your 2nd users ID Bytes
  
  000 , 000, 000, 000,  // Dummy 0 ID – These 4 digits are your 4th users ID Bytes
  111 , 111, 111, 111 // Dummy1 ID – These 4 digits are your 3rd users ID Bytes


};

String Names[4] = {

  "Jethro",     // 1st User- what will be printed to LCD *See keys[] for ID

  "Pickle",     // 2nd User- what will be printed to LCD *See keys[] for ID

  "Dummy0",   // 3rd User- what will be printed to LCD *See keys[] for ID

  "Dummy1"    // 4th User- what will be printed to LCD *See keys[] for ID

};

// Variable to Store Tag User Name to show on screen

String Tag_Name = "i"; // initialize it as "i" so Access denied is default

void setup() {

   Serial.begin(9600);                        // RFID reader SOUT pin connected to Serial RX pin at 9600bps 

  // start the SPI library for RFID

  SPI.begin();

  //RFID Config

  pinMode(chipSelectPin,OUTPUT);              // Set digital pin 10 as OUTPUT to connect it to the RFID /ENABLE pin

    digitalWrite(chipSelectPin, LOW);         // Activate the RFID reader

  pinMode(NRSTPD,OUTPUT);                     // Set digital pin 5 , Not Reset and Power-down

     digitalWrite(NRSTPD, HIGH);

  myRFID.AddicoreRFID_Init();  

  //LCD Initializations

  lcd.begin(16, 2);

  lcdprintmain();

}

void loop()

{

    uchar i, tmp, checksum1;

  uchar status;

        uchar str[MAX_LEN];

        uchar RC_size;

        uchar blockAddr;  //Selection operation block address 0 to 63

        String mynum = "";

        str[1] = 0x4400;

  //Find tags, return tag type

  status = myRFID.AddicoreRFID_Request(PICC_REQIDL, str); 

  //Anti-collision, return tag serial number 4 bytes

  status = myRFID.AddicoreRFID_Anticoll(str);

  if (status == MI_OK)

  {
        Serial.println("Read");
        uint tagType = str[0] << 8;
        tagType = tagType + str[1];
        checksum1 = str[0] ^ str[1] ^ str[2] ^ str[3]; // Calculate a checksum to make sure there is no error
        if(checksum1 == str[4])
        {
            Tag_Name = testkey(str,keys,Names);
            if(Tag_Name != "i")
            {
                  lcdprintwelcome(Tag_Name);
                  delay(2000);
                  lcdprintmain();
            } 
            else 
            {             
                lcdaccessdenied();//If the read tag isnt in our data base, we will print an Access Denied message
                delay(2000);
                lcdprintmain(); // refresh to home screen
            }
         }
         else
         {
                  lcd.setCursor(0,0);
                  lcd.write("    Tag Error   ");
                  lcd.setCursor(0,1);
                  lcd.write("   Try Again!   ");
                  delay(2000);
         }
           
   myRFID.AddicoreRFID_Halt();      //Command tag into hibernation 
  }
}
        


void lcdprintmain(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write("The BreakoutBros");
  lcd.setCursor(0,1);
  lcd.write(" Scan Your Card");
}

void lcdaccessdenied(){
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.write(" Access Denied  "); // print Access denied
}

void lcdprintwelcome(String Name){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("  Welcome Home  ");
    lcd.setCursor(5,1);
    lcd.print(Name);
}

String testkey(uchar Read_ID[4], uchar Stored_ID[16], String Stored_Name[4]){
    int n = 0; // Variable for Overall ID loop
    int test = 0; //Variable for testing each Byte
    int i = 0; // Variable for Read Byte Loop
    int g = 0;// Variable for Stored Byte Loop
    for(n=0; n < 4; n++) // Loop for each Name test
    {
      for(i=0; i<4; i++)  //Loop for each ID test
      {
        if(Read_ID[i] == Stored_ID[g]) // test bytes 0-3 of each ID
        {
          test = test+1; // if a test passes increase test by 1
        }
        g = g + 1; // Counter or stored ID indexing
      }
    if( test == 4) // if all 4 byts pass
    {
      return Stored_Name[n]; // return the place in the loop that passed
    }
    else
    {
      test = 0; // if not all loops pass
      i = 0;
     }
    }
    return "i"; // Return "i" for a failure - this is what we will use for access denied
    }
