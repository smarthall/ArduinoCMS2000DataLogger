/*
 ArduinoCMS200DataLogger - Gathers Data from a CMS2000 and logs it to an SDCard
 Copyright (C) 2011 Daniel Hall

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#include <SoftwareSerial.h>

// Software Serial Pins
#define rxPin 2
#define txPin 3

#define OkPin 13

// Command indexes
#define CMD_RESET 0
#define CMD_POLL 1
#define CMD_GET_SERIAL 2
#define CMD_SEND_SERIAL 3

// Command data
byte commands[4][13] = {{11, 0xAA, 0xAA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x59, 0x00},
                        {11, 0xAA, 0xAA, 0x01, 0x00, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01, 0x59, 0x00},
                        {11, 0xAA, 0xAA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x55, 0x00},
                        { 9, 0xAA, 0xAA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x00, 0x00, 0x00}};
  
// Response indexes               
#define RESP_SERIAL 0
#define RESP_CONFIRM 1
                 
// Reponse data
byte responses[2][13] = {{9,  0xAA, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x0A, 0x00, 0x00, 0x00},
                         {12, 0xAA, 0xAA, 0x00, 0x01, 0x01, 0x00, 0x00, 0x81, 0x01, 0x06, 0x01, 0xDE}};

char serial[11];

// Software Serial Port
SoftwareSerial openlog = SoftwareSerial(rxPin, txPin);

// Buffer for recvieve data
byte recieveBuffer[128];

// This function sends one of the predefined commands
void sendCMD(byte command) {
  byte bytesTo = commands[command][0] + 1;
  for (int i = 1; i <= bytesTo; i++) {
    Serial.write(commands[command][i]);
    
  }
}

// This function returns after either 128 bytes
// are read, or wait milliseconds passes
int getResp(int wait) {
  int position = 0;
  unsigned long endtime = millis() + wait;
  
  // Until timeout
  while (millis() < endtime) {
    // If there is data ready
    if (Serial.available() > 0) {
      // Read it in
      recieveBuffer[position] = Serial.read();
      position = position + 1;
      
      // Exit at 128 bytes
      if (position == 102) return 102;
    }
    
    // TODO: Is a delay() needed here for power saving
  }
  
  // Tell them how much data was read
  return position;
}

boolean checkResp(int count, int respCode) {
  // If the response is too small to match
  if (count < responses[respCode][0])
    return false;
  
  // If the data doesnt match
  for (int i = 1; i <= responses[respCode][0]; i++) {
    // recieveBuffer starts at 0, responses starts at 1
    if (recieveBuffer[i - 1] != responses[respCode][i])
      return false;
  }
  
  return true;
}

boolean handshake() {
  int respCount, checksum = 0;
  byte bytesTo;
  
  // Ask for the inverters Serial
  openlog.println("Requesting Serial Number");
  sendCMD(CMD_GET_SERIAL);
  
  // Wait for three seconds
  respCount = getResp(1000);
  
  // Check the response is valid
  if (!checkResp(respCount, RESP_SERIAL))
    return false;
    
  // Get serial
  for (int i = 9; i <= 18; i++) {
    serial[i - 9] = recieveBuffer[i];
  }

  serial[10] = '\0';
  
  // Put serial in the log
  openlog.print("Serial Number is: ");
  openlog.println(serial);
  
  // Write out the command
  // Manually because we need to calculate checksum
  bytesTo = commands[CMD_SEND_SERIAL][0];
  for (int i = 1; i <= bytesTo; i++) {
    Serial.write(commands[CMD_SEND_SERIAL][i]);
    checksum += commands[CMD_SEND_SERIAL][i];
  }
  
  for (int i = 0; i < 10; i++) {
    Serial.write(serial[i]);
    checksum += serial[i];
  }
  
  Serial.print(0x01, BYTE);
  checksum += 0x01;

  Serial.print(checksum >> 8, BYTE);
  Serial.print(checksum & 0xFF, BYTE);
  
  // Wait for three seconds
  respCount = getResp(1000);
  
  // Check the response is valid
  if (!checkResp(respCount, RESP_CONFIRM))
    return false;
    
  digitalWrite(OkPin, HIGH);
  
  return true;
}

void printBuffer(int count) {
  for (int i = 0; i < count; i++) {
    openlog.print(recieveBuffer[i], BYTE);
    delay(5);
  }
}

void setup() {
  pinMode(OkPin, OUTPUT);
  digitalWrite(OkPin, LOW);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, HIGH);
  Serial.begin(9600);
  openlog.begin(9600);
  
  delay(3000);
  
  openlog.println("Resetting Inverter... ");
  
  // Send a reset command three times, like the software does
  for (int i = 0; i < 4; i++) {
    sendCMD(CMD_RESET);
    delay(1000);
  }
  
  // Flush any stange data from the Serial port
  Serial.flush();
  
  openlog.println("Done!");
  
  openlog.println("Performing Handshake... ");
  if (handshake()) {
    openlog.println("Handshake Successful!");
    openlog.println("Beggining Binary Data");
  } else {
    openlog.println("Handshake Failure! :( Will Retry Soon");
  }
  

}

void loop() {
  int respCount;
  int high_byte;
  int low_byte;
  int Data[14];
  
  if (digitalRead(OkPin) == 1) {
  delay(9000);
  sendCMD(CMD_POLL);
  respCount = getResp(1000);
  if (respCount > 0) {
    openlog.print(millis());
    for (int i = 9, j = 0; i < 36; i++) {
      
      
      high_byte = recieveBuffer[i] * 256;
      i++;
      low_byte = recieveBuffer[i];
      Data[j] = high_byte + low_byte;
      j++;
      
      }
  
  openlog.print(',');
  openlog.print(Data[0]);
  openlog.print(',');
  openlog.print(Data[1]);
  openlog.print(',');
  openlog.print(Data[2]);
  openlog.print(',');
  openlog.print(Data[3]);
  openlog.print(',');
  openlog.print(Data[4]);
  openlog.print(',');
  openlog.print(Data[5]);
  openlog.print(',');
  openlog.print(Data[6]);
  openlog.print(',');
  openlog.print(Data[9]);
  openlog.print(',');
  openlog.print(Data[10]);
  openlog.print(',');
  openlog.print(Data[13]);
  openlog.print(',');
  openlog.print(Data[14]);
  openlog.println('\n');
  
  
  
  
  }
  
  else {
    openlog.println('\n');
    openlog.println("Error.... No Data");
      digitalWrite(OkPin, LOW);
} }
else {

  delay (60000);
  openlog.println("Waited 60 seconds, now try again");
  
  // Send a reset command three times, like the software does
  for (int i = 0; i < 4; i++) {
    sendCMD(CMD_RESET);
    delay(1000);
  }
  
  // Flush any stange data from the Serial port
  Serial.flush();
  
  openlog.println("Performing Handshake... ");
  if (handshake()) {
    openlog.println("Handshake Successful!");
  } else {
    openlog.println("Handshake Failure! :( Will Retry Soon");
  }}
}
