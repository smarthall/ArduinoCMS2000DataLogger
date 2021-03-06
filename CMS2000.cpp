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

#include <WProgram.h>
#include "CMS2000.h"

// Recieve Timeout
#define CMS2000_RECIEVE_TIMEOUT 3000

// Communication start
#define CMS2000_COMM_START   0xAAAA

// Command Modes
#define CMS2000_MODE_NETWORK 0
#define CMS2000_MODE_COMMS   1

// Network addresses
#define CMS2000_BROADCAST    0x00
#define CMS2000_SELF         0x10
#define CMS2000_INVERTER1    0x01
#define CMS2000_INVERTER2    0x02
#define CMS2000_INVERTER3    0x03
#define CMS2000_INVERTER4    0x04
#define CMS2000_INVERTER5    0x05
#define CMS2000_INVERTER6    0x06
#define CMS2000_INVERTER7    0x07
#define CMS2000_INVERTER8    0x08

// Network Commands
#define CMS2000_QUERY        0x00
#define CMS2000_ASSIGN       0x01
#define CMS2000_RESET        0x04

// Communication commands
#define CMS2000_GETDATA      0x02
#define CMS2000_GETINFO      0x03

// Responses
#define CMS2000_SERIAL       0x80
#define CMS2000_ACK          0x81
#define CMS2000_DATA         0x82
#define CMS2000_INFO         0x83

CMS2000::CMS2000(unsigned int myDst) {
  dst = myDst;
  state = CMS2000_STATE_INIT;
  serial[0] = '\0';
}

int CMS2000::handshake() {
  int readResponse;
  unsigned int src, dst;
  byte mode, type, extraCount;
  byte extraData[64];
  
  // Reset CMS2000 network three times
  for (int i = 0; i < 3; i++) {
    sendCmd(CMS2000_SELF, CMS2000_BROADCAST, CMS2000_MODE_NETWORK, CMS2000_RESET, 0, NULL);
    delay(500);
  }
  
  // Query the network looking for inverters
  sendCmd(CMS2000_SELF, CMS2000_BROADCAST, CMS2000_MODE_NETWORK, CMS2000_QUERY, 0, NULL);
  
  // Expect a response from an inverter
  readResponse = recvCmd(&src, &dst, &mode, &type, &extraCount, extraData);
  if (readResponse != CMS2000_SUCCESS)
    return readResponse;
    
  // Check that the packet was for us, is of type serial and has ten digits of serial
  if ((dst != CMS2000_SELF) || (mode != CMS2000_MODE_NETWORK) || (type != CMS2000_SERIAL) || (extraCount != 10))
    return CMS2000_ERROR_INVALID;
  
  // Copy the serial into the serial string
  memcpy(serial, extraData, extraCount);
  serial[extraCount] = '\0';
  
  // Assign an address to the inverter
  sendCmd(CMS2000_SELF, CMS2000_BROADCAST, CMS2000_MODE_NETWORK, CMS2000_ASSIGN, 10, extraData);
  
  // Check the inverter has accepted the new address
  readResponse = recvCmd(&src, &dst, &mode, &type, &extraCount, extraData);
  if (readResponse != CMS2000_SUCCESS)
    return readResponse;
    
  // Check that the packet was for us, is of type acknowledgement and has no extra data
  if ((dst != CMS2000_SELF) || (mode != CMS2000_MODE_NETWORK) || (type != CMS2000_ACK) || (extraCount == 0))
    return CMS2000_ERROR_INVALID;
}

String CMS2000::getSerial() {
  if (state == CMS2000_STATE_CONNECTED) {
    return String(serial);
  } else {
    return String();
  }
}

int CMS2000::getState() {
  return state;
}

unsigned int CMS2000::getDestination() {
  return dst;
}

void CMS2000::sendCmd(unsigned int src, unsigned int dst, byte mode,
                      byte type, byte extraCount = 0, byte *extraData = NULL) {
  int checksum = 0;
  
  // Command beginning
  checksum += sendInt(CMS2000_COMM_START);
  
  // Source Address
  checksum += sendInt(src);
  
  // Destination Address
  checksum += sendInt(dst);
  
  // Command Mode
  checksum += sendByte(mode);
  
  // Command Type
  checksum += sendByte(type);
  
  // Extra Byte Count
  checksum += sendByte(extraCount);
  
  // Extra Bytes
  for (int i = 0; i < extraCount; i++)
    checksum += sendByte(extraData[i]);
  
  // Checksum
  sendInt(checksum);
}

int CMS2000::recvCmd(unsigned int *src, unsigned int *dst, byte *mode,
                      byte *type, byte *extraCount, byte *extraData) {
  long int quitAt;                      
  unsigned int intRead;
  unsigned int readChecksum;
  unsigned int checksum = 0;
  
  // When is quitting time?
  quitAt = millis() + CMS2000_RECIEVE_TIMEOUT;
  
  if (recvInt(&checksum, &intRead, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
    
  if (intRead != CMS2000_COMM_START)
    return CMS2000_ERROR_NOSTART;
    
  if (recvInt(&checksum, src, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
    
  if (recvInt(&checksum, dst, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
    
  if (recvByte(&checksum, mode, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
    
  if (recvByte(&checksum, type, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
    
  if (recvByte(&checksum, extraCount, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
  
  // TODO: Buffer overflow potential
  for (int i = 0; i < *extraCount; i++) {
    if (recvByte(&checksum, extraData + i, quitAt) != CMS2000_SUCCESS)
      return CMS2000_ERROR_TIMEOUT;
  }
  
  if (recvInt(&intRead, &readChecksum, quitAt) != CMS2000_SUCCESS)
    return CMS2000_ERROR_TIMEOUT;
    
  if (readChecksum != checksum)
    return CMS2000_ERROR_CHECKSUM;
}

byte CMS2000::sendByte(byte toSend) {
  Serial.write(toSend);
  
  return toSend;
}

byte CMS2000::sendInt(int toSend) {
  byte hi_byte  = toSend >> 8;
  byte low_byte = toSend & 0xFF;
  
  Serial.write(hi_byte);
  Serial.write(low_byte);
  
  return hi_byte + low_byte;
}

int CMS2000::recvByte(unsigned int *checksum, byte *byteRead, long int quitAt) {
  while (millis() < quitAt) {
    if (Serial.available() > 0) {
      *byteRead = Serial.read();
      *checksum += *byteRead;
      
      return CMS2000_SUCCESS;
    }
  }
  
  return CMS2000_ERROR_TIMEOUT;
}

int CMS2000::recvInt(unsigned int *checksum, unsigned int *intRead, long int quitAt) {
    byte hi_byte, low_byte;
  
    while (millis() < quitAt) {
    if (Serial.available() > 1) {
      hi_byte = Serial.read();
      low_byte = Serial.read();
      *checksum += (int)hi_byte + low_byte;
      *intRead = (hi_byte * 0x10) + low_byte;
      
      return CMS2000_SUCCESS;
    }
  }
  
  return CMS2000_ERROR_TIMEOUT;
}


