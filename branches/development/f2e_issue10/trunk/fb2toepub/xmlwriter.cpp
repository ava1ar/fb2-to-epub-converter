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

#include "xmlwriter.h"
#include "error.h"

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
// XMLWriter implementation
//-----------------------------------------------------------------------
class XMLWriterImpl : public XMLWriter
{
public:
    XMLWriterImpl(OutStm *out);
    XMLWriterImpl(OutStm *out, const String &encoding);

    // virtuals
    void DoEmptyElement (const String &name, bool ln, const AttrVector *attrs);
    void DoStartElement (const String &name, bool startLn, bool endLn, const AttrVector *attrs);
    void AddAttribute   (const String &name, const String &val);
    void AddAttributes  (const AttrVector *attrs);
    void Data           (const String &data);
    void EndElements    (int cnt);
    int ElementNumber   () const    {return elements_.size();}

private:
    typedef std::vector<std::pair<String, bool> > ElementVector;

    Ptr<OutStm>     out_;                   // output stream
    bool            currExists_;            // is element currently being written
    bool            currEmpty_, currLn_;    // description
    ElementVector   elements_;              // stack of open elements

    void FlushCurrentElement();
    void AddAttributesNoCheck(const AttrVector *attrs);
};

//-----------------------------------------------------------------------
XMLWriterImpl::XMLWriterImpl(OutStm *out)
    :   out_(out),
        currExists_(false),
        currEmpty_(false),
        currLn_(false)
{
    out_->WriteStr("<?xml version=\"1.0\"?>\n");
}

//-----------------------------------------------------------------------
XMLWriterImpl::XMLWriterImpl(OutStm *out, const String &encoding)
    :   out_(out),
        currExists_(false),
        currEmpty_(false),
        currLn_(false)
{
    out_->WriteFmt("<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::FlushCurrentElement()
{
    if(currExists_)
    {
        currExists_ = false;
        if(currEmpty_)
            out_->WriteStr(currLn_ ? "/>\n" : "/>");
        else
            out_->WriteStr(currLn_ ? ">\n" : ">");
    }
}

//-----------------------------------------------------------------------
void XMLWriterImpl::AddAttributesNoCheck(const AttrVector *attrs)
{
    AttrVector::const_iterator cit = attrs->begin(), cit_end = attrs->end();
    for(; cit < cit_end; ++cit)
        out_->WriteFmt(" %s=\"%s\"", cit->first.c_str(), cit->second.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::DoEmptyElement(const String &name, bool ln, const AttrVector *attrs)
{
    FlushCurrentElement();
    currExists_ = true;
    currEmpty_  = true;
    currLn_     = ln;

    out_->WriteFmt("<%s", name.c_str());
    if(attrs)
        AddAttributesNoCheck(attrs);
}

//-----------------------------------------------------------------------
void XMLWriterImpl::DoStartElement(const String &name, bool startLn, bool endLn, const AttrVector *attrs)
{
    FlushCurrentElement();
    currExists_ = true;
    currEmpty_  = false;
    currLn_     = startLn;

    elements_.push_back(ElementVector::value_type(name, endLn));
    out_->WriteFmt("<%s", name.c_str());
    if(attrs)
        AddAttributesNoCheck(attrs);
}

//-----------------------------------------------------------------------
void XMLWriterImpl::AddAttribute(const String &name, const String &val)
{
    if(!currExists_)
        InternalError(__FILE__, __LINE__, "no current element 1");
    out_->WriteFmt(" %s=\"%s\"", name.c_str(), val.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::AddAttributes(const AttrVector *attrs)
{
    if(!currExists_)
        InternalError(__FILE__, __LINE__, "no current element 2");
    if(attrs)
        AddAttributesNoCheck(attrs);
}

//-----------------------------------------------------------------------
void XMLWriterImpl::Data(const String &data)
{
    FlushCurrentElement();
    out_->WriteStr(data.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::EndElements(int cnt)
{
    FlushCurrentElement();

    if(cnt < 0)
        cnt = elements_.size();
    else if(elements_.size() < static_cast<size_t>(cnt))
        InternalError(__FILE__, __LINE__, "not enough count of XML writer elements");

    ElementVector::const_reverse_iterator cit = elements_.rbegin();
    for(int i = cnt; --i >= 0; ++cit)
        out_->WriteFmt(cit->second ? "</%s>\n" : "</%s>", cit->first.c_str());

    elements_.resize(elements_.size() - cnt);
}

//-----------------------------------------------------------------------
Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out)
{
    return new XMLWriterImpl(out);
}

//-----------------------------------------------------------------------
Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out, const String &encoding)
{
    return new XMLWriterImpl(out, encoding);
}


//-----------------------------------------------------------------------
// XMLWriter helpers
//-----------------------------------------------------------------------

// StartElementN
void XMLWriter::StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    DoStartElement(name, startLn, endLn, &av);
}
void XMLWriter::StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    DoStartElement(name, startLn, endLn, &av);
}
void XMLWriter::StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    DoStartElement(name, startLn, endLn, &av);
}
void XMLWriter::StartElement(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    av.push_back(AttrVector::value_type(a4, v4));
    DoStartElement(name, startLn, endLn, &av);
}
// EmptyElementN
void XMLWriter::EmptyElement(const String &name, bool ln, S_ a1, S_ v1)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    DoEmptyElement(name, ln, &av);
}
void XMLWriter::EmptyElement(const String &name, bool ln, S_ a1, S_ v1, S_ a2, S_ v2)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    DoEmptyElement(name, ln, &av);
}
void XMLWriter::EmptyElement(const String &name, bool ln, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    DoEmptyElement(name, ln, &av);
}
void XMLWriter::EmptyElement(const String &name, bool ln, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    av.push_back(AttrVector::value_type(a4, v4));
    DoEmptyElement(name, ln, &av);
}


};  //namespace Fb2ToEpub
