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
    void EmptyElement   (const String &name, bool endLn, const AttrVector *attrs);
    void StartElement   (const String &name, bool startLn, bool endLn, const AttrVector *attrs);
    void Data           (const String &data);
    void EndElements    (int cnt);

private:
    typedef std::vector<std::pair<String, bool> > ElementVector;

    Ptr<OutStm>     out_;
    ElementVector   elements_;
};

//-----------------------------------------------------------------------
XMLWriterImpl::XMLWriterImpl(OutStm *out) : out_(out)
{
    out_->WriteStr("<?xml version=\"1.0\"?>\n");
}

//-----------------------------------------------------------------------
XMLWriterImpl::XMLWriterImpl(OutStm *out, const String &encoding) : out_(out)
{
    out_->WriteFmt("<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::EmptyElement(const String &name, bool endLn, const AttrVector *attrs)
{
    if(!attrs)
        out_->WriteFmt(endLn ? "<%s/>\n" : "<%s/>", name.c_str());
    else
    {
        out_->WriteFmt("<%s", name.c_str());
        AttrVector::const_iterator cit = attrs->begin(), cit_end = attrs->end();
        for(; cit < cit_end; ++cit)
            out_->WriteFmt(" %s=\"%s\"", cit->first.c_str(), cit->second.c_str());
        out_->WriteStr(endLn ? "/>\n" : "/>");
    }
}

//-----------------------------------------------------------------------
void XMLWriterImpl::StartElement(const String &name, bool startLn, bool endLn, const AttrVector *attrs)
{
    if(!attrs)
        out_->WriteFmt(startLn ? "<%s>\n" : "<%s>", name.c_str());
    else
    {
        out_->WriteFmt("<%s", name.c_str());
        AttrVector::const_iterator cit = attrs->begin(), cit_end = attrs->end();
        for(; cit < cit_end; ++cit)
            out_->WriteFmt(" %s=\"%s\"", cit->first.c_str(), cit->second.c_str());
        out_->WriteStr(startLn ? ">\n" : ">");
    }
    elements_.push_back(ElementVector::value_type(name, endLn));
}

//-----------------------------------------------------------------------
void XMLWriterImpl::Data(const String &data)
{
    out_->WriteStr(data.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::EndElements(int cnt)
{
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
void XMLWriter::StartElement1(const String &name, bool startLn, bool endLn, S_ a1, S_ v1)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    StartElement(name, startLn, endLn, &av);
}
void XMLWriter::StartElement2(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    StartElement(name, startLn, endLn, &av);
}
void XMLWriter::StartElement3(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    StartElement(name, startLn, endLn, &av);
}
void XMLWriter::StartElement4(const String &name, bool startLn, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    av.push_back(AttrVector::value_type(a4, v4));
    StartElement(name, startLn, endLn, &av);
}
// EmptyElementN
void XMLWriter::EmptyElement1(const String &name, bool endLn, S_ a1, S_ v1)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    EmptyElement(name, endLn, &av);
}
void XMLWriter::EmptyElement2(const String &name, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    EmptyElement(name, endLn, &av);
}
void XMLWriter::EmptyElement3(const String &name, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    EmptyElement(name, endLn, &av);
}
void XMLWriter::EmptyElement4(const String &name, bool endLn, S_ a1, S_ v1, S_ a2, S_ v2, S_ a3, S_ v3, S_ a4, S_ v4)
{
    AttrVector av;
    av.push_back(AttrVector::value_type(a1, v1));
    av.push_back(AttrVector::value_type(a2, v2));
    av.push_back(AttrVector::value_type(a3, v3));
    av.push_back(AttrVector::value_type(a4, v4));
    EmptyElement(name, endLn, &av);
}


};  //namespace Fb2ToEpub
