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
        typedef const String &S_;
    public:
        virtual void EmptyElement   (const String &name, bool endLn, const AttrVector *attrs = NULL)                = 0;
        virtual void StartElement   (const String &name, bool startLn, bool endLn, const AttrVector *attrs = NULL)  = 0;
        virtual void Data           (const String &data)                                                            = 0;
        virtual void EndElements    (int cnt)                                                                       = 0;

        // helpers

        void EndElement()   {EndElements(1);}       // end last element
        void Flush()        {EndElements(-1);}      // end all elements

        // StartElementN
        void StartElement0(const String &name, bool startLn, bool endLn) {StartElement(name, startLn, endLn);}
        void StartElement1(const String &name, bool startLn, bool endLn, S_ a1, S_ v1);
        void StartElement2(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2);
        void StartElement3(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3);
        void StartElement4(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4);

        // EmptyElementN
        void EmptyElement0(const String &name, bool endLn) {EmptyElement(name, endLn);}
        void EmptyElement1(const String &name, bool endLn, S_ a1, S_ v1);
        void EmptyElement2(const String &name, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2);
        void EmptyElement3(const String &name, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3);
        void EmptyElement4(const String &name, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4);
    };
    //-----------------------------------------------------------------------
    Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out);
    Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out, const String &encoding);

};  //namespace Fb2ToEpub

#endif
