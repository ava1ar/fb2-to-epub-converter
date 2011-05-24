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


#ifndef FB2TOEPUB__XMLWRITER_H
#define FB2TOEPUB__XMLWRITER_H

#include "stream.h"
#include <utility>
#include <vector>

namespace Fb2ToEpub
{
    //-----------------------------------------------------------------------
    typedef std::vector<std::pair<String, String> > AttrVector;

    //-----------------------------------------------------------------------
    // Useful XML writer
    //-----------------------------------------------------------------------
    class XMLWriter : public Object
    {
    public:
        virtual void EmptyElement   (const String &name, bool newLn, const AttrVector *attrs = NULL)    = 0;
        virtual void StartElement   (const String &name, bool newLn, const AttrVector *attrs = NULL)    = 0;
        virtual void Data           (const String &data)                                                = 0;
        virtual void EndElements    (int cnt)                                                           = 0;

        // helper
        void EndElement()   {EndElements(1);}       // end last element
        void Flush()        {EndElements(-1);}      // end all elements
    };
    //-----------------------------------------------------------------------
    Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out);
    Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out, const String &encoding);

};  //namespace Fb2ToEpub

#endif
