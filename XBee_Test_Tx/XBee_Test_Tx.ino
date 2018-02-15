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
 Sends a ZB TX request with the value of analogRead(pin5) and checks the status response for success
*/

SoftwareSerial xbeeSerial(2, 3); // RX, TX


// create the XBee object
XBee xbee = XBee();

uint8_t payload[] = { 0,0,0 };

// SH + SL Address of receiving XBee
XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x00000000);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

int errorLed = 13;
int dataLed = 13;
int statusLed = 13;


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
}

void loop() {
  
  //Build test Payload
  payload[0] = 'A';
  payload[1] = ':';
  payload[2] = 'C';

  xbee.send(zbTx);

  // flash TX indicator
  flashLed(statusLed, 1, 100);

  // after sending a tx request, we expect a status response
  // wait up to 2 seconds for the status response
  if (xbee.readPacket(2000)) {
    // got a response!

    Serial.print("Got Packet of len:");
    Serial.println( xbee.getResponse().getPacketLength() );

    
    // should be a znet tx status            	
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      xbee.getResponse().getZBTxStatusResponse(txStatus);


      Serial.print("ZBTxStatusResponse.getDeliveryStatus:");
      printHex(txStatus.getDeliveryStatus(), 4);
      Serial.println();
    
      // get the delivery status, the fifth byte
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        // success.  time to celebrate
        Serial.println("SUCCESS");
        flashLed(dataLed, 10, 50);
      } else {
        // the remote XBee did not receive our packet. is it powered on?
        Serial.println("ERROR 3");
        flashLed(errorLed, 3, 500);
      }
    }
  } else if (xbee.getResponse().isError()) {
    //nss.print("Error reading packet.  Error code: ");  
    //nss.println(xbee.getResponse().getErrorCode());
    Serial.println("ERROR 1");
    flashLed(errorLed, 1, 500);
  } else {
    // local XBee did not provide a timely TX Status Response -- should not happen
    Serial.println("ERROR 2");
    flashLed(errorLed, 2, 500);
  }

  delay(2000);
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}
