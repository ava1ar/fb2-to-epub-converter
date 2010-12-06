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

namespace Fb2ToEpub
{

    bool IsValidUUID(const String &id)
    {
        return false;
    }

    String GenerateUUID()
    {
        return "49fdf150-b8dd-11de-92bf-00a0d1e7a3b4";
    }
    
    void MakeAdobeKey(const String &id, char *adobeKey)
    {
        char *p = adobeKey;
        for(int i = 0; i < 1024/16; ++i)
        {
            memcpy(p, "\x49\xfd\xf1\x50\xb8\xdd\x11\xde\x92\xbf\x00\xa0\xd1\xe7\xa3\xb4", 16);
            p += 16;
        }
    }


};  //namespace Fb2ToEpub
