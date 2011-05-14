//
//  Copyright (C) 2010, 2011 Alexey Bobkov
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

#include <sstream>
#include "converter.h"

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
int PrintInfo(const String &in)
{
    DoPrintInfo(in);
    return 0;
}

//-----------------------------------------------------------------------
int Convert(InStm *pin, const strvector &css, const strvector &fonts, const strvector &mfonts,
            XlitConv *xlitConv, OutPackStm *pout)
{
    /*
    // test of new pass 1
    {
        UnitArray units;
        DoConvertionPass1_new(CreateScanner(pin), &units);
        pin->Rewind();

#if 1
        {
            printf("TEST RESULTS:\n\n\n\n");

            for(std::size_t i = 0; i < units.size(); ++i)
                printf ("%d %d-%d-%d %s size=%d, parent=%d, level = %d\n", i, units[i].bodyType_, units[i].type_,
                        units[i].id_, units[i].title_.c_str(), units[i].size_, units[i].parent_, units[i].level_);

            printf("\n\nREGULAR RESULTS:\n\n\n\n");
        }
#endif
    }
    */


    // perform pass 1 to determine fb2 document structure and to collect all cross-references inside the fb2 file
    UnitArray units;
    DoConvertionPass1_new(CreateScanner(pin), &units);
    pin->Rewind();

    // sanity check
    if(units.size() == 0)
        InternalError(__FILE__, __LINE__, "I don't know why but it happened that there is no content in input file!");

    // perform pass 2 to create epub document
    DoConvertionPass2(CreateScanner(pin), css, fonts, mfonts, xlitConv, &units, pout);
    return 0;
}


};  //namespace Fb2ToEpub
