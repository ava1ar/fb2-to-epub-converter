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

    protected:
        virtual void DoEmptyElement (const String &name, bool ln, const AttrVector *attrs)                  = 0;
        virtual void DoStartElement (const String &name, bool startLn, bool endLn, const AttrVector *attrs) = 0;

    public:
        virtual void AddAttribute   (const String &name, const String &val)                                 = 0;
        virtual void AddAttributes  (const AttrVector *attrs)                                               = 0;
        virtual void Data           (const String &data)                                                    = 0;
        virtual void EndElements    (int cnt)                                                               = 0;

        virtual int  ElementNumber  () const                                                                = 0;

        // StartElement
        void StartElement(const String &name, bool startLn, bool endLn, const AttrVector *attrs = NULL)
            {DoStartElement(name, startLn, endLn, attrs);}

        // EmptyElement
        void EmptyElement(const String &name, bool ln, const AttrVector *attrs = NULL)
            {DoEmptyElement(name, ln, attrs);}

        // helpers

        void StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1);
        void StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2);
        void StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3);
        void StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4);
        void EmptyElement(const String &name, bool ln, S_ a1, S_ v1);
        void EmptyElement(const String &name, bool ln, S_ a1, S_ v1, S_ a2, S_ v2);
        void EmptyElement(const String &name, bool ln, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3);
        void EmptyElement(const String &name, bool ln, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4);

        void EndElement()   {EndElements(1);}       // end last element
        void Flush()        {EndElements(-1);}      // end all elements
    };
    //-----------------------------------------------------------------------
    Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out);
    Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out, const String &encoding);

    //-----------------------------------------------------------------------
    // Helper framing class
    //-----------------------------------------------------------------------
    class XMLFrame
    {
        XMLWriter   *wrt_;
        int         initElementNumber_;
    public:
        XMLFrame(XMLWriter *wrt) :
            wrt_(wrt), initElementNumber_(wrt->ElementNumber()) {}

        void End()
        {
            wrt_->EndElements(wrt_->ElementNumber() - initElementNumber_);
        }
    };

};  //namespace Fb2ToEpub

#endif
