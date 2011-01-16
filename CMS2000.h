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

#ifndef H_CMS2000
#define H_CMS2000

#include <WProgram.h>

#define CMS2000_SUCCESS        0
#define CMS2000_ERROR_NOSTART  1
#define CMS2000_ERROR_INVALID  2
#define CMS2000_ERROR_SIZE     3
#define CMS2000_ERROR_TIMEOUT  4
#define CMS2000_ERROR_CHECKSUM 5

class CMS2000
{
  public:
    CMS2000();
  private:
    void sendCmd(unsigned int src, unsigned int dst, byte mode, byte type, byte extraCount, byte *extraData);
    int recvCmd(unsigned int *src, unsigned int *dst, byte *mode, byte *type, byte *extraCount, byte *extraData);
    byte sendByte(byte toSend);
    byte sendInt(int toSend);
    int recvByte(unsigned int *checksum, byte *byteRead, long int quitAt);
    int recvInt(unsigned int *checksum, unsigned int *intRead, long int quitAt);
};


#endif
