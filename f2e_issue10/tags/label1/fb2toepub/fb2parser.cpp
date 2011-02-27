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

static const String ename[E_COUNT] =
{
    "FictionBook",
    "a",
    "annotation",
    "author",
    "binary",
    "body",
    "book-name",
    "book-title",
    "cite",
    "city",
    "code",
    "coverpage",
    "custom-info",
    "date",
    "description",
    "document-info",
    "email",
    "emphasis",
    "empty-line",
    "epigraph",
    "first-name",
    "genre",
    "history",
    "home-page",
    "id",
    "isbn",
    "image",
    "keywords",
    "lang",
    "last-name",
    "middle-name",
    "nickname",
    "output-document-class",
    "output",
    "p",
    "part",
    "poem",
    "program-used",
    "publish-info",
    "publisher",
    "section",
    "sequence",
    "src-lang",
    "src-ocr",
    "src-title-info",
    "src-url",
    "stanza",
    "strikethrough",
    "strong",
    "style",
    "stylesheet",
    "sub",
    "subtitle",
    "sup",
    "table",
    "td",
    "text-author",
    "th",
    "title",
    "title-info",
    "tr",
    "translator",
    "v",
    "version",
    "year"
};

typedef std::vector<Ptr<ElementHandler> > HandlerVector;

//-----------------------------------------------------------------------
// Helper classes to parse stag and etag of element
class Element : Noncopyable
{
    Ptr<LexScanner> s_;
    bool            notempty_;
public:
    Element(ElementType type, LexScanner *s, AttrMap *attrmap = NULL) : s_(s)
        {notempty_ = s_->BeginElement(ename[type], attrmap);}
    ~Element()
        {if(notempty_) s_->EndElement();}

    bool NotEmpty() const   {return notempty_;}
};
//-----------------------------------------------------------------------
class ElementSkipRest : Noncopyable
{
    Ptr<LexScanner> s_;
    bool            notempty_;
public:
    ElementSkipRest(ElementType type, LexScanner *s, AttrMap *attrmap = NULL) : s_(s)
        {notempty_ = s_->BeginElement(ename[type], attrmap);}
    ~ElementSkipRest()
        {if(notempty_) s_->SkipRestOfElementContent();}

    bool NotEmpty() const   {return notempty_;}
};
//-----------------------------------------------------------------------
class NotEmptyElement : Noncopyable
{
    Ptr<LexScanner> s_;
public:
    NotEmptyElement(ElementType type, LexScanner *s, AttrMap *attrmap = NULL) : s_(s)
        {s_->BeginNotEmptyElement(ename[type], attrmap);}
    ~NotEmptyElement()
        {s_->EndElement();}
};
//-----------------------------------------------------------------------
class NotEmptyElementSkipRest : Noncopyable
{
    Ptr<LexScanner> s_;
public:
    NotEmptyElementSkipRest(ElementType type, LexScanner *s, AttrMap *attrmap = NULL) : s_(s)
        {s_->BeginNotEmptyElement(ename[type], attrmap);}
    ~NotEmptyElementSkipRest()
        {s_->SkipRestOfElementContent();}
};

//-----------------------------------------------------------------------
// Helper class to automatically call element handler methods
class AutoHandler : Noncopyable
{
    ElementType                 type_;
    Ptr<ElementHandler>         h_;
public:
    AutoHandler(ElementType type, const HandlerVector &hv, AttrMap *attrmap = NULL) : type_(type), h_(hv[type])
    {
        h_->Start(type, attrmap);
    }
    ~AutoHandler()
    {
        h_->End(type_);
    }

    void Contents(const String &data)   {h_->Contents(type_, data);}
};

//-----------------------------------------------------------------------
// And combination of above helper classes
template<class ET>
class AutoHandlerFor : Noncopyable
{
    ET          e_;
    AutoHandler h_;
public:
    AutoHandlerFor(ElementType type, const HandlerVector &hv, LexScanner *s, AttrMap *attrmap = NULL)
        : e_(type, s, attrmap), h_(type, hv, attrmap) {}

    bool NotEmpty() const               {return e_.NotEmpty();}
    void Contents(const String &data)   {h_.Contents(data);}
};



//-----------------------------------------------------------------------
// Dummy handler is used if custom handler is not registered
class DummyElementHandler : public ElementHandler
{
public:
    //virtuals
    void    Start   (ElementType type, const AttrMap *attrmap)  {}
    void    Contents(ElementType type, const String &data)      {}
    void    End     (ElementType type)                          {}
};



//-----------------------------------------------------------------------
// Parser itself
//-----------------------------------------------------------------------
class Fb2ParserImpl : public Fb2Parser, Noncopyable
{
public:
    Fb2ParserImpl(LexScanner *scanner) : s_(scanner), hv_(E_COUNT, dummy_) {}

    //virtuals
    void Register(ElementType type, ElementHandler *h);
    void Parse();

private:
    Ptr<LexScanner>             s_;
    HandlerVector               hv_;
    std::set<String>            xlns_;  // xlink namespaces
    static Ptr<ElementHandler>  dummy_;

    void ParseText              (ElementType type, AttrMap *attrmap);
    void ParseTextAttr          (ElementType type);
    void ParseTextNoAttr        (ElementType type);
    void SimpleTextAttr         (ElementType type);
    void SimpleTextNoAttr       (ElementType type);

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
Ptr<ElementHandler> Fb2ParserImpl::dummy_ = new DummyElementHandler();

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
void Fb2ParserImpl::ParseText(ElementType type, AttrMap *attrmap)
{
    AutoHandlerFor<Element> h(type, hv_, s_, attrmap);
    if(!h.NotEmpty())
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
                ss << "<" << t.s_ << "> unexpected in <" << ename[type] + ">";
                s_->Error(ss.str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </a>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::ParseTextAttr(ElementType type)
{
    AttrMap attrmap;
    ParseText(type, &attrmap);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::ParseTextNoAttr(ElementType type)
{
    ParseText(type, NULL);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::SimpleTextAttr(ElementType type)
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(type, hv_, s_, &attrmap);
    if(h.NotEmpty())
    {
        SetScannerDataMode setDataMode(s_);
        if(s_->LookAhead().type_ == LexScanner::DATA)
            h.Contents(s_->GetToken().s_);
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::SimpleTextNoAttr(ElementType type)
{
    AutoHandlerFor<Element> h(type, hv_, s_);
    if(h.NotEmpty())
    {
        SetScannerDataMode setDataMode(s_);
        if(s_->LookAhead().type_ == LexScanner::DATA)
            h.Contents(s_->GetToken().s_);
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::FictionBook()
{
    AttrMap attrmap;
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_FICTIONBOOK, hv_, s_, &attrmap);

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
                s_->Error("bad FictionBook namespace definition");
            has_fb = true;
        }
        else if(!cit->second.compare(xlID))
        {
            if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                s_->Error("bad xlink namespace definition");
            xlns_.insert(cit->first.substr(xmlns_len+1));
        }
    }
    if(!has_fb)
        s_->Error("missing FictionBook namespace definition");
    if(!has_emptyfb)
        s_->Error("non-empty FictionBook namespace not implemented");

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::a()
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_A, hv_, s_, &attrmap);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::annotation()
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_ANNOTATION, hv_, s_, &attrmap);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::author()
{
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_AUTHOR, hv_, s_);
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::binary()
{
    AttrMap attrmap;
    AutoHandlerFor<NotEmptyElement> h(E_BINARY, hv_, s_, &attrmap);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::body()
{
    AttrMap attrmap;
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_BODY, hv_, s_, &attrmap);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::book_title()
{
    SimpleTextNoAttr(E_BOOK_TITLE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::cite()
{
    AttrMap attrmap;
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_CITE, hv_, s_, &attrmap);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::code()
{
    ParseTextNoAttr(E_CODE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::coverpage()
{
    AutoHandlerFor<NotEmptyElement> h(E_COVERPAGE, hv_, s_);
    do
        image();
    while(s_->IsNextElement("image"));
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::date()
{
    SimpleTextAttr(E_DATE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::description()
{
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_DESCRIPTION, hv_, s_);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::document_info()
{
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_DOCUMENT_INFO, hv_, s_);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::emphasis()
{
    ParseTextNoAttr(E_EMPHASIS);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::empty_line()
{
    AutoHandlerFor<Element> h(E_EMPTY_LINE, hv_, s_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::epigraph()
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_EPIGRAPH, hv_, s_, &attrmap);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::first_name()
{
    SimpleTextNoAttr(E_FIRST_NAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::id()
{
    SimpleTextNoAttr(E_ID);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::image()
{
    ClrScannerDataMode clrDataMode(s_);
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_IMAGE, hv_, s_, &attrmap);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::isbn()
{
    SimpleTextNoAttr(E_ISBN);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::last_name()
{
    SimpleTextNoAttr(E_LAST_NAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::middle_name()
{
    SimpleTextNoAttr(E_MIDDLE_NAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::nickname()
{
    SimpleTextNoAttr(E_NICKNAME);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::lang()
{
    SimpleTextNoAttr(E_LANG);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::p()
{
    ParseTextAttr(E_P);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::poem()
{
    AttrMap attrmap;
    AutoHandlerFor<NotEmptyElement> h(E_POEM, hv_, s_, &attrmap);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::publish_info()
{
    AutoHandlerFor<ElementSkipRest> h(E_PUBLISH_INFO, hv_, s_);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::section()
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_SECTION, hv_, s_, &attrmap);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::stanza()
{
    AutoHandlerFor<NotEmptyElement> h(E_STANZA, hv_, s_);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strikethrough()
{
    ParseTextNoAttr(E_STRIKETHROUGH);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strong()
{
    ParseTextNoAttr(E_STRONG);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::style()
{
    ParseTextNoAttr(E_STYLE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sub()
{
    ParseTextNoAttr(E_SUB);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::subtitle()
{
    ParseTextAttr(E_SUBTITLE);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sup()
{
    ParseTextNoAttr(E_SUP);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::table()
{
    AttrMap attrmap;
    AutoHandlerFor<NotEmptyElement> h(E_TABLE, hv_, s_, &attrmap);

    do
    {
        //<tr>
        tr();
        //</tr>
    }
    while(s_->IsNextElement("tr"));
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::td()
{
    ParseTextAttr(E_TD);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::text_author()
{
    ParseTextAttr(E_TEXT_AUTHOR);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::th()
{
    ParseTextAttr(E_TH);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title()
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_TITLE, hv_, s_, &attrmap);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title_info()
{
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_TITLE_INFO, hv_, s_);

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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::tr()
{
    AttrMap attrmap;
    AutoHandlerFor<Element> h(E_TR, hv_, s_, &attrmap);
    if(!h.NotEmpty())
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
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::v()
{
    ParseTextAttr(E_V);
}

//-----------------------------------------------------------------------
Ptr<Fb2Parser> FB2TOEPUB_DECL CreateFb2Parser(LexScanner *scanner)
{
    return new Fb2ParserImpl(scanner);
}

};  //namespace Fb2ToEpub
