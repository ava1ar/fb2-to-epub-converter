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

#include "fb2parser.h"
#include <vector>
#include <set>
#include <sstream>

namespace Fb2ToEpub
{

static const Fb2ElementInfo einfo[E_COUNT] =
{
    //              name                  notempty? id?     href?   lang?
    Fb2ElementInfo("FictionBook",           true,   false,  false,  false),
    Fb2ElementInfo("a",                     false,  false,  true,   false),
    Fb2ElementInfo("annotation",            false,  true,   false,  true),
    Fb2ElementInfo("author",                false,  false,  false,  false),
    Fb2ElementInfo("binary",                true,   true,   false,  false),
    Fb2ElementInfo("body",                  false,  false,  false,  true),
    Fb2ElementInfo("book-name",             false,  false,  false,  true),
    Fb2ElementInfo("book-title",            false,  false,  false,  true),
    Fb2ElementInfo("cite",                  true,   true,   false,  true),
    Fb2ElementInfo("city",                  false,  false,  false,  true),
    Fb2ElementInfo("code",                  false,  false,  false,  false),
    Fb2ElementInfo("coverpage",             false,  false,  false,  false),
    Fb2ElementInfo("custom-info",           false,  false,  false,  false),
    Fb2ElementInfo("date",                  false,  false,  false,  true),
    Fb2ElementInfo("description",           true,   false,  false,  false),
    Fb2ElementInfo("document-info",         false,  false,  false,  false),
    Fb2ElementInfo("email",                 false,  false,  false,  false),
    Fb2ElementInfo("emphasis",              false,  false,  false,  false),
    Fb2ElementInfo("empty-line",            false,  false,  false,  false),

    //              name                  notempty? id?     href?   lang?
    Fb2ElementInfo("epigraph",              false,  true,   false,  false),
    Fb2ElementInfo("first-name",            false,  false,  false,  true),
    Fb2ElementInfo("genre",                 false,  false,  false,  false),
    Fb2ElementInfo("history",               false,  true,   false,  true),
    Fb2ElementInfo("home-page",             false,  false,  false,  false),
    Fb2ElementInfo("id",                    false,  false,  false,  false),
    Fb2ElementInfo("isbn",                  false,  false,  false,  true),
    Fb2ElementInfo("image",                 false,  true,   true,   false),     // <image> in <body>, <section> (except top image)
    Fb2ElementInfo("image",                 false,  false,  true,   false),     // <image> in all others (inline)
    Fb2ElementInfo("image",                 false,  true,   true,   false),     // <image> in the top of the <section>
    Fb2ElementInfo("keywords",              false,  false,  false,  false),
    Fb2ElementInfo("lang",                  false,  false,  false,  false),
    Fb2ElementInfo("last-name",             false,  false,  false,  true),
    Fb2ElementInfo("middle-name",           false,  false,  false,  true),
    Fb2ElementInfo("nickname",              false,  false,  false,  true),
    Fb2ElementInfo("output-document-class", false,  false,  false,  false),
    Fb2ElementInfo("output",                false,  false,  false,  false),
    Fb2ElementInfo("p",                     false,  true,   false,  true),
    Fb2ElementInfo("part",                  false,  false,  true,   false),

    //              name                  notempty? id?     href?   lang?
    Fb2ElementInfo("poem",                  false,  true,   false,  true),
    Fb2ElementInfo("program-used",          false,  false,  false,  true),
    Fb2ElementInfo("publish-info",          false,  false,  false,  false),
    Fb2ElementInfo("publisher",             false,  false,  false,  true),      // <publisher> in <publish-info>
    Fb2ElementInfo("publisher",             false,  false,  false,  false),     // <publisher> in <document-info>
    Fb2ElementInfo("section",               false,  true,   false,  true),
    Fb2ElementInfo("sequence",              false,  false,  false,  true),
    Fb2ElementInfo("src-lang",              false,  false,  false,  false),
    Fb2ElementInfo("src-ocr",               false,  false,  false,  true),
    Fb2ElementInfo("src-title-info",        false,  false,  false,  false),
    Fb2ElementInfo("src-url",               false,  false,  false,  false),
    Fb2ElementInfo("stanza",                false,  false,  false,  false),
    Fb2ElementInfo("strikethrough",         false,  false,  false,  false),
    Fb2ElementInfo("strong",                false,  false,  false,  false),
    Fb2ElementInfo("style",                 false,  false,  false,  true),
    Fb2ElementInfo("stylesheet",            false,  false,  false,  false),
    Fb2ElementInfo("sub",                   false,  false,  false,  false),
    Fb2ElementInfo("subtitle",              false,  false,  false,  false),
    Fb2ElementInfo("sup",                   false,  false,  false,  false),

    //              name                  notempty? id?     href?   lang?
    Fb2ElementInfo("table",                 false,  true,   false,  false),
    Fb2ElementInfo("td",                    false,  true,   false,  true),
    Fb2ElementInfo("text-author",           false,  true,   false,  true),
    Fb2ElementInfo("th",                    false,  true,   false,  true),
    Fb2ElementInfo("title",                 false,  false,  false,  true),
    Fb2ElementInfo("title-info",            true,   false,  false,  false),
    Fb2ElementInfo("tr",                    false,  false,  false,  false),
    Fb2ElementInfo("translator",            false,  false,  false,  false),
    Fb2ElementInfo("v",                     false,  true,   false,  true),
    Fb2ElementInfo("version",               false,  false,  false,  false),
    Fb2ElementInfo("year",                  false,  false,  false,  false)
};

//-----------------------------------------------------------------------
inline void EmptyElementError(LexScanner *s, const String &name)
{
    std::ostringstream ss;
    ss << "element <" << name << "> can't be empty";
    s->Error(ss.str());
}


//-----------------------------------------------------------------------
typedef std::vector<Fb2EType> TypeVector;


//-----------------------------------------------------------------------
// Fb2Host helper methods
//-----------------------------------------------------------------------
Fb2EType Fb2Host::Type() const
{
    size_t size = GetTypeStackSize();
#if defined(_DEBUG)
    if(size < 1)
        InternalError(__FILE__, __LINE__, "empty type stack");
#endif
    return GetTypeStackAt(size-1);
}
Fb2EType Fb2Host::ParentType() const
{
    size_t size = GetTypeStackSize();
#if defined(_DEBUG)
    if(size < 2)
        InternalError(__FILE__, __LINE__, "no parent element type");
#endif
    return GetTypeStackAt(size-2);
}
String Fb2Host::GetAttrValue(const String &attr) const
{
    const AttrMap& attrmap = GetAttributes();
    AttrMap::const_iterator cit = attrmap.find(attr);
    if(cit != attrmap.end())
        return cit->second;
    else
        return "";
}


//-----------------------------------------------------------------------
// Dummy namespace lookup
//-----------------------------------------------------------------------
class DummyNsLookup : public Fb2NsLookup
{
public:
    //virtual
    String Findhref(const AttrMap&) const
    {
        InternalError(__FILE__, __LINE__, "Namespace lookup isn't registered");
        return "";
    }
};

//-----------------------------------------------------------------------
// State structure
//-----------------------------------------------------------------------
struct ParserState
{
    TypeVector          elemTypeStack_;
    Ptr<LexScanner>     s_;
    Ptr<Fb2NsLookup>    nsLookup_;

    ParserState(LexScanner *s) : s_(s), nsLookup_(new DummyNsLookup()) {}
};


//-----------------------------------------------------------------------
// Exit parser exception
//-----------------------------------------------------------------------
struct ExitParserException {};


//-----------------------------------------------------------------------
// Auto handler
//-----------------------------------------------------------------------
class AutoHandler : private Fb2Host, Noncopyable
{
public:
    AutoHandler(Fb2EType type, ParserState *prsState, Fb2Ctxt *ctxt);

    bool            StartTag();
    Fb2Ctxt*        Ctxt() const                            {return newCtxt_;}
    void            Data(const String &data, size_t size)   {ph_->Data(data, size);}
    void            EndTag();

    //virtuals
    const AttrMap&  GetAttributes() const;
    String          Findhref() const                        {return prsState_->nsLookup_->Findhref(attrmap_);}
    size_t          GetTypeStackSize() const                {return prsState_->elemTypeStack_.size();}
    Fb2EType        GetTypeStackAt(int i) const             {return prsState_->elemTypeStack_[i];}
    LexScanner*     Scanner() const                         {return prsState_->s_;}
    void            RegisterNsLookup(Fb2NsLookup *lookup)   {prsState_->nsLookup_ = lookup;}
    void            Exit() const                            {throw ExitParserException();}

private:
    enum MyState {NOT_SCANNED, SCANNED_EMPTY, SCANNED_NOTEMPTY};

    Fb2EType            type_;
    ParserState         *prsState_;
    mutable MyState     state_;
    mutable bool        hasAttributes_;
    mutable AttrMap     attrmap_;
    Ptr<Fb2EHandler>    ph_;
    Ptr<Fb2Ctxt>        newCtxt_;
};

//-----------------------------------------------------------------------
AutoHandler::AutoHandler(Fb2EType type, ParserState *prsState, Fb2Ctxt *ctxt)
    :   type_(type),
        prsState_(prsState),
        state_(NOT_SCANNED),
        hasAttributes_(false)
{
    ctxt->GetNext(type_, &ph_, &newCtxt_);
}

//-----------------------------------------------------------------------
bool AutoHandler::StartTag()
{
    prsState_->elemTypeStack_.push_back(type_);
    if (ph_->StartTag(type_, this))
    {
        prsState_->elemTypeStack_.pop_back();
        return true;    // handler has done all itself
    }

    if(state_ == NOT_SCANNED)
    {
        const Fb2ElementInfo &ei = einfo[type_];
        if(prsState_->s_->BeginElement(ei.name_))
            state_ = SCANNED_NOTEMPTY;
        else
        {
            if(ei.notempty_)
                EmptyElementError(prsState_->s_, ei.name_);
            state_ = SCANNED_EMPTY;
        }
    }

    switch(state_)
    {
    default:
        InternalError(__FILE__, __LINE__, "bad host state");
        return true;

    case SCANNED_EMPTY:
        ph_->EndTag(true, this);
        prsState_->elemTypeStack_.pop_back();
        return true;

    case SCANNED_NOTEMPTY:
        return false;
    }
}

//-----------------------------------------------------------------------
void AutoHandler::EndTag()
{
    if(!ph_->EndTag(false, this))
        prsState_->s_->EndElement();
    prsState_->elemTypeStack_.pop_back();
}

//-----------------------------------------------------------------------
const AttrMap& AutoHandler::GetAttributes() const
{
    if(!hasAttributes_)
    {
        if(state_ != NOT_SCANNED)
            InternalError(__FILE__, __LINE__, "attributes are skipped");

        const Fb2ElementInfo &ei = einfo[type_];
        if(prsState_->s_->BeginElement(ei.name_, &attrmap_))
            state_ = SCANNED_NOTEMPTY;
        else
        {
            if(ei.notempty_)
                EmptyElementError(prsState_->s_, ei.name_);
            state_ = SCANNED_EMPTY;
        }
        hasAttributes_ = true;
    }
    return attrmap_;
}


//-----------------------------------------------------------------------
// Syntax parser
//-----------------------------------------------------------------------
class Fb2ParserImpl : public Fb2Parser, Noncopyable
{
public:
    Fb2ParserImpl(LexScanner *scanner) : state_(scanner) {}

    //virtuals
    void Parse(Fb2Ctxt *ctxt);

private:
    ParserState     state_;

    void AuthorElement          (Fb2EType type, Fb2Ctxt *ctxt);
    void ParseText              (Fb2EType type, Fb2Ctxt *ctxt);
    void SimpleText             (Fb2EType type, Fb2Ctxt *ctxt);
    void TitleInfo              (Fb2EType type, Fb2Ctxt *ctxt);

    // FictionBook elements
    void FictionBook            (Fb2Ctxt *ctxt);
    void a                      (Fb2Ctxt *ctxt);
    void annotation             (Fb2Ctxt *ctxt);
    void author                 (Fb2Ctxt *ctxt);
    void binary                 (Fb2Ctxt *ctxt);
    void body                   (Fb2Ctxt *ctxt);
    void book_name              (Fb2Ctxt *ctxt);
    void book_title             (Fb2Ctxt *ctxt);
    void cite                   (Fb2Ctxt *ctxt);
    void city                   (Fb2Ctxt *ctxt);
    void code                   (Fb2Ctxt *ctxt);
    void coverpage              (Fb2Ctxt *ctxt);
    void custom_info            (Fb2Ctxt *ctxt);
    void date                   (Fb2Ctxt *ctxt);
    void description            (Fb2Ctxt *ctxt);
    void document_info          (Fb2Ctxt *ctxt);
    void email                  (Fb2Ctxt *ctxt);
    void emphasis               (Fb2Ctxt *ctxt);
    void empty_line             (Fb2Ctxt *ctxt);
    void epigraph               (Fb2Ctxt *ctxt);
    void first_name             (Fb2Ctxt *ctxt);
    void genre                  (Fb2Ctxt *ctxt);
    void history                (Fb2Ctxt *ctxt);
    void home_page              (Fb2Ctxt *ctxt);
    void id                     (Fb2Ctxt *ctxt);
    void image                  (Fb2Ctxt *ctxt);    // <image> in <body>, <section> (except top image)
    void image_inline           (Fb2Ctxt *ctxt);    // <image> in <coverpage>, <p>, <a>, <v>, <subtitle>, <th>, <td>, <text-author>
    void image_section_top      (Fb2Ctxt *ctxt);    // <image> in the top of the <section>
    void isbn                   (Fb2Ctxt *ctxt);
    void keywords               (Fb2Ctxt *ctxt);
    void lang                   (Fb2Ctxt *ctxt);
    void last_name              (Fb2Ctxt *ctxt);
    void middle_name            (Fb2Ctxt *ctxt);
    void nickname               (Fb2Ctxt *ctxt);
    //void output_document_class  (Fb2Ctxt *ctxt);
    //void output                 (Fb2Ctxt *ctxt);
    void p                      (Fb2Ctxt *ctxt);
    //void part                   (Fb2Ctxt *ctxt);
    void poem                   (Fb2Ctxt *ctxt);
    void program_used           (Fb2Ctxt *ctxt);
    void publish_info           (Fb2Ctxt *ctxt);
    void publisher_pi           (Fb2Ctxt *ctxt);    // <publisher> in <publish-info>
    void publisher_di           (Fb2Ctxt *ctxt);    // <publisher> in <document-info>
    void section                (Fb2Ctxt *ctxt);
    void sequence               (Fb2Ctxt *ctxt);
    void src_lang               (Fb2Ctxt *ctxt);
    void src_ocr                (Fb2Ctxt *ctxt);
    void src_title_info         (Fb2Ctxt *ctxt);
    void src_url                (Fb2Ctxt *ctxt);
    void stanza                 (Fb2Ctxt *ctxt);
    void strikethrough          (Fb2Ctxt *ctxt);
    void strong                 (Fb2Ctxt *ctxt);
    void style                  (Fb2Ctxt *ctxt);
    //void stylesheet             (Fb2Ctxt *ctxt);
    void sub                    (Fb2Ctxt *ctxt);
    void subtitle               (Fb2Ctxt *ctxt);
    void sup                    (Fb2Ctxt *ctxt);
    void table                  (Fb2Ctxt *ctxt);
    void td                     (Fb2Ctxt *ctxt);
    void text_author            (Fb2Ctxt *ctxt);
    void th                     (Fb2Ctxt *ctxt);
    void title                  (Fb2Ctxt *ctxt);
    void title_info             (Fb2Ctxt *ctxt);
    void tr                     (Fb2Ctxt *ctxt);
    void translator             (Fb2Ctxt *ctxt);
    void v                      (Fb2Ctxt *ctxt);
    void version                (Fb2Ctxt *ctxt);
    void year                   (Fb2Ctxt *ctxt);
};


//-----------------------------------------------------------------------
void Fb2ParserImpl::Parse(Fb2Ctxt *ctxt)
{
    state_.s_->SkipXMLDeclaration();
    try
    {
        FictionBook(ctxt);
    }
    catch(ExitParserException&)
    {
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::AuthorElement(Fb2EType type, Fb2Ctxt *ctxt)
{
    AutoHandler h(type, &state_, ctxt);
    if(h.StartTag())
        return;

    if(state_.s_->IsNextElement("first-name"))
    {
        //<first-name>
        first_name(h.Ctxt());
        //</first-name>

        //<middle-name>
        if(state_.s_->IsNextElement("middle-name"))
            middle_name(h.Ctxt());
        //<middle-name>

        //<last-name>
        last_name(h.Ctxt());
        //</last-name>
    }
    else if(state_.s_->IsNextElement("nickname"))
    {
        //<nickname>
        nickname(h.Ctxt());
        //</nickname>
    }
    else
        state_.s_->Error("<first-name> or <nickname> expected");

    //<home-page>
    while(state_.s_->IsNextElement("home-page"))
        home_page(h.Ctxt());
    //</home-page>

    //<email>
    while(state_.s_->IsNextElement("email"))
        email(h.Ctxt());
    //</email>

    //<id>
    if(state_.s_->IsNextElement("id"))
        id(h.Ctxt());
    //<id>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::ParseText(Fb2EType type, Fb2Ctxt *ctxt)
{
    AutoHandler h(type, &state_, ctxt);
    if(h.StartTag())
        return;

    SetScannerDataMode setDataMode(state_.s_);
    for(;;)
    {
        LexScanner::Token t = state_.s_->LookAhead();
        switch(t.type_)
        {
        default:
            h.EndTag();
            return;

        case LexScanner::DATA:
            state_.s_->GetToken();
            h.Data(t.s_, t.size_);
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <a>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong(h.Ctxt());
            else if(!t.s_.compare("emphasis"))
                emphasis(h.Ctxt());
            else if(!t.s_.compare("style"))
                style(h.Ctxt());
            else if(!t.s_.compare("a"))
                a(h.Ctxt());
            else if(!t.s_.compare("strikethrough"))
                strikethrough(h.Ctxt());
            else if(!t.s_.compare("sub"))
                sub(h.Ctxt());
            else if(!t.s_.compare("sup"))
                sup(h.Ctxt());
            else if(!t.s_.compare("code"))
                code(h.Ctxt());
            else if(!t.s_.compare("image"))
                image_inline(h.Ctxt());
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <" << einfo[type].name_ + ">";
                state_.s_->Error(ss.str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </a>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::SimpleText(Fb2EType type, Fb2Ctxt *ctxt)
{
    AutoHandler h(type, &state_, ctxt);
    if(!h.StartTag())
    {
        SetScannerDataMode setDataMode(state_.s_);
        if(state_.s_->LookAhead().type_ == LexScanner::DATA)
        {
            LexScanner::Token t = state_.s_->GetToken();
            h.Data(t.s_, t.size_);
        }
        h.EndTag();
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::TitleInfo(Fb2EType type, Fb2Ctxt *ctxt)
{
    AutoHandler h(type, &state_, ctxt);
    if(h.StartTag())
        return;

    //<genre>
    do
        genre(h.Ctxt());
    while(state_.s_->IsNextElement("genre"));
    //</genre>

    //<author>
    do
        author(h.Ctxt());
    while(state_.s_->IsNextElement("author"));
    //<author>

    //<book-title>
    book_title(h.Ctxt());
    //</book-title>

    //<annotation>
    if(state_.s_->IsNextElement("annotation"))
        annotation(h.Ctxt());
    //</annotation>

    //<keywords>
    if(state_.s_->IsNextElement("keywords"))
        keywords(h.Ctxt());
    //</keywords>

    //<date>
    if(state_.s_->IsNextElement("date"))
        date(h.Ctxt());
    //<date>

    //<coverpage>
    if(state_.s_->IsNextElement("coverpage"))
        coverpage(h.Ctxt());
    //</coverpage>

    //<lang>
    lang(h.Ctxt());
    //</lang>

    //<src-lang>
    if(state_.s_->IsNextElement("src-lang"))
        src_lang(h.Ctxt());
    //</src-lang>

    //<translator>
    while(state_.s_->IsNextElement("translator"))
        translator(h.Ctxt());
    //</translator>

    //<sequence>
    while(state_.s_->IsNextElement("sequence"))
        sequence(h.Ctxt());
    //</sequence>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::FictionBook(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_FICTIONBOOK, &state_, ctxt);
    if(h.StartTag())
        return;

    //<stylesheet>
    state_.s_->SkipAll("stylesheet");
    //</stylesheet>

    //<description>
    description(h.Ctxt());
    //</description>

    //<body>
    body(h.Ctxt());
    if(state_.s_->IsNextElement("body"))
        body(h.Ctxt());
    if(state_.s_->IsNextElement("body"))
        body(h.Ctxt());
    //</body>

    //<binary>
    while(state_.s_->IsNextElement("binary"))
        binary(h.Ctxt());
    //</binary>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::a(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_A, &state_, ctxt);
    if(h.StartTag())
        return;

    SetScannerDataMode setDataMode(state_.s_);
    for(;;)
    {
        LexScanner::Token t = state_.s_->LookAhead();
        switch(t.type_)
        {
        default:
            h.EndTag();
            return;

        case LexScanner::DATA:
            state_.s_->GetToken();
            h.Data(t.s_, t.size_);
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong(h.Ctxt());
            else if(!t.s_.compare("emphasis"))
                emphasis(h.Ctxt());
            else if(!t.s_.compare("style"))
                style(h.Ctxt());
            else if(!t.s_.compare("strikethrough"))
                strikethrough(h.Ctxt());
            else if(!t.s_.compare("sub"))
                sub(h.Ctxt());
            else if(!t.s_.compare("sup"))
                sup(h.Ctxt());
            else if(!t.s_.compare("code"))
                code(h.Ctxt());
            else if(!t.s_.compare("image"))
                image_inline(h.Ctxt());
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <a>";
                state_.s_->Error(ss.str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::annotation(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_ANNOTATION, &state_, ctxt);
    if(h.StartTag())
        return;

    for(LexScanner::Token t = state_.s_->LookAhead(); t.type_ == LexScanner::START; t = state_.s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <subtitle>, <empty-line>, <table>
        if(!t.s_.compare("p"))
            p(h.Ctxt());
        else if(!t.s_.compare("poem"))
            poem(h.Ctxt());
        else if(!t.s_.compare("cite"))
            cite(h.Ctxt());
        else if(!t.s_.compare("subtitle"))
            subtitle(h.Ctxt());
        else if(!t.s_.compare("empty-line"))
            empty_line(h.Ctxt());
        else if(!t.s_.compare("table"))
            table(h.Ctxt());
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <annotation>";
            state_.s_->Error(ss.str());
        }
        //</p>, </poem>, </cite>, </subtitle>, </empty-line>, </table>
    }

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::author(Fb2Ctxt *ctxt)
{
    AuthorElement(E_AUTHOR, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::binary(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_BINARY, &state_, ctxt);
    if(h.StartTag())
        return;

    SetScannerDataMode setDataMode(state_.s_);
    LexScanner::Token t = state_.s_->GetToken();
    if(t.type_ != LexScanner::DATA)
        state_.s_->Error("<binary> data expected");
    h.Data(t.s_, t.size_);

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::body(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_BODY, &state_, ctxt);
    if(h.StartTag())
        return;

    //<image>
    if(state_.s_->IsNextElement("image"))
        image(h.Ctxt());
    //</image>

    //<title>
    if(state_.s_->IsNextElement("title"))
        title(h.Ctxt());
    //</title>

    //<epigraph>
    while(state_.s_->IsNextElement("epigraph"))
        epigraph(h.Ctxt());
    //</epigraph>

    do
    {
        //<section>
        section(h.Ctxt());
        //</section>
    }
    while(state_.s_->IsNextElement("section"));

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::book_name(Fb2Ctxt *ctxt)
{
    SimpleText(E_BOOK_NAME, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::book_title(Fb2Ctxt *ctxt)
{
    SimpleText(E_BOOK_TITLE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::city(Fb2Ctxt *ctxt)
{
    SimpleText(E_CITY, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::cite(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_CITE, &state_, ctxt);
    if(h.StartTag())
        return;

    for(LexScanner::Token t = state_.s_->LookAhead(); t.type_ == LexScanner::START; t = state_.s_->LookAhead())
    {
        //<p>, <subtitle>, <empty-line>, <poem>, <table>
        if(!t.s_.compare("p"))
            p(h.Ctxt());
        else if(!t.s_.compare("subtitle"))
            subtitle(h.Ctxt());
        else if(!t.s_.compare("empty-line"))
            empty_line(h.Ctxt());
        else if(!t.s_.compare("poem"))
            poem(h.Ctxt());
        else if(!t.s_.compare("table"))
            table(h.Ctxt());
        else if(!t.s_.compare("text-author"))
            break;
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <cite>";
            state_.s_->Error(ss.str());
        }
        //</p>, </subtitle>, </empty-line>, </poem>, </table>
    }

    //<text-author>
    while(state_.s_->IsNextElement("text-author"))
        text_author(h.Ctxt());
    //</text-author>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::code(Fb2Ctxt *ctxt)
{
    ParseText(E_CODE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::custom_info(Fb2Ctxt *ctxt)
{
    SimpleText(E_CUSTOM_INFO, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::coverpage(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_COVERPAGE, &state_, ctxt);
    if(h.StartTag())
        return;

    do
        image_inline(h.Ctxt());
    while(state_.s_->IsNextElement("image"));

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::date(Fb2Ctxt *ctxt)
{
    SimpleText(E_DATE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::description(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_DESCRIPTION, &state_, ctxt);
    if(h.StartTag())
        return;

    //<title-info>
    title_info(h.Ctxt());
    //</title-info>

    //<src-title-info>
    if(state_.s_->IsNextElement("src-title-info"))
        src_title_info(h.Ctxt());
    //</src-title-info>

    //<document-info>
    document_info(h.Ctxt());
    //</document-info>

    //<publish-info>
    if(state_.s_->IsNextElement("publish-info"))
        publish_info(h.Ctxt());
    //</publish-info>

    //<custom-info>
    while(state_.s_->IsNextElement("custom-info"))
        custom_info(h.Ctxt());
    //</custom-info>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::email(Fb2Ctxt *ctxt)
{
    SimpleText(E_EMAIL, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::document_info(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_DOCUMENT_INFO, &state_, ctxt);
    if(h.StartTag())
        return;

    //<author>
    do
        author(h.Ctxt());
    while(state_.s_->IsNextElement("author"));
    //</author>

    //<program-used>
    if(state_.s_->IsNextElement("program-used"))
        program_used(h.Ctxt());
    //</program-used>

    //<date>
    date(h.Ctxt());
    //</date>

    //<src-url>
    while(state_.s_->IsNextElement("src-url"))
        src_url(h.Ctxt());
    //</src-url>

    //<src-ocr>
    if(state_.s_->IsNextElement("src-ocr"))
        src_ocr(h.Ctxt());
    //</src-ocr>

    //<id>
    id(h.Ctxt());
    //<id>

    //<version>
    version(h.Ctxt());
    //</version>

    //<history>
    if(state_.s_->IsNextElement("history"))
        history(h.Ctxt());
    //</history>

    //<publisher>
    while(state_.s_->IsNextElement("publisher"))
        publisher_di(h.Ctxt());
    //</publisher>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::emphasis(Fb2Ctxt *ctxt)
{
    ParseText(E_EMPHASIS, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::empty_line(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_EMPTY_LINE, &state_, ctxt);
    if(!h.StartTag())
        h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::epigraph(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_EPIGRAPH, &state_, ctxt);
    if(h.StartTag())
        return;

    for(LexScanner::Token t = state_.s_->LookAhead(); t.type_ == LexScanner::START; t = state_.s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <empty-line>
        if(!t.s_.compare("p"))
            p(h.Ctxt());
        else if(!t.s_.compare("poem"))
            poem(h.Ctxt());
        else if(!t.s_.compare("cite"))
            cite(h.Ctxt());
        else if(!t.s_.compare("empty-line"))
            empty_line(h.Ctxt());
        else if(!t.s_.compare("text-author"))
            break;
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <epigraph>";
            state_.s_->Error(ss.str());
        }
        //</p>, </poem>, </cite>, </empty-line>
    }

    //<text-author>
    while(state_.s_->IsNextElement("text-author"))
        text_author(h.Ctxt());
    //</text-author>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::first_name(Fb2Ctxt *ctxt)
{
    SimpleText(E_FIRST_NAME, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::genre(Fb2Ctxt *ctxt)
{
    SimpleText(E_GENRE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::history(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_HISTORY, &state_, ctxt);
    if(h.StartTag())
        return;

    for(LexScanner::Token t = state_.s_->LookAhead(); t.type_ == LexScanner::START; t = state_.s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <subtitle>, <empty-line>, <table>
        if(!t.s_.compare("p"))
            p(h.Ctxt());
        else if(!t.s_.compare("poem"))
            poem(h.Ctxt());
        else if(!t.s_.compare("cite"))
            cite(h.Ctxt());
        else if(!t.s_.compare("subtitle"))
            subtitle(h.Ctxt());
        else if(!t.s_.compare("empty-line"))
            empty_line(h.Ctxt());
        else if(!t.s_.compare("table"))
            table(h.Ctxt());
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <history>";
            state_.s_->Error(ss.str());
        }
        //</p>, </poem>, </cite>, </subtitle>, </empty-line>, </table>
    }

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::home_page(Fb2Ctxt *ctxt)
{
    SimpleText(E_HOME_PAGE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::id(Fb2Ctxt *ctxt)
{
    SimpleText(E_ID, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::image(Fb2Ctxt *ctxt)
{
    ClrScannerDataMode clrDataMode(state_.s_);
    AutoHandler h(E_IMAGE, &state_, ctxt);
    if(!h.StartTag())
        h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::image_inline(Fb2Ctxt *ctxt)
{
    ClrScannerDataMode clrDataMode(state_.s_);
    AutoHandler h(E_IMAGE_INLINE, &state_, ctxt);
    if(!h.StartTag())
        h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::image_section_top(Fb2Ctxt *ctxt)
{
    ClrScannerDataMode clrDataMode(state_.s_);
    AutoHandler h(E_IMAGE_SECTION_TOP, &state_, ctxt);
    if(!h.StartTag())
        h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::isbn(Fb2Ctxt *ctxt)
{
    SimpleText(E_ISBN, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::keywords(Fb2Ctxt *ctxt)
{
    SimpleText(E_KEYWORDS, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::last_name(Fb2Ctxt *ctxt)
{
    SimpleText(E_LAST_NAME, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::middle_name(Fb2Ctxt *ctxt)
{
    SimpleText(E_MIDDLE_NAME, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::nickname(Fb2Ctxt *ctxt)
{
    SimpleText(E_NICKNAME, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::lang(Fb2Ctxt *ctxt)
{
    SimpleText(E_LANG, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::p(Fb2Ctxt *ctxt)
{
    ParseText(E_P, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::poem(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_POEM, &state_, ctxt);
    if(h.StartTag())
        return;

    //<title>
    if(state_.s_->IsNextElement("title"))
        title(h.Ctxt());
    //</title>

    //<epigraph>
    while(state_.s_->IsNextElement("epigraph"))
        epigraph(h.Ctxt());
    //</epigraph>

    //<stanza>
    do
        stanza(h.Ctxt());
    while(state_.s_->IsNextElement("stanza"));
    //</stanza>

    //<text-author>
    while(state_.s_->IsNextElement("text-author"))
        text_author(h.Ctxt());
    //</text-author>

    //<data>
    if(state_.s_->IsNextElement("date"))
        date(h.Ctxt());
    //</data>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::program_used(Fb2Ctxt *ctxt)
{
    SimpleText(E_PROGRAM_USED, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::publish_info(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_PUBLISH_INFO, &state_, ctxt);
    if(h.StartTag())
        return;

    //<book-name>
    if(state_.s_->IsNextElement("book-name"))
        book_name(h.Ctxt());
    //</book-name>

    //<publisher>
    if(state_.s_->IsNextElement("publisher"))
        publisher_pi(h.Ctxt());
    //</publisher>

    //<city>
    if(state_.s_->IsNextElement("city"))
        city(h.Ctxt());
    //</city>

    //<year>
    if(state_.s_->IsNextElement("year"))
        year(h.Ctxt());
    //</year>

    //<isbn>
    if(state_.s_->IsNextElement("isbn"))
        isbn(h.Ctxt());
    //</isbn>

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::publisher_pi(Fb2Ctxt *ctxt)
{
    SimpleText(E_PUBLISHER_PI, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::publisher_di(Fb2Ctxt *ctxt)
{
    AuthorElement(E_PUBLISHER_DI, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::section(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_SECTION, &state_, ctxt);
    if(h.StartTag())
        return;

    //<title>
    if(state_.s_->IsNextElement("title"))
        title(h.Ctxt());
    //</title>

    //<epigraph>
    while(state_.s_->IsNextElement("epigraph"))
        epigraph(h.Ctxt());
    //</epigraph>

    //<image>
    if(state_.s_->IsNextElement("image"))
        image_section_top(h.Ctxt());
    //</image>

    //<annotation>
    if(state_.s_->IsNextElement("annotation"))
        annotation(h.Ctxt());
    //</annotation>

    if(state_.s_->IsNextElement("section"))
        do
        {
            //<section>
            section(h.Ctxt());
            //</section>
        }
        while(state_.s_->IsNextElement("section"));
    else
    {
        for(LexScanner::Token t = state_.s_->LookAhead(); t.type_ == LexScanner::START; t = state_.s_->LookAhead())
        {
            //<p>, <image>, <poem>, <subtitle>, <cite>, <empty-line>, <table>
            if(!t.s_.compare("p"))
                p(h.Ctxt());
            else if(!t.s_.compare("image"))
                image(h.Ctxt());
            else if(!t.s_.compare("poem"))
                poem(h.Ctxt());
            else if(!t.s_.compare("subtitle"))
                subtitle(h.Ctxt());
            else if(!t.s_.compare("cite"))
                cite(h.Ctxt());
            else if(!t.s_.compare("empty-line"))
                empty_line(h.Ctxt());
            else if(!t.s_.compare("table"))
                table(h.Ctxt());
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <section>";
                state_.s_->Error(ss.str());
            }
            //</p>, </image>, </poem>, </subtitle>, </cite>, </empty-line>, </table>
        }
    }

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sequence(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_SEQUENCE, &state_, ctxt);
    if(!h.StartTag())
        h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::src_lang(Fb2Ctxt *ctxt)
{
    SimpleText(E_SRC_LANG, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::src_ocr(Fb2Ctxt *ctxt)
{
    SimpleText(E_SRC_OCR, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::src_title_info(Fb2Ctxt *ctxt)
{
    TitleInfo(E_SRC_TITLE_INFO, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::src_url(Fb2Ctxt *ctxt)
{
    SimpleText(E_SRC_URL, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::stanza(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_STANZA, &state_, ctxt);
    if(h.StartTag())
        return;

    //<title>
    if(state_.s_->IsNextElement("title"))
        title(h.Ctxt());
    //</title>

    //<subtitle>
    if(state_.s_->IsNextElement("subtitle"))
        subtitle(h.Ctxt());
    //</subtitle>

    do
    {
        //<v>
        v(h.Ctxt());
        //</v>
    }
    while(state_.s_->IsNextElement("v"));

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strikethrough(Fb2Ctxt *ctxt)
{
    ParseText(E_STRIKETHROUGH, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strong(Fb2Ctxt *ctxt)
{
    ParseText(E_STRONG, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::style(Fb2Ctxt *ctxt)
{
    ParseText(E_STYLE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sub(Fb2Ctxt *ctxt)
{
    ParseText(E_SUB, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::subtitle(Fb2Ctxt *ctxt)
{
    ParseText(E_SUBTITLE, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sup(Fb2Ctxt *ctxt)
{
    ParseText(E_SUP, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::table(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_TABLE, &state_, ctxt);
    if(h.StartTag())
        return;

    do
    {
        //<tr>
        tr(h.Ctxt());
        //</tr>
    }
    while(state_.s_->IsNextElement("tr"));

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::td(Fb2Ctxt *ctxt)
{
    ParseText(E_TD, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::text_author(Fb2Ctxt *ctxt)
{
    ParseText(E_TEXT_AUTHOR, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::th(Fb2Ctxt *ctxt)
{
    ParseText(E_TH, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_TITLE, &state_, ctxt);
    if(h.StartTag())
        return;

    for(LexScanner::Token t = state_.s_->LookAhead(); t.type_ == LexScanner::START; t = state_.s_->LookAhead())
    {
        if(!t.s_.compare("p"))
        {
            //<p>
            p(h.Ctxt());
            //</p>
        }
        else if(!t.s_.compare("empty-line"))
        {
            //<empty-line>
            empty_line(h.Ctxt());
            //</empty-line>
        }
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <title>";
            state_.s_->Error(ss.str());
        }
    }

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title_info(Fb2Ctxt *ctxt)
{
    TitleInfo(E_TITLE_INFO, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::tr(Fb2Ctxt *ctxt)
{
    AutoHandler h(E_TR, &state_, ctxt);
    if(h.StartTag())
        return;

    for(;;)
    {
        //<th>, <td>
        if(state_.s_->IsNextElement("th"))
            th(h.Ctxt());
        else if(state_.s_->IsNextElement("td"))
            td(h.Ctxt());
        else
            break;
        //</th>, </td>
    }

    h.EndTag();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::translator(Fb2Ctxt *ctxt)
{
    AuthorElement(E_TRANSLATOR, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::v(Fb2Ctxt *ctxt)
{
    ParseText(E_V, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::version(Fb2Ctxt *ctxt)
{
    SimpleText(E_VERSION, ctxt);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::year(Fb2Ctxt *ctxt)
{
    SimpleText(E_YEAR, ctxt);
}

//-----------------------------------------------------------------------
Ptr<Fb2Parser> FB2TOEPUB_DECL CreateFb2Parser(LexScanner *scanner)
{
    return new Fb2ParserImpl(scanner);
}



//-----------------------------------------------------------------------
// Helper classes implementation
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
class Fb2StdCtxtImpl : public Fb2StdCtxt
{
    struct Entry
    {
        Ptr<Fb2EHandler>        h_;
        Ptr<Fb2Ctxt>            ctxt_;

        Entry() {}
        Entry(Fb2EHandler *h) : h_(h) {}
    };
    std::vector<Entry> entries_;

    Fb2StdCtxtImpl() : entries_(E_COUNT, Entry(CreateRecursiveEHandler())) {}

public:
    //virtual
    void GetNext(Fb2EType type, Ptr<Fb2EHandler> *h, Ptr<Fb2Ctxt> *ctxt)
    {
        if(h)
            *h = entries_[type].h_;
        if(ctxt)
        {
            Ptr<Fb2Ctxt> ret = entries_[type].ctxt_;
            if(ret)
                *ctxt = ret;
            else
                *ctxt = this;
        }
    }
    void RegisterCtxt(Fb2EType type, Fb2Ctxt *ctxt)
    {
        size_t idx = type;
#if defined(_DEBUG)
        if(idx >= E_COUNT)
        {
            std::ostringstream ss;
            ss << "Bad index " << idx;
            InternalError(__FILE__, __LINE__, ss.str());
        }
#endif
        if(ctxt == this)
            entries_[idx].ctxt_ = NULL;
        else
            entries_[idx].ctxt_ = ctxt;
    }
    void RegisterHandler(Fb2EType type, Fb2EHandler *h)
    {
        size_t idx = type;
#if defined(_DEBUG)
        if(idx >= E_COUNT)
        {
            std::ostringstream ss;
            ss << "Bad index " << idx;
            InternalError(__FILE__, __LINE__, ss.str());
        }
#endif
        entries_[idx].h_ = h;
    }

    static Fb2StdCtxt* Obj()
    {
        static Ptr<Fb2StdCtxt> obj_ = new Fb2StdCtxtImpl();
        return obj_;
    }
};


//-----------------------------------------------------------------------
Ptr<Fb2StdCtxt> FB2TOEPUB_DECL CreateFb2StdCtxt()
{
    return Fb2StdCtxtImpl::Obj();
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
const Fb2ElementInfo* FB2TOEPUB_DECL Fb2GetElementInfo(Fb2EType type)
{
#if defined(_DEBUG)
    if(type < 0 || type >= E_COUNT)
    {
        std::ostringstream ss;
        ss << "Bad index " << int(type);
        InternalError(__FILE__, __LINE__, ss.str());
    }
#endif
    return &einfo[type];
}


//-----------------------------------------------------------------------
// RECURSIVE HANDLER
//-----------------------------------------------------------------------
class RecursiveEHandler : public Fb2BaseEHandler<true>
{
public:
    static Fb2EHandler* Obj()
    {
        static Ptr<Fb2EHandler> obj_ = new RecursiveEHandler();
        return obj_;
    }
};

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateRecursiveEHandler()
{
    return RecursiveEHandler::Obj();
}


//-----------------------------------------------------------------------
// SKIP HANDLER
//-----------------------------------------------------------------------
class SkipEHandler : public Fb2BaseEHandler<>
{
public:
    //virtuals
    bool StartTag(Fb2EType, Fb2Host *host)
    {
        host->Scanner()->SkipElement();
        return true;
    }

    static Fb2EHandler* Obj()
    {
        static Ptr<Fb2EHandler> obj_ = new SkipEHandler();
        return obj_;
    }
};

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateSkipEHandler()
{
    return SkipEHandler::Obj();
}


//-----------------------------------------------------------------------
// NOP HANDLER
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
class ExitEHandler : public Fb2BaseEHandler<>
{
public:
    //virtuals
    bool StartTag(Fb2EType, Fb2Host *host)
    {
        host->Exit();
        return false;
    }

    static Fb2EHandler* Obj()
    {
        static Ptr<Fb2EHandler> obj_ = new ExitEHandler();
        return obj_;
    }
};

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateExitEHandler()
{
    return ExitEHandler::Obj();
}


//-----------------------------------------------------------------------
// SIMPLE TEXT HANDLER
//-----------------------------------------------------------------------
class TextHandlerBase : public Fb2TextHandler
{
public:
    //virtuals
    bool StartTag   (Fb2EType, Fb2Host*)    {return false;}
    bool EndTag     (bool, Fb2Host*)        {return false;}
};
//-----------------------------------------------------------------------
class TextHandler : public TextHandlerBase
{
public:
    TextHandler(String *text) : text_(text ? text : &buf_) {}

    //virtuals
    void Data(const String &data, size_t)   {*text_ += data;}
    void Reset()                            {*text_ = "";}
    const String& Text() const              {return *text_;}

private:
    String buf_, *text_;
};
//-----------------------------------------------------------------------
class TextHandlerConcat : public TextHandlerBase
{
public:
    TextHandlerConcat(const String &concatDivider, String *text)
        : divider_(concatDivider), text_(text ? text : &buf_) {}

    //virtuals
    void Data(const String &data, size_t)   {*text_ = Concat(*text_, divider_, data);}
    void Reset()                            {*text_ = "";}
    const String& Text() const              {return *text_;}

private:
    String divider_;
    String buf_, *text_;
};
//-----------------------------------------------------------------------
Ptr<Fb2TextHandler> FB2TOEPUB_DECL CreateTextEHandler(const String &concatDivider, String *text)
{
    if(concatDivider.empty())
        return new TextHandler(text);
    else
        return new TextHandlerConcat(concatDivider, text);
}
Ptr<Fb2TextHandler> FB2TOEPUB_DECL CreateTextEHandler(String *text)
{
    return new TextHandler(text);
}


//-----------------------------------------------------------------------
// ROOT ELEMENT HANDLER
//-----------------------------------------------------------------------
class FictionBoolElement : public Fb2BaseEHandler<true>
{
public:
    //virtuals
    bool StartTag(Fb2EType, Fb2Host *host)
    {
        host->RegisterNsLookup(Ptr<Lookup>(new Lookup(host)));
        return false;
    }

private:
    class Lookup : public Fb2NsLookup
    {
    public:
        Lookup(Fb2Host *host)
        {
            const AttrMap &attrmap = host->GetAttributes();

            // namespaces
            AttrMap::const_iterator cit = attrmap.begin(), cit_end = attrmap.end();
            bool has_fb = false, has_emptyfb = false;
            for(; cit != cit_end; ++cit)
            {
                static const String xmlns = "xmlns";
                static const std::size_t xmlns_len = xmlns.length();
                static const String fbID = "http://www.gribuser.ru/xml/fictionbook/2.0",
                                    xlID = "http://www.w3.org/1999/xlink";

                if(!cit->second.compare(fbID))
                {
                    if(!cit->first.compare(xmlns))
                        has_emptyfb = true;
                    else if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                        host->Error("bad FictionBook namespace definition");
                    has_fb = true;
                }
                else if(!cit->second.compare(xlID))
                {
                    if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                        host->Error("bad xlink namespace definition");
                    xlns_.insert(cit->first.substr(xmlns_len+1));
                }
            }
            if(!has_fb)
                host->Error("missing FictionBook namespace definition");
            if(!has_emptyfb)
                host->Error("non-empty FictionBook namespace not implemented");
        }

        //virtual
        String Findhref(const AttrMap &attrmap) const
        {
            std::set<String>::const_iterator cit = xlns_.begin(), cit_end = xlns_.end();
            for(; cit != cit_end; ++cit)
            {
                String href;
                if(cit->empty())
                    href = "href";
                else
                    href = (*cit)+":href";
                AttrMap::const_iterator ait = attrmap.find(href);
                if(ait != attrmap.end())
                    return ait->second;
            }
            return "";
        }

    private:
        std::set<String> xlns_; // xlink namespaces
    };
};
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateRootEHandler()
{
    return new FictionBoolElement();
}




};  //namespace Fb2ToEpub
