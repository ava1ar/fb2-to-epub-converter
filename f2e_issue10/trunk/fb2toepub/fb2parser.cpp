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
    ElementInfo("author",               true),
    ElementInfo("binary",               true),
    ElementInfo("body",                 true),
    ElementInfo("book-name",            false),
    ElementInfo("book-title",           false),
    ElementInfo("cite",                 true),
    ElementInfo("city",                 false),
    ElementInfo("code",                 false),
    ElementInfo("coverpage",            true),
    ElementInfo("custom-info",          false),
    ElementInfo("date",                 false),
    ElementInfo("description",          true),
    ElementInfo("document-info",        true),
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
    ElementInfo("image",                false),
    ElementInfo("keywords",             false),
    ElementInfo("lang",                 false),
    ElementInfo("last-name",            false),
    ElementInfo("middle-name",          false),
    ElementInfo("nickname",             false),
    ElementInfo("output-document-class",false),
    ElementInfo("output",               false),
    ElementInfo("p",                    false),
    ElementInfo("part",                 false),
    ElementInfo("poem",                 true),
    ElementInfo("program-used",         false),
    ElementInfo("publish-info",         false),
    ElementInfo("publisher",            false),
    ElementInfo("section",              false),
    ElementInfo("sequence",             false),
    ElementInfo("src-lang",             false),
    ElementInfo("src-ocr",              false),
    ElementInfo("src-title-info",       false),
    ElementInfo("src-url",              false),
    ElementInfo("stanza",               true),
    ElementInfo("strikethrough",        false),
    ElementInfo("strong",               false),
    ElementInfo("style",                false),
    ElementInfo("stylesheet",           false),
    ElementInfo("sub",                  false),
    ElementInfo("subtitle",             false),
    ElementInfo("sup",                  false),
    ElementInfo("table",                true),
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
// Default handler
//-----------------------------------------------------------------------
class DefElementHandler : public ElementHandler
{
public:
    //virtuals
    bool Start(ElementType type, LexScanner *s)
    {
        const ElementInfo &ei = einfo[type];
        if(s->BeginElement(ei.name_))
            return false;

        if(ei.notempty_)
        {
            std::ostringstream ss;
            ss << "element <" << ei.name_ << "> can't be empty";
            s->Error(ss.str());
        }
        return true;
    }
    void Contents(const String&)    {}
    void End(LexScanner *s)         {s->EndElement();}
};


typedef std::vector<Ptr<ElementHandler> > HandlerVector;

//-----------------------------------------------------------------------
// Auto handler
//-----------------------------------------------------------------------
class AutoHandler : Noncopyable
{
    ElementType         type_;
    Ptr<ElementHandler> ph_;
public:
    AutoHandler(ElementType type, const HandlerVector &hv) : type_(type), ph_(hv[type]) {}

    bool Start      (LexScanner *s)         {return ph_->Start(type_, s);}
    void Contents   (const String &data)    {ph_->Contents(data);}
    void End        (LexScanner *s)         {ph_->End(s);}
};

//-----------------------------------------------------------------------
// Syntax parser
//-----------------------------------------------------------------------
class Fb2ParserImpl : public Fb2Parser, Noncopyable
{
public:
    Fb2ParserImpl(LexScanner *scanner) : s_(scanner), hv_(E_COUNT, def_) {}

    //virtuals
    void Register(ElementType type, ElementHandler *h);
    void Parse();

private:
    Ptr<LexScanner>             s_;
    HandlerVector               hv_;
    static Ptr<ElementHandler>  def_;

    void ParseText              (ElementType type);
    void SimpleText             (ElementType type);

    // FictionBook elements
    void FictionBook            ();
    void a                      ();
    void annotation             ();
    void author                 ();
    void binary                 ();
    void body                   ();
    //void book_name              ();
    void book_title             ();
    void cite                   ();
    //void city                   ();
    void code                   ();
    void coverpage              ();
    //void custom_info            ();
    void date                   ();
    void description            ();
    void document_info          ();
    //void email                  ();
    void emphasis               ();
    void empty_line             ();
    void epigraph               ();
    void first_name             ();
    //void genre                  ();
    //void history                ();
    //void home_page              ();
    void id                     ();
    void image                  ();
    void isbn                   ();
    //void keywords               ();
    void lang                   ();
    void last_name              ();
    void middle_name            ();
    void nickname               ();
    //void output_document_class  ();
    //void output                 ();
    void p                      ();
    //void part                   ();
    void poem                   ();
    //void program_used           ();
    void publish_info           ();
    //void publisher              ();
    void section                ();
    //void sequence               ();
    //void src_lang               ();
    //void src_ocr                ();
    //void src_title_info         ();
    //void src_url                ();
    void stanza                 ();
    void strikethrough          ();
    void strong                 ();
    void style                  ();
    //void stylesheet             ();
    void sub                    ();
    void subtitle               ();
    void sup                    ();
    void table                  ();
    void td                     ();
    void text_author            ();
    void th                     ();
    void title                  ();
    void title_info             ();
    void tr                     ();
    //void translator             ();
    void v                      ();
    //void version                ();
    //void year                   ();
};


//-----------------------------------------------------------------------
Ptr<ElementHandler> Fb2ParserImpl::def_ = new DefElementHandler();

//-----------------------------------------------------------------------
void Fb2ParserImpl::Register(ElementType type, ElementHandler *h)
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
    hv_[idx] = h;
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::Parse()
{
    FictionBook();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::ParseText(ElementType type)
{
    AutoHandler h(type, hv_);
    if(h.Start(s_))
        return;

    SetScannerDataMode setDataMode(s_);
    for(;;)
    {
        LexScanner::Token t = s_->LookAhead();
        switch(t.type_)
        {
        default:
            return;

        case LexScanner::DATA:
            h.Contents(s_->GetToken().s_);
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <a>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong();
            else if(!t.s_.compare("emphasis"))
                emphasis();
            else if(!t.s_.compare("style"))
                style();
            else if(!t.s_.compare("a"))
                a();
            else if(!t.s_.compare("strikethrough"))
                strikethrough();
            else if(!t.s_.compare("sub"))
                sub();
            else if(!t.s_.compare("sup"))
                sup();
            else if(!t.s_.compare("code"))
                code();
            else if(!t.s_.compare("image"))
                image();
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <" << einfo[type].name_ + ">";
                s_->Error(ss.str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </a>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::SimpleText(ElementType type)
{
    AutoHandler h(type, hv_);
    if(!h.Start(s_))
    {
        SetScannerDataMode setDataMode(s_);
        if(s_->LookAhead().type_ == LexScanner::DATA)
            h.Contents(s_->GetToken().s_);
        h.End(s_);
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::FictionBook()
{
    AutoHandler h(E_FICTIONBOOK, hv_);
    h.Start(s_);

    //<stylesheet>
    s_->SkipAll("stylesheet");
    //</stylesheet>

    //<description>
    description();
    //</description>

    //<body>
    body();
    if(s_->IsNextElement("body"))
        body();
    if(s_->IsNextElement("body"))
        body();
    //</body>

    //<binary>
    while(s_->IsNextElement("binary"))
        binary();
    //</binary>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::a()
{
    AutoHandler h(E_A, hv_);
    if(h.Start(s_))
        return;

    SetScannerDataMode setDataMode(s_);
    for(;;)
    {
        LexScanner::Token t = s_->LookAhead();
        switch(t.type_)
        {
        default:
            return;

        case LexScanner::DATA:
            h.Contents(s_->GetToken().s_);
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong();
            else if(!t.s_.compare("emphasis"))
                emphasis();
            else if(!t.s_.compare("style"))
                style();
            else if(!t.s_.compare("strikethrough"))
                strikethrough();
            else if(!t.s_.compare("sub"))
                sub();
            else if(!t.s_.compare("sup"))
                sup();
            else if(!t.s_.compare("code"))
                code();
            else if(!t.s_.compare("image"))
                image();
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <a>";
                s_->Error(ss.str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::annotation()
{
    AutoHandler h(E_ANNOTATION, hv_);
    if(h.Start(s_))
        return;

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <subtitle>, <empty-line>, <table>
        if(!t.s_.compare("p"))
            p();
        else if(!t.s_.compare("poem"))
            poem();
        else if(!t.s_.compare("cite"))
            cite();
        else if(!t.s_.compare("subtitle"))
            subtitle();
        else if(!t.s_.compare("empty-line"))
            empty_line();
        else if(!t.s_.compare("table"))
            table();
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <annotation>";
            s_->Error(ss.str());
        }
        //</p>, </poem>, </cite>, </subtitle>, </empty-line>, </table>
    }

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::author()
{
    AutoHandler h(E_AUTHOR, hv_);
    h.Start(s_);

    if(s_->IsNextElement("first-name"))
    {
        //<first-name>
        first_name();
        //</first-name>

        //<middle-name>
        if(s_->IsNextElement("middle-name"))
            middle_name();
        //<middle-name>

        //<last-name>
        last_name();
        //</last-name>
    }
    else if(s_->IsNextElement("nickname"))
    {
        //<nickname>
        nickname();
        //</nickname>
    }
    else
        s_->Error("<first-name> or <nickname> expected");

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::binary()
{
    AutoHandler h(E_BINARY, hv_);
    h.Start(s_);

    SetScannerDataMode setDataMode(s_);
    LexScanner::Token t = s_->GetToken();
    if(t.type_ != LexScanner::DATA)
        s_->Error("<binary> data expected");
    h.Contents(t.s_);

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::body()
{
    AutoHandler h(E_BODY, hv_);
    h.Start(s_);

    //<image>
    if(s_->IsNextElement("image"))
        image();
    //</image>

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    //<epigraph>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</epigraph>

    do
    {
        //<section>
        section();
        //</section>
    }
    while(s_->IsNextElement("section"));

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::book_title()
{
    SimpleText(E_BOOK_TITLE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::cite()
{
    AutoHandler h(E_CITE, hv_);
    h.Start(s_);

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <subtitle>, <empty-line>, <poem>, <table>
        if(!t.s_.compare("p"))
            p();
        else if(!t.s_.compare("subtitle"))
            subtitle();
        else if(!t.s_.compare("empty-line"))
            empty_line();
        else if(!t.s_.compare("poem"))
            poem();
        else if(!t.s_.compare("table"))
            table();
        else if(!t.s_.compare("text-author"))
            break;
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <cite>";
            s_->Error(ss.str());
        }
        //</p>, </subtitle>, </empty-line>, </poem>, </table>
    }

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::code()
{
    ParseText(E_CODE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::coverpage()
{
    AutoHandler h(E_COVERPAGE, hv_);
    h.Start(s_);
    do
        image();
    while(s_->IsNextElement("image"));
    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::date()
{
    SimpleText(E_DATE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::description()
{
    AutoHandler h(E_DESCRIPTION, hv_);
    h.Start(s_);

    //<title-info>
    title_info();
    //</title-info>

    //<src-title-info>
    s_->SkipIfElement("src-title-info");
    //</src-title-info>

    //<document-info>
    document_info();
    //</document-info>

    //<publish-info>
    if(s_->IsNextElement("publish-info"))
        publish_info();
    //</publish-info>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::document_info()
{
    AutoHandler h(E_DOCUMENT_INFO, hv_);
    h.Start(s_);

    //<author>
    s_->CheckAndSkipElement("author");
    s_->SkipAll("author");
    //</author>

    //<program-used>
    s_->SkipIfElement("program-used");
    //</program-used>

    //<date>
    s_->CheckAndSkipElement("date");
    //</date>

    //<src-url>
    s_->SkipAll("src-url");
    //</src-url>

    //<src-ocr>
    s_->SkipIfElement("src-ocr");
    //</src-ocr>

    //<id>
    id();
    //<id>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::emphasis()
{
    ParseText(E_EMPHASIS);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::empty_line()
{
    AutoHandler h(E_EMPTY_LINE, hv_);
    if(!h.Start(s_))
        h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::epigraph()
{
    AutoHandler h(E_EPIGRAPH, hv_);
    if(h.Start(s_))
        return;

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        //<p>, <poem>, <cite>, <empty-line>
        if(!t.s_.compare("p"))
            p();
        else if(!t.s_.compare("poem"))
            poem();
        else if(!t.s_.compare("cite"))
            cite();
        else if(!t.s_.compare("empty-line"))
            empty_line();
        else if(!t.s_.compare("text-author"))
            break;
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <epigraph>";
            s_->Error(ss.str());
        }
        //</p>, </poem>, </cite>, </empty-line>
    }

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::first_name()
{
    SimpleText(E_FIRST_NAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::id()
{
    SimpleText(E_ID);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::image()
{
    ClrScannerDataMode clrDataMode(s_);
    AutoHandler h(E_IMAGE, hv_);
    if(!h.Start(s_))
        h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::isbn()
{
    SimpleText(E_ISBN);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::last_name()
{
    SimpleText(E_LAST_NAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::middle_name()
{
    SimpleText(E_MIDDLE_NAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::nickname()
{
    SimpleText(E_NICKNAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::lang()
{
    SimpleText(E_LANG);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::p()
{
    ParseText(E_P);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::poem()
{
    AutoHandler h(E_POEM, hv_);
    h.Start(s_);

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    //<epigraph>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</epigraph>

    //<stanza>
    do
        stanza();
    while(s_->IsNextElement("stanza"));
    //</stanza>

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    //<data>
    if(s_->IsNextElement("date"))
        date();
    //</data>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::publish_info()
{
    AutoHandler h(E_PUBLISH_INFO, hv_);
    if(h.Start(s_))
        return;

    //<book-name>
    s_->SkipIfElement("book-name");
    //</book-name>

    //<publisher>
    s_->SkipIfElement("publisher");
    //</publisher>

    //<city>
    s_->SkipIfElement("city");
    //</city>

    //<year>
    s_->SkipIfElement("year");
    //</year>

    //<isbn>
    if(s_->IsNextElement("isbn"))
        isbn();
    //</isbn>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::section()
{
    AutoHandler h(E_SECTION, hv_);
    if(h.Start(s_))
        return;

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    //<epigraph>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</epigraph>

    //<image>
    if(s_->IsNextElement("image"))
        image();
    //</image>

    //<annotation>
    if(s_->IsNextElement("annotation"))
        annotation();
    //</annotation>

    if(s_->IsNextElement("section"))
        do
        {
            //<section>
            section();
            //</section>
        }
        while(s_->IsNextElement("section"));
    else
    {
        for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
        {
            //<p>, <image>, <poem>, <subtitle>, <cite>, <empty-line>, <table>
            if(!t.s_.compare("p"))
                p();
            else if(!t.s_.compare("image"))
                image();
            else if(!t.s_.compare("poem"))
                poem();
            else if(!t.s_.compare("subtitle"))
                subtitle();
            else if(!t.s_.compare("cite"))
                cite();
            else if(!t.s_.compare("empty-line"))
                empty_line();
            else if(!t.s_.compare("table"))
                table();
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <section>";
                s_->Error(ss.str());
            }
            //</p>, </image>, </poem>, </subtitle>, </cite>, </empty-line>, </table>
        }
    }

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::stanza()
{
    AutoHandler h(E_STANZA, hv_);
    h.Start(s_);

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    //<subtitle>
    if(s_->IsNextElement("subtitle"))
        subtitle();
    //</subtitle>

    do
    {
        //<v>
        v();
        //</v>
    }
    while(s_->IsNextElement("v"));

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strikethrough()
{
    ParseText(E_STRIKETHROUGH);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strong()
{
    ParseText(E_STRONG);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::style()
{
    ParseText(E_STYLE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sub()
{
    ParseText(E_SUB);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::subtitle()
{
    ParseText(E_SUBTITLE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sup()
{
    ParseText(E_SUP);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::table()
{
    AutoHandler h(E_TABLE, hv_);
    h.Start(s_);

    do
    {
        //<tr>
        tr();
        //</tr>
    }
    while(s_->IsNextElement("tr"));

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::td()
{
    ParseText(E_TD);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::text_author()
{
    ParseText(E_TEXT_AUTHOR);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::th()
{
    ParseText(E_TH);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title()
{
    AutoHandler h(E_TITLE, hv_);
    if(h.Start(s_))
        return;

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        if(!t.s_.compare("p"))
        {
            //<p>
            p();
            //</p>
        }
        else if(!t.s_.compare("empty-line"))
        {
            //<empty-line>
            empty_line();
            //</empty-line>
        }
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <title>";
            s_->Error(ss.str());
        }
    }

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title_info()
{
    AutoHandler h(E_TITLE_INFO, hv_);
    h.Start(s_);

    //<genre>
    s_->CheckAndSkipElement("genre");
    s_->SkipAll("genre");
    //</genre>

    //<author>
    do
        author();
    while(s_->IsNextElement("author"));
    //<author>

    //<book-title>
    book_title();
    //</book-title>

    //<annotation>
    if(s_->IsNextElement("annotation"))
        annotation();
    //</annotation>

    //<keywords>
    s_->SkipIfElement("keywords");
    //</keywords>

    //<date>
    if(s_->IsNextElement("date"))
        date();
    //<date>

    //<coverpage>
    if(s_->IsNextElement("coverpage"))
        coverpage();
    //</coverpage>

    //<lang>
    lang();
    //</lang>

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::tr()
{
    AutoHandler h(E_TR, hv_);
    if(h.Start(s_))
        return;

    for(;;)
    {
        //<th>, <td>
        if(s_->IsNextElement("th"))
            th();
        else if(s_->IsNextElement("td"))
            td();
        else
            break;
        //</th>, </td>
    }

    h.End(s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::v()
{
    ParseText(E_V);
}

//-----------------------------------------------------------------------
Ptr<Fb2Parser> FB2TOEPUB_DECL CreateFb2Parser(LexScanner *scanner)
{
    return new Fb2ParserImpl(scanner);
}

};  //namespace Fb2ToEpub
