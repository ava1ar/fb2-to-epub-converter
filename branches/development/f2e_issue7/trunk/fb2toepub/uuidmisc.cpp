//
//  Copyright (C) 2010 Alexey Bobkov
//
//  This file is part of Fb2toepub converter.
//
//  Fb2toepub converter is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Fb2toepub converter is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Fb2toepub converter.  If not, see <http://www.gnu.org/licenses/>.
//


#include "hdr.h"

#include "uuidmisc.h"
//#include <algorithm>
#include <ctype.h>

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
bool IsValidUUID(const String &id)
{
    return (id.length() == 36 &&
            isxdigit(id[0]) && isxdigit(id[1]) && isxdigit(id[2]) && isxdigit(id[3]) &&
            isxdigit(id[4]) && isxdigit(id[5]) && isxdigit(id[6]) && isxdigit(id[7]) &&
            id[8] == '-' &&
            isxdigit(id[9]) && isxdigit(id[10]) && isxdigit(id[11]) && isxdigit(id[12]) &&
            id[13] == '-' &&
            isxdigit(id[14]) && isxdigit(id[15]) && isxdigit(id[16]) && isxdigit(id[17]) &&
            id[18] == '-' &&
            isxdigit(id[19]) && isxdigit(id[20]) && isxdigit(id[21]) && isxdigit(id[22]) &&
            id[23] == '-' &&
            isxdigit(id[24]) && isxdigit(id[25]) && isxdigit(id[26]) && isxdigit(id[27]) &&
            isxdigit(id[28]) && isxdigit(id[29]) && isxdigit(id[30]) && isxdigit(id[31]) &&
            isxdigit(id[32]) && isxdigit(id[33]) && isxdigit(id[34]) && isxdigit(id[35]));
}

//-----------------------------------------------------------------------
String GenerateUUID()
{
    return "49fdf150-b8dd-11de-92bf-00a0d1e7a3b4";
}
    
//-----------------------------------------------------------------------
void MakeAdobeKey(const String &uuid, char *adobeKey)
{
#if defined(_DEBUG)
    if(!IsValidUUID(uuid))
        Error("uuid internal error");
#endif

    const char *p = uuid.c_str();
    bool high = true;
    for(;;)
    {
        char nibble;
        char c = *p++;
        switch(c)
        {
        case '\0':  return;
        case '-':   continue;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            nibble = (c - '0');
            break;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            nibble = (10 + (c - 'a'));
            break;

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            nibble = (10 + (c - 'A'));
            break;
        }

        if(high)
        {
            high = false;
            *adobeKey = (nibble << 4);
        }
        else
        {
            high = true;
            *adobeKey++ |= nibble;
        }
    }
}


};  //namespace Fb2ToEpub
