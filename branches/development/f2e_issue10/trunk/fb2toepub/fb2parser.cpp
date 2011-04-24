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

struct ElementInfo
{
    String  name_;
    bool    notempty_;

    ElementInfo() {}
    ElementInfo(const String &name, bool notempty) : name_(name), notempty_(notempty) {}
};

static const ElementInfo einfo[E_COUNT] =
{
    ElementInfo("FictionBook",          true),
    ElementInfo("a",                    false),
    ElementInfo("annotation",           false),
    ElementInfo("author",               false),
    ElementInfo("binary",               true),
    ElementInfo("body",                 false),
    ElementInfo("book-name",            false),
    ElementInfo("book-title",           false),
    ElementInfo("cite",                 true),
    ElementInfo("city",                 false),
    ElementInfo("code",                 false),
    ElementInfo("coverpage",            false),
    ElementInfo("custom-info",          false),
    ElementInfo("date",                 false),
    ElementInfo("description",          true),
    ElementInfo("document-info",        false),
    ElementInfo("email",                false),
    ElementInfo("emphasis",             false),
    ElementInfo("empty-line",           false),
    ElementInfo("epigraph",             false),
    ElementInfo("first-name",           false),
    ElementInfo("genre",                false),
    ElementInfo("history",              false),
    ElementInfo("home-page",            false),
    ElementInfo("id",                   false),
    ElementInfo("isbn",                 false),
    ElementInfo("image",                false),     // <image> in <body>, <section> (except top image)
    ElementInfo("image",                false),     // <image> in all others (inline)
    ElementInfo("image",                false),     // <image> in the top of the <section>
    ElementInfo("keywords",             false),
    ElementInfo("lang",                 false),
    ElementInfo("last-name",            false),
    ElementInfo("middle-name",          false),
    ElementInfo("nickname",             false),
    ElementInfo("output-document-class",false),
    ElementInfo("output",               false),
    ElementInfo("p",                    false),
    ElementInfo("part",                 false),
    ElementInfo("poem",                 false),
    ElementInfo("program-used",         false),
    ElementInfo("publish-info",         false),
    ElementInfo("publisher",            false),     // <publisher> in <publish-info>
    ElementInfo("publisher",            false),     // <publisher> in <document-info>
    ElementInfo("section",              false),
    ElementInfo("sequence",             false),
    ElementInfo("src-lang",             false),
    ElementInfo("src-ocr",              false),
    ElementInfo("src-title-info",       false),
    ElementInfo("src-url",              false),
    ElementInfo("stanza",               false),
    ElementInfo("strikethrough",        false),
    ElementInfo("strong",               false),
    ElementInfo("style",                false),
    ElementInfo("stylesheet",           false),
    ElementInfo("sub",                  false),
    ElementInfo("subtitle",             false),
    ElementInfo("sup",                  false),
    ElementInfo("table",                false),
    ElementInfo("td",                   false),
    ElementInfo("text-author",          false),
    ElementInfo("th",                   false),
    ElementInfo("title",                false),
    ElementInfo("title-info",           true),
    ElementInfo("tr",                   false),
    ElementInfo("translator",           false),
    ElementInfo("v",                    false),
    ElementInfo("version",              false),
    ElementInfo("year",                 false)
};

//-----------------------------------------------------------------------
inline void EmptyElementError(LexScanner *s, const String &name)
{
    std::ostringstream ss;
    ss << "element <" << name << "> can't be empty";
    s->Error(ss.str());
}


//-----------------------------------------------------------------------
typedef std::vector<Ptr<Fb2EHandler> > HandlerVector;
typedef std::vector<Fb2EType> TypeVector;

//-----------------------------------------------------------------------
// Dummy namespace lookup
//-----------------------------------------------------------------------
class DummyNsLookup : public Fb2NsLookup
{
public:
    //virtual
    String Findhref(const AttrMap &attrmap) const
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
// Auto handler
//-----------------------------------------------------------------------
class AutoHandler : private Fb2Host, Noncopyable
{
    Fb2EType            type_;
    ParserState         *state_;
    Ptr<Fb2EHandler>    ph_;
    Ptr<Fb2Ctxt>        newCtxt_;
public:
    AutoHandler(Fb2EType type, ParserState *state, Fb2Ctxt *ctxt)
        :   type_(type),
            state_(state)
    {
        ph_ = ctxt->GetHandler(type);   // get handler from old context
        newCtxt_ = ph_->GetCtxt(ctxt);  // get new context from handler
    }

    bool StartTag()
    {
        if (ph_->StartTag(type_, state_->s_, this))
            return true;
        state_->elemTypeStack_.push_back(type_);
        return false;
    }
    Fb2Ctxt* Ctxt() const
    {
        return newCtxt_;
    }
    void Data(const String &data)
    {
        ph_->Data(data);
    }
    void EndTag()
    {
        state_->elemTypeStack_.pop_back();
        ph_->EndTag(state_->s_);
    }

    //virtual 
    LexScanner* Scanner() const                         {return state_->s_;}
    size_t      GetTypeStackSize() const                {return state_->elemTypeStack_.size();}
    Fb2EType    GetTypeStackAt(int i) const             {return state_->elemTypeStack_[i];}
    String      Findhref(const AttrMap &attrmap) const  {return state_->nsLookup_->Findhref(attrmap);}

    //virtual
    void RegisterNsLookup(Fb2NsLookup *lookup)
    {
        state_->nsLookup_ = lookup;
    }
};

//-----------------------------------------------------------------------
// Syntax parser
//-----------------------------------------------------------------------
class Fb2ParserImpl : public Fb2Parser, Noncopyable
{
public:
    Fb2ParserImpl(LexScanner *scanner, Fb2EHandler *defHandler) : state_(scanner), ctxt_(new Ctxt(defHandler)) {}

    //virtuals
    Ptr<Fb2Ctxt> GetDefaultCtxt() const             {return ctxt_;}
    void Register(Fb2EType type, Fb2EHandler *h)    {ctxt_->Register(type, h);}
    void Parse();

private:
    class Ctxt : public Fb2Ctxt
    {
        HandlerVector   handlers_;
    public:
        Ctxt(Fb2EHandler *defHandler)
            : handlers_(E_COUNT, defHandler) {}

        //virtual
        Ptr<Fb2EHandler> GetHandler(Fb2EType type) const    {return handlers_[type];}
        void Register(Fb2EType type, Fb2EHandler *h);
    };

    ParserState     state_;
    Ptr<Ctxt>       ctxt_;

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
void Fb2ParserImpl::Ctxt::Register(Fb2EType type, Fb2EHandler *h)
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
    handlers_[idx] = h;
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::Parse()
{
    state_.s_->SkipXMLDeclaration();
    FictionBook(ctxt_);
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
            h.Data(state_.s_->GetToken().s_);
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
            h.Data(state_.s_->GetToken().s_);
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
            h.Data(state_.s_->GetToken().s_);
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
    h.Data(t.s_);

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
Ptr<Fb2Parser> FB2TOEPUB_DECL CreateFb2Parser(LexScanner *scanner, Fb2EHandler *defHandler)
{
    return new Fb2ParserImpl(scanner, defHandler);
}



//-----------------------------------------------------------------------
// Helper classes implementation
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
const String& FB2TOEPUB_DECL Fb2EName(Fb2EType type)
{
    return einfo[type].name_;
}

//-----------------------------------------------------------------------
// RECURSIVE HANDLER
//-----------------------------------------------------------------------
class RecursiveEHandler : public Fb2EHandler
{
public:
    //virtuals
    bool StartTag(Fb2EType type, LexScanner *s, Fb2Host *host)
    {
        const ElementInfo &ei = einfo[type];
        if(s->BeginElement(ei.name_))
            return false;

        if(ei.notempty_)
            EmptyElementError(s, ei.name_);
        return true;
    }
    Ptr<Fb2Ctxt>    GetCtxt(Fb2Ctxt *oldCtxt)   {return oldCtxt;}
    void            Data(const String&)         {}
    void            EndTag(LexScanner *s)       {s->SkipRestOfElementContent();}

    static Fb2EHandler* Obj()                       {return obj_;}

private:
    static Ptr<Fb2EHandler> obj_;
};
Ptr<Fb2EHandler> RecursiveEHandler::obj_ = new RecursiveEHandler();

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateRecursiveEHandler()
{
    return RecursiveEHandler::Obj();
}


//-----------------------------------------------------------------------
// SKIP HANDLER
//-----------------------------------------------------------------------
class SkipEHandler : public Fb2EHandler
{
public:
    //virtuals
    bool            StartTag(Fb2EType, LexScanner *s, Fb2Host*) {s->SkipElement(); return true;}
    Ptr<Fb2Ctxt>    GetCtxt (Fb2Ctxt *oldCtxt)                  {return oldCtxt;}
    void            Data    (const String&)                     {}
    void            EndTag  (LexScanner*)                       {}

    static Fb2EHandler* Obj()                                   {return obj_;}

private:
    static Ptr<Fb2EHandler> obj_;
};
Ptr<Fb2EHandler> SkipEHandler::obj_ = new SkipEHandler();

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateSkipEHandler()
{
    return SkipEHandler::Obj();
}


//-----------------------------------------------------------------------
// NOP HANDLER
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
class NopEHandler : public Fb2EHandler
{
public:
    //virtuals
    bool            StartTag(Fb2EType, LexScanner*, Fb2Host*)   {return true;}
    Ptr<Fb2Ctxt>    GetCtxt(Fb2Ctxt *oldCtxt)                   {return oldCtxt;}
    void            Data    (const String&)                     {}
    void            EndTag  (LexScanner*)                       {}

    static Fb2EHandler* Obj()                                   {return obj_;}

private:
    static Ptr<Fb2EHandler> obj_;
};
Ptr<Fb2EHandler> NopEHandler::obj_ = new NopEHandler();

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateNopEHandler()
{
    return NopEHandler::Obj();
}


//-----------------------------------------------------------------------
// SIMPLE TEXT HANDLER
//-----------------------------------------------------------------------
class TextHandlerBase : public Fb2TextHandler
{
public:
    //virtuals
    bool StartTag(Fb2EType type, LexScanner *s, Fb2Host*)
    {
        const ElementInfo &ei = einfo[type];
        if(s->BeginElement(ei.name_))
            return false;

        if(ei.notempty_)
            EmptyElementError(s, ei.name_);
        return true;
    }
    Ptr<Fb2Ctxt> GetCtxt (Fb2Ctxt *oldCtxt)
    {
        return oldCtxt;
    }
    void EndTag(LexScanner *s)
    {
        s->EndElement();
    }
};
//-----------------------------------------------------------------------
class TextHandler : public TextHandlerBase
{
public:
    TextHandler(String *text) : text_(text ? text : &buf_) {}

    //virtuals
    void Data(const String &data)       {*text_ += data;}
    void Reset()                        {*text_ = "";}
    const String& Text() const          {return *text_;}

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
    void Data(const String &data)       {*text_ = Concat(*text_, divider_, data);}
    void Reset()                        {*text_ = "";}
    const String& Text() const          {return *text_;}

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
template<class EE> class EHandlerAttr : public Fb2EHandler, Noncopyable
{
    Ptr<Fb2AttrHandler> pah_;
public:
    EHandlerAttr(Fb2AttrHandler *pah) : pah_(pah) {}

    //virtuals
    bool StartTag(Fb2EType type, LexScanner *s, Fb2Host *host)
    {
        const ElementInfo &ei = einfo[type];
        AttrMap attrmap;
        if(s->BeginElement(ei.name_, &attrmap))
        {
            pah_->Begin(type, attrmap, host);
            return false;
        }

        if(ei.notempty_)
            EmptyElementError(s, ei.name_);
        pah_->Begin(type, attrmap, host);
        pah_->End();
        return true;
    }
    Ptr<Fb2Ctxt> GetCtxt(Fb2Ctxt *oldCtxt)
    {
        return pah_->GetCtxt(oldCtxt);
    }
    void Data(const String &data)
    {
        pah_->Contents(data);
    }
    void EndTag(LexScanner *s)
    {
        pah_->End();
        EE::End(s);
    }
};

//-----------------------------------------------------------------------
template<class EE> class EHandlerNoAttr : public Fb2EHandler, Noncopyable
{
    Ptr<Fb2NoAttrHandler> pah_;
public:
    EHandlerNoAttr(Fb2NoAttrHandler *pah) : pah_(pah) {}

    //virtuals
    bool StartTag(Fb2EType type, LexScanner *s, Fb2Host *host)
    {
        const ElementInfo &ei = einfo[type];
        if(s->BeginElement(ei.name_))
        {
            pah_->Begin(type, host);
            return false;
        }

        if(ei.notempty_)
            EmptyElementError(s, ei.name_);
        pah_->Begin(type, host);
        pah_->End();
        return true;
    }
    Ptr<Fb2Ctxt> GetCtxt(Fb2Ctxt *oldCtxt)
    {
        return pah_->GetCtxt(oldCtxt);
    }
    void Data(const String &data)
    {
        pah_->Contents(data);
    }
    void EndTag(LexScanner *s)
    {
        pah_->End();
        EE::End(s);
    }
};

//-----------------------------------------------------------------------
struct EE_Normal
{
    static void End(LexScanner *s) {s->EndElement();}
};
struct EE_SkipRest
{
    static void End(LexScanner *s) {s->SkipRestOfElementContent();}
};

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateEHandler(Ptr<Fb2AttrHandler> ph, bool skipRest)
{
    if(skipRest)
        return new EHandlerAttr<EE_SkipRest>(ph);
    else
        return new EHandlerAttr<EE_Normal>(ph);
}

//-----------------------------------------------------------------------
Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateEHandler(Ptr<Fb2NoAttrHandler> ph, bool skipRest)
{
    if(skipRest)
        return new EHandlerNoAttr<EE_SkipRest>(ph);
    else
        return new EHandlerNoAttr<EE_Normal>(ph);
}


//-----------------------------------------------------------------------
// ROOT ELEMENT HANDLER
//-----------------------------------------------------------------------
class FictionBoolElement : public Fb2EHandler
{
public:
    //virtuals
    bool StartTag(Fb2EType, LexScanner *s, Fb2Host *host)
    {
        Ptr<Lookup> lookup = new Lookup(s, host);
        host->RegisterNsLookup(lookup);
        return false;
    }
    Ptr<Fb2Ctxt>    GetCtxt (Fb2Ctxt *oldCtxt)      {return oldCtxt;}
    void            Data    (const String &data)    {}
    void            EndTag  (LexScanner *s)         {}      // skip rest without processing

private:
    class Lookup : public Fb2NsLookup
    {
    public:
        Lookup(LexScanner *s, Fb2Host *host)
        {
            AttrMap attrmap;
            s->BeginNotEmptyElement("FictionBook", &attrmap);

            // namespaces
            AttrMap::const_iterator cit = attrmap.begin(), cit_end = attrmap.end();
            bool has_fb = false, has_emptyfb = false;
            for(; cit != cit_end; ++cit)
            {
                static const String xmlns = "xmlns";
                static const std::size_t xmlns_len = xmlns.length();
                static const String fbID = "http://www.gribuser.ru/xml/fictionbook/2.0", xlID = "http://www.w3.org/1999/xlink";

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
