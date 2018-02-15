/**
 * Copyright (c) 2009 Andrew Rapp. All rights reserved.
 *
 * This file is part of XBee-Arduino.
 *
 * XBee-Arduino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XBee-Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XBee-Arduino.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <XBee.h>
#include <SoftwareSerial.h>

/*
This example is for Series 2 XBee
Receives a ZB RX packet and sets a PWM value based on packet data.
Error led is flashed if an unexpected packet is received
*/

SoftwareSerial xbeeSerial(2, 3); // RX, TX
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

int statusLed = 13;
int errorLed = 13;
int dataLed = 13;

void flashLed(int pin, int times, int wait) {
    
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      
      if (i + 1 < times) {
        delay(wait);
      }
    }
}

void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  pinMode(dataLed,  OUTPUT);

  //Start debug HW serial
  Serial.begin(9600);

  // start xbee
  xbeeSerial.begin(9600);
  xbee.begin(xbeeSerial);
  
  flashLed(statusLed, 3, 50);
}

// continuously reads packets, looking for ZB Receive or Modem Status
void loop() {

  flashLed(statusLed, 1, 100);

    Serial.println(".");
    xbee.readPacket(10000);
    
    if (xbee.getResponse().isAvailable()) {
      // got something

      Serial.print("Got Packet of len:");
      Serial.println( xbee.getResponse().getPacketLength() );

      //FrameType: 0x90
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        Serial.println("got a ZB_RX_RESPONSE packet (0x90) ");
        
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);

            
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
            // the sender got an ACK
            Serial.println("SUCCESS - ACK");
            flashLed(dataLed, 10, 50);
        } else if ( rx.getOption() == ZB_BROADCAST_PACKET ) {
          // the sender got a unreliable broadcast packet
            Serial.println("SUCCESS - BROADCAST");
            flashLed(dataLed, 5, 50);
            
        } else {
            // we got it (obviously) but sender didn't get an ACK
            Serial.println("ERROR 7");
            flashLed(errorLed, 7, 500);
        }
        // set dataLed PWM to value of the first byte in the data
        analogWrite(dataLed, rx.getData(0));


      //FrameType: 0x8a
      } else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
        Serial.println("got a MODEM_STATUS_RESPONSE packet (0x8a) ");
        
        xbee.getResponse().getModemStatusResponse(msr);
        // the local XBee sends this response on certain events, like association/dissociation
        
        if (msr.getStatus() == ASSOCIATED) {
          // yay this is great.  flash led
          flashLed(dataLed, 3, 500);
        } else if (msr.getStatus() == DISASSOCIATED) {
          // this is awful.. flash led to show our discontent
        flashLed(errorLed, 6, 500);
        } else {
          // another status
        flashLed(errorLed, 5, 500);
        }
      } else {
      	// not something we were expecting
        flashLed(errorLed, 4, 500);    
      }


      
    } else if (xbee.getResponse().isError()) {
      Serial.print("Ignoring Frame which is in Error");

      //nss.print("Error reading packet.  Error code: ");  
      //nss.println(xbee.getResponse().getErrorCode());
        flashLed(errorLed, 2, 500);

    } else {

        Serial.print("Ignoring FrameType:");
        printHex(xbee.getResponse().getApiId(), 4);
        Serial.println();
        
        flashLed(errorLed, 1, 500);
    }


    //delay(2000);
}


void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

