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
    void EmptyElement   (const String &name, bool newLn, const AttrVector *attrs);
    void StartElement   (const String &name, bool newLn, const AttrVector *attrs);
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
void XMLWriterImpl::EmptyElement(const String &name, bool newLn, const AttrVector *attrs)
{
    if(!attrs)
        out_->WriteFmt(newLn ? "<%s/>\n" : "<%s/>", name.c_str());
    else
    {
        out_->WriteFmt("<%s", name.c_str());
        AttrVector::const_iterator cit = attrs->begin(), cit_end = attrs->end();
        for(; cit < cit_end; ++cit)
            out_->WriteFmt(" %s=\"%s\"", cit->first.c_str(), cit->second.c_str());
        out_->WriteStr(newLn ? "/>\n" : "/>");
    }
}

//-----------------------------------------------------------------------
void XMLWriterImpl::StartElement(const String &name, bool newLn, const AttrVector *attrs)
{
    if(!attrs)
        out_->WriteFmt("<%s>", name.c_str());
    else
    {
        out_->WriteFmt("<%s", name.c_str());
        AttrVector::const_iterator cit = attrs->begin(), cit_end = attrs->end();
        for(; cit < cit_end; ++cit)
            out_->WriteFmt(" %s=\"%s\"", cit->first.c_str(), cit->second.c_str());
        out_->WriteStr(">");
    }
    elements_.push_back(ElementVector::value_type(name, newLn));
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
    for(; --cnt >= 0; ++cit)
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


};  //namespace Fb2ToEpub
