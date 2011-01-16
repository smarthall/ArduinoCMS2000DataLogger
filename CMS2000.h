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

class CMS2000
{
  public:
    CMS2000();
  private:
    void sendCmd(int src, int dst, byte mode, byte type, byte extraCount, byte *extraData);
    byte sendVal(byte toSend);
    byte sendVal(int toSend);
};


#endif
