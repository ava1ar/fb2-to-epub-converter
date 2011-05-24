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
    XMLWriterImpl(OutStm *out, const String &encoding);
    ~XMLWriterImpl() {DoFlush();}

    // virtuals
    void StartFrame ();
    void EmptyTag   (const String &name, bool newLn, const AttrVector *attrs);
    void StartTag   (const String &name, bool newLn, const AttrVector *attrs);
    void Data       (const String &data);
    void EndTag     ();
    void EndFrame   ();
    void Flush      ()  {DoFlush();}

private:
    class Frame
    {
        typedef std::pair<String, bool> Tag;
        typedef std::vector<Tag>        TagVector;
        TagVector tags_;
    public:
        void StartTag(OutStm *out, const String &name, bool newLn, const AttrVector *attrs);
        void EndTag(OutStm *out);
        void End(OutStm *out);
    };
    typedef std::vector<Frame> FrameVector;

    Ptr<OutStm> out_;
    FrameVector frames_;

    void DoFlush();
};

//-----------------------------------------------------------------------
XMLWriterImpl::XMLWriterImpl(OutStm *out, const String &encoding) : out_(out)
{
    out_->WriteFmt("<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::Frame::StartTag(OutStm *out, const String &name, bool newLn, const AttrVector *attrs)
{
    if(!attrs || attrs->empty())
        out->WriteFmt("<%s>", name.c_str());
    else
    {
        out->WriteFmt("<%s", name.c_str());
        AttrVector::const_iterator cit = attrs->begin(), cit_end = attrs->end();
        for(; cit < cit_end; ++cit)
            out->WriteFmt(" %s=\"%s\"", cit->first.c_str(), cit->second.c_str());
        out->WriteStr(">");
    }
    tags_.push_back(TagVector::value_type(name, newLn));
}

//-----------------------------------------------------------------------
void XMLWriterImpl::Frame::EndTag(OutStm *out)
{
    if(tags_.empty())
        InternalError(__FILE__, __LINE__, "no XML writer frame tag");
    
    {
        Tag &tag = tags_.back();
        out->WriteFmt(tag.second ? "</%s>\n" : "</%s>", tag.first.c_str());
    }

    tags_.pop_back();
}

//-----------------------------------------------------------------------
void XMLWriterImpl::Frame::End(OutStm *out)
{
    TagVector::const_reverse_iterator cit = tags_.rbegin(), cit_end = tags_.rend();
    for(; cit < cit_end; ++cit)
        out->WriteFmt(cit->second ? "</%s>\n" : "</%s>", cit->first.c_str());
    tags_.clear();
}

//-----------------------------------------------------------------------
void XMLWriterImpl::StartFrame()
{
    frames_.push_back(FrameVector::value_type());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::EmptyTag(const String &name, bool newLn, const AttrVector *attrs)
{
    if(!attrs || attrs->empty())
        out_->WriteFmt("<%s/>", name.c_str());
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
void XMLWriterImpl::StartTag(const String &name, bool newLn, const AttrVector *attrs)
{
    if(frames_.empty())
        InternalError(__FILE__, __LINE__, "no XML writer frame 1");
    frames_.back().StartTag(out_, name, newLn, attrs);
}

//-----------------------------------------------------------------------
void XMLWriterImpl::Data(const String &data)
{
    out_->WriteStr(data.c_str());
}

//-----------------------------------------------------------------------
void XMLWriterImpl::EndTag()
{
    if(frames_.empty())
        InternalError(__FILE__, __LINE__, "no XML writer frame 2");
    frames_.back().EndTag(out_);
}

//-----------------------------------------------------------------------
void XMLWriterImpl::EndFrame()
{
    if(frames_.empty())
        InternalError(__FILE__, __LINE__, "no XML writer frame 3");
    frames_.back().End(out_);
    frames_.pop_back();
}

//-----------------------------------------------------------------------
void XMLWriterImpl::DoFlush()
{
    FrameVector::reverse_iterator it = frames_.rbegin(), it_end = frames_.rend();
    for(; it < it_end; ++it)
        it->End(out_);
   frames_.clear();
}

//-----------------------------------------------------------------------
Ptr<XMLWriter> FB2TOEPUB_DECL CreateXMLWriter(OutStm *out, const String &encoding)
{
    return new XMLWriterImpl(out, encoding);
}


};  //namespace Fb2ToEpub
