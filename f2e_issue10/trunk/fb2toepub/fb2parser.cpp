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
    bool            notempty_, notproc_;
public:
    Element(ElementType type, LexScanner *s, AttrMap *attrmap = NULL)
        : s_(s), notempty_(s->BeginElement(ename[type], attrmap)), notproc_(true) {}
    ~Element()
        {if(notempty_ && notproc_) s_->EndElement();}

    bool NotEmpty() const   {return notempty_;}
    void SetProcessed()     {notproc_ = false;}
};
//-----------------------------------------------------------------------
class NotEmptyElement : Noncopyable
{
    Ptr<LexScanner> s_;
    bool            notproc_;
public:
    NotEmptyElement(ElementType type, LexScanner *s, AttrMap *attrmap = NULL) : s_(s)
        {s_->BeginNotEmptyElement(ename[type], attrmap);}
    ~NotEmptyElement()
        {if(notproc_) s_->EndElement();}

    void SetProcessed()     {notproc_ = false;}
};
//-----------------------------------------------------------------------
class NotEmptyElementSkipRest : Noncopyable
{
    Ptr<LexScanner> s_;
    bool            notproc_;
public:
    NotEmptyElementSkipRest(ElementType type, LexScanner *s, AttrMap *attrmap = NULL) : s_(s)
        {s_->BeginNotEmptyElement(ename[type], attrmap);}
    ~NotEmptyElementSkipRest()
        {if(notproc_) s_->SkipRestOfElementContent();}

    void SetProcessed()     {notproc_ = false;}
};

//-----------------------------------------------------------------------
// Helper class to automatically call element handler methods
class AutoHandler : Noncopyable
{
    ElementType                 type_;
    Ptr<ElementHandler>         h_;
    bool                        process_;
public:
    AutoHandler(ElementType type, const HandlerVector &hv, LexScanner *s, AttrMap *attrmap = NULL)
        : type_(type), h_(hv[type]), process_(h_->Start(type, s, attrmap)) {}
    ~AutoHandler()
        {h_->End(type_);}

    bool ToProcess() const              {return process_;}
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
        : e_(type, s, attrmap), h_(type, hv, s, attrmap)
    {
        if(!h_.ToProcess())
            e_.SetProcessed();
    }

    bool NotEmpty() const               {return e_.NotEmpty();}
    bool ToProcess() const              {return h_.ToProcess();}
    void Contents(const String &data)   {h_.Contents(data);}
};



//-----------------------------------------------------------------------
// Dummy handler is used if custom handler is not registered
class DummyElementHandler : public ElementHandler
{
public:
    //virtuals
    bool    Start   (ElementType type, LexScanner *s, const AttrMap *attrmap)   {return true;}
    void    Contents(ElementType type, const String &data)                      {}
    void    End     (ElementType type)                                          {}
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

    // FictionBook elements
    void FictionBook            ();
    //void a                      ();
    //void annotation             ();
    //void author                 ();
    //void binary                 ();
    //void body                   ();
    //void book_name              ();
    //void book_title             ();
    //void cite                   ();
    //void city                   ();
    //void code                   ();
    //void coverpage              ();
    //void custom_info            ();
    //void date                   ();
    void description            ();
    //void document_info          ();
    //void email                  ();
    //void emphasis               ();
    //void empty_line             ();
    //void epigraph               ();
    //void first_name             ();
    //void genre                  ();
    //void history                ();
    //void home_page              ();
    //void id                     ();
    //void image                  ();
    //void isbn                   ();
    //void keywords               ();
    //void lang                   ();
    //void last_name              ();
    //void middle_name            ();
    //void nickname               ();
    //void output_document_class  ();
    //void output                 ();
    //void p                      ();
    //void part                   ();
    //void poem                   ();
    //void program_used           ();
    //void publish_info           ();
    //void publisher              ();
    //void section                ();
    //void sequence               ();
    //void src_lang               ();
    //void src_ocr                ();
    //void src_title_info         ();
    //void src_url                ();
    //void stanza                 ();
    //void strikethrough          ();
    //void strong                 ();
    //void style                  ();
    //void stylesheet             ();
    //void sub                    ();
    //void subtitle               ();
    //void sup                    ();
    //void table                  ();
    //void td                     ();
    //void text_author            ();
    //void th                     ();
    //void title                  ();
    //void title_info             ();
    //void tr                     ();
    //void translator             ();
    //void v                      ();
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

    if(h.ToProcess())
    {
        //<stylesheet>
        s_->SkipAll("stylesheet");
        //</stylesheet>

        //<description>
        description();
        //</description>

        /*
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
        */
    }
}

/*
//-----------------------------------------------------------------------
void Fb2ParserImpl::a()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("a", &attrmap);

    AutoHandler h(hdls_, E_A, &attrmap);
    if(!notempty)
        return;

    SetScannerDataMode setDataMode(s_);
    for(;;)
    {
        LexScanner::Token t = s_->LookAhead();
        switch(t.type_)
        {
        default:
            s_->EndElement();
            return;

        case LexScanner::DATA:
            h.Contents(t.s_);
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
                image(true, true, false);
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
void Fb2ParserImpl::annotation(bool startUnit)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("annotation", &attrmap);

    AutoHandler h(hdls_, E_ANNOTATION, &attrmap);
    if(!notempty)
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

    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::author()
{
    s_->BeginNotEmptyElement("author");
    AutoHandler h(hdls_, E_AUTHOR);
    s_->SkipRestOfElementContent();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::binary()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("binary", &attrmap);
    AutoHandler h(hdls_, E_BINARY, &attrmap);
    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::body()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("body", &attrmap);

    AutoHandler h(hdls_, E_BODY, &attrmap);

    //<image>
    if(s_->IsNextElement("image"))
        image();
    //</image>

    //<title>
    if(s_->IsNextElement("title"))
        title(true);
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

    s_->SkipRestOfElementContent(); // skip rest of <body>
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::book_title()
{
    title_ = s_->SimpleTextElement("book-title");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::cite()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("cite", &attrmap);
    pout_->WriteStr("<div class=\"citation\"");
    AddId(attrmap);
    CopyXmlLang(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

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

    s_->EndElement();
    pout_->WriteStr("</div>\n");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::code()
{
    if(s_->BeginElement("code"))
    {
        pout_->WriteStr("<code class=\"e_code\">");
        ParseTextAndEndElement("code");
        pout_->WriteStr("</code>");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::coverpage()
{
    s_->BeginNotEmptyElement("coverpage");
    StartUnit(Unit::COVERPAGE);
    do
    {
        pout_->WriteStr("<div class=\"coverpage\">");
        image(true, false, true);
        pout_->WriteStr("</div>");
    }
    while(s_->IsNextElement("image"));
    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::date()
{
    AttrMap attrmap;
    if(s_->BeginElement("date"), &attrmap)
    {
        SetScannerDataMode setDataMode(s_);
        if(s_->LookAhead().type_ == LexScanner::DATA)
        {
            pout_->WriteFmt("<p class=\"date\"");
            CopyXmlLang(attrmap);
            pout_->WriteFmt(">%s</p>\n", s_->GetToken().s_.c_str());
        }
        s_->EndElement();
    }
}

//-----------------------------------------------------------------------
static bool IsDateCorrect(const String &s)
{
    // date format should be YYYY[-MM[-DD]]
    // (but we don't check if year, month or day value is valid!)
    if(s.length() < 4 || !isdigit(s[0]) || !isdigit(s[1]) || !isdigit(s[2]) || !isdigit(s[3]))
        return false;
    if(s.length() > 4 && (s.length() < 7 || s[4] != '-' || !isdigit(s[5]) || !isdigit(s[6])))
        return false;
    if(s.length() > 7 && (s.length() != 10 || s[7] != '-' || !isdigit(s[8]) || !isdigit(s[9])))
        return false;
    return true;
}

//-----------------------------------------------------------------------
String Fb2ParserImpl::date__epub()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("date", &attrmap);

    String text = attrmap["value"];
    if(IsDateCorrect(text))
    {
        if(notempty)
            s_->EndElement();
        return text;
    }

    if(!notempty)
        return "";

    SetScannerDataMode setDataMode(s_);
    if(s_->LookAhead().type_ == LexScanner::DATA)
        text = s_->GetToken().s_;
    s_->EndElement();
    return IsDateCorrect(text) ? text : String("");
}
*/

//-----------------------------------------------------------------------
void Fb2ParserImpl::description()
{
    AutoHandlerFor<NotEmptyElementSkipRest> h(E_DESCRIPTION, hv_, s_);
    if(h.ToProcess())
    {
        /*
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
        */
    }
}

/*
//-----------------------------------------------------------------------
void Fb2ParserImpl::document_info()
{
    s_->BeginNotEmptyElement("document-info");

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

    s_->SkipRestOfElementContent(); // skip rest of <document-info>
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::emphasis()
{
    if(s_->BeginElement("emphasis"))
    {
        pout_->WriteStr("<em class=\"emphasis\">");
        ParseTextAndEndElement("emphasis");
        pout_->WriteStr("</em>");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::empty_line()
{
    bool notempty = s_->BeginElement("empty-line");
    pout_->WriteStr("<p class=\"empty-line\"> </p>\n");
    if(notempty)
        s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::epigraph()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("epigraph", &attrmap);
    pout_->WriteStr("<div class=\"epigraph\"");
    AddId(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

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

    s_->EndElement();
    pout_->WriteStr("</div>\n");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::id()
{
    static const String uuidpfx = "urn:uuid:";

    String id = s_->SimpleTextElement("id"), uuid = id;
    if(!uuid.compare(0, uuidpfx.length(), uuidpfx))
        uuid = uuid.substr(uuidpfx.length());
    if(!IsValidUUID(uuid))
    {
        id1_ = id;
        uuid = GenerateUUID();
    }

    id_ = uuidpfx + uuid;
    MakeAdobeKey(uuid, adobeKey_);
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::image(bool fb2_inline, bool html_inline, bool scale)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("image", &attrmap);

    // get file href
    String href = Findhref(attrmap), alt = attrmap["alt"];
    if(!href.empty())
    {
        if(href[0] == '#')
        {
            // internal reference
            href = String("bin/") + href.substr(1);

            // remember name of the cover page image file
            if(units_[unitIdx_].type_ == Unit::COVERPAGE && coverFile_.empty())
                coverFile_ = href;
        }

        bool has_id = !fb2_inline && attrmap.find("id") != attrmap.end();
        if(has_id)
        {
            pout_->WriteStr("<div");
            AddId(attrmap);
            pout_->WriteStr(">");
        }

        String group = html_inline ? "span" : "div";

        pout_->WriteFmt("<%s class=\"image\">", group.c_str());
        if(scale)
            pout_->WriteFmt("<img style=\"height: 100%%;\" alt=\"%s\" src=\"%s\"/>", EncodeStr(alt).c_str(), EncodeStr(href).c_str());
        else
            pout_->WriteFmt("<img alt=\"%s\" src=\"%s\"/>", EncodeStr(alt).c_str(), EncodeStr(href).c_str());

        if(!fb2_inline)
        {
            if(html_inline)
                InternalError(__FILE__, __LINE__, "<image> error");
            AttrMap::const_iterator cit = attrmap.find("title");
            if(cit != attrmap.end())
                pout_->WriteFmt("<p>%s</p>\n", EncodeStr(cit->second).c_str());
        }
        pout_->WriteFmt("</%s>", group.c_str());

        if(has_id)
            pout_->WriteStr("</div>\n");
    }
    if(!notempty)
        return;
    ClrScannerDataMode clrDataMode(s_);
    s_->EndElement();
}

//-----------------------------------------------------------------------
String Fb2ParserImpl::isbn()
{
    if(!s_->BeginElement("isbn"))
        return "";

    String text;
    SetScannerDataMode setDataMode(s_);
    if(s_->LookAhead().type_ == LexScanner::DATA)
        text = s_->GetToken().s_;
    s_->EndElement();
    return text;
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::lang()
{
    lang_ = s_->SimpleTextElement("lang");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::p(const char *pelement, const char *cls)
{
    AttrMap attrmap;
    if(s_->BeginElement("p", &attrmap))
    {
        pout_->WriteFmt("<%s", pelement);
        if(cls)
            pout_->WriteFmt(" class=\"%s\"", cls);
        AddId(attrmap);
        CopyXmlLang(attrmap);
        pout_->WriteStr(">");

        ParseTextAndEndElement("p");
        pout_->WriteFmt("</%s>\n", pelement);
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::poem()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("poem", &attrmap);
    pout_->WriteStr("<div class=\"poem\"");
    AddId(attrmap);
    CopyXmlLang(attrmap);
    pout_->WriteStr(">");

    //<title>
    if(s_->IsNextElement("title"))
        title(false);
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

    pout_->WriteStr("</div>\n");
    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::publish_info()
{
    if(!s_->BeginElement("publish-info"))
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
        isbn_ = isbn();
    //</isbn>

    s_->SkipRestOfElementContent(); // skip rest of <publish-info>
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::section()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("section", &attrmap);

    // set section language
    SetLanguage l(&sectXmlLang_, attrmap);

    sectionSize_ = 0;
    StartUnit(Unit::SECTION, &attrmap);

    if(!notempty)
        return;

    //<title>
    if(s_->IsNextElement("title"))
    {
        // add anchor ref
        String id = units_[unitIdx_].noteRefId_;
        if(!id.empty())
        {
            id = noteidToAnchorId_[refidToNew_[id]];
            if(!id.empty())
                id = refidToUnit_[id]->file_ + ".xhtml#" + id;
        }

        title(false, id);
    }
    //</title>

    //<epigraph>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</epigraph>

    //<image>
    if(s_->IsNextElement("image"))
        image(false, false, false);
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
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE1);
                image(false, false, false);
            }
            else if(!t.s_.compare("poem"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE1);
                poem();
            }
            else if(!t.s_.compare("subtitle"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE0);
                subtitle();
            }
            else if(!t.s_.compare("cite"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE2);
                cite();
            }
            else if(!t.s_.compare("empty-line"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE2);
                empty_line();
            }
            else if(!t.s_.compare("table"))
            {
                SwitchUnitIfSizeAbove(UNIT_SIZE1);
                table();
            }
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <section>";
                s_->Error(ss.str());
            }
            //</p>, </image>, </poem>, </subtitle>, </cite>, </empty-line>, </table>

            SwitchUnitIfSizeAbove(MAX_UNIT_SIZE);
        }
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::stanza()
{
    s_->BeginNotEmptyElement("stanza");
    pout_->WriteStr("<div class=\"stanza\">");

    //<title>
    if(s_->IsNextElement("title"))
        title(false);
    //</title>

    //<subtitle>
    if(s_->IsNextElement("subtitle"))
        subtitle();
    //</subtitle>

    do
        v();
    while(s_->IsNextElement("v"));

    pout_->WriteStr("</div>\n");
    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strikethrough()
{
    if(s_->BeginElement("strikethrough"))
    {
        pout_->WriteStr("<del class=\"strikethrough\">");
        ParseTextAndEndElement("strikethrough");
        pout_->WriteStr("</del>");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::strong()
{
    if(s_->BeginElement("strong"))
    {
        pout_->WriteStr("<strong class=\"e_strong\">");
        ParseTextAndEndElement("strong");
        pout_->WriteStr("</strong>");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::style()
{
    // ignore style
    if(s_->BeginElement("style"))
        ParseTextAndEndElement("strong");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sub()
{
    if(s_->BeginElement("sub"))
    {
        pout_->WriteStr("<sub class=\"e_sub\">");
        ParseTextAndEndElement("sub");
        pout_->WriteStr("</sub>");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::subtitle()
{
    AttrMap attrmap;
    if(s_->BeginElement("subtitle", &attrmap))
    {
        pout_->WriteStr("<h2 class=\"e_h2\"");
        AddId(attrmap);
        CopyXmlLang(attrmap);
        pout_->WriteStr(">");

        ParseTextAndEndElement("subtitle");
        pout_->WriteStr("</h2>\n");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::sup()
{
    if(s_->BeginElement("sup"))
    {
        pout_->WriteStr("<sup class=\"e_sup\">");
        ParseTextAndEndElement("sup");
        pout_->WriteStr("</sup>");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::table()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("table", &attrmap);
    pout_->WriteFmt("<table");
    AddId(attrmap);

    CopyAttribute("style", attrmap);

    pout_->WriteStr(">");
    do
    {
        //<tr>
        tr();
        //</tr>
    }
    while(s_->IsNextElement("tr"));
    pout_->WriteFmt("</table>\n");
    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::td()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("td", &attrmap);

    pout_->WriteFmt("<td");
    AddId(attrmap);

    CopyAttribute("style", attrmap);
    CopyAttribute("colspan", attrmap);
    CopyAttribute("rowspan", attrmap);
    CopyAttribute("align", attrmap);
    CopyAttribute("valign", attrmap);
    CopyXmlLang(attrmap);

    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

    ParseTextAndEndElement("td");
    pout_->WriteStr("</td>\n");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::text_author()
{
    AttrMap attrmap;
    if(s_->BeginElement("text-author", &attrmap))
    {
        pout_->WriteFmt("<div class=\"text_author\"");
        AddId(attrmap);
        CopyXmlLang(attrmap);
        pout_->WriteStr(">");

        ParseTextAndEndElement("text-author");
        pout_->WriteStr("</div>\n");
    }
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::th()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("th", &attrmap);

    pout_->WriteFmt("<th");
    AddId(attrmap);

    CopyAttribute("style", attrmap);
    CopyAttribute("colspan", attrmap);
    CopyAttribute("rowspan", attrmap);
    CopyAttribute("align", attrmap);
    CopyAttribute("valign", attrmap);
    CopyXmlLang(attrmap);

    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

    ParseTextAndEndElement("th");
    pout_->WriteStr("</th>\n");
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title(bool startUnit, const String &anchorid)
{
    AttrMap attrmap;
    if(!s_->BeginElement("title", &attrmap))
        return;

    if(startUnit)
        StartUnit(Unit::TITLE);

    pout_->WriteFmt("<div class=\"title\"");
    CopyXmlLang(attrmap);
    pout_->WriteFmt(">\n");
    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        if(!t.s_.compare("p"))
        {
            //<p>
            p("h1", "e_h1");
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
    if(!anchorid.empty())
        pout_->WriteFmt("<h1><span class=\"anchor\"><a href=\"%s\">[&lt;-]</a></span></h1>", anchorid.c_str());
    pout_->WriteStr("</div>\n");

    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::title_info()
{
    s_->BeginNotEmptyElement("title-info");

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
        annotation(true);
    //</annotation>

    //<keywords>
    s_->SkipIfElement("keywords");
    //</keywords>

    //<date>
    if(s_->IsNextElement("date"))
        title_info_date_ = date__epub();
    //<date>

    //<coverpage>
    if(s_->IsNextElement("coverpage"))
        coverpage();
    //</coverpage>

    //<lang>
    lang();
    //</lang>

    s_->SkipRestOfElementContent(); // skip rest of <title-info>
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::tr()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("tr", &attrmap);
    pout_->WriteStr("<tr");

    CopyAttribute("align", attrmap);

    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

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

    pout_->WriteStr("</tr>\n");
    s_->EndElement();
}

//-----------------------------------------------------------------------
void Fb2ParserImpl::v()
{
    AttrMap attrmap;
    if(s_->BeginElement("v", &attrmap))
    {
        pout_->WriteStr("<p class=\"v\"");
        AddId(attrmap);
        CopyXmlLang(attrmap);
        pout_->WriteStr(">");

        ParseTextAndEndElement("v");
        pout_->WriteStr("</p>\n");
    }
}
*/

//-----------------------------------------------------------------------
Ptr<Fb2Parser> FB2TOEPUB_DECL CreateFb2Parser(LexScanner *scanner)
{
    return new Fb2ParserImpl(scanner);
}

};  //namespace Fb2ToEpub
