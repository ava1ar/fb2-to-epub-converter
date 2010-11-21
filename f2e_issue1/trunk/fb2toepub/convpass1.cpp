//
//  Copyright (C) 2010 Alexey Bobkov
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

#include "converter.h"
#include <sstream>
#include <set>

namespace Fb2ToEpub
{


//-----------------------------------------------------------------------
// CONVERTER PASS 1 IMPLEMENTATION
//-----------------------------------------------------------------------
class FB2TOEPUB_DECL ConverterPass1 : public Object, Noncopyable
{
public:
    ConverterPass1(LexScanner *scanner, UnitArray *units) : s_(scanner), units_(units), sectionCnt_(0), textMode_(false), bodyType_(Unit::BODY_NONE) {}

    void Scan();

private:
    Ptr<LexScanner>         s_;
    UnitArray               *units_;
    int                     sectionCnt_;
    bool                    textMode_;
    Unit::BodyType          bodyType_;
    std::set<std::string>   xlns_;      // xlink namespaces
    std::set<std::string>   allRefIds_; // all ref ids

    const std::string* AddId    (const AttrMap &attrmap, bool setFileId = false);
    std::string Findhref        (const AttrMap &attrmap) const;
    void ParseTextAndEndElement (const std::string &element, std::string *plainText);

    // FictionBook elements
    void FictionBook            ();
    void a                      (std::string *plainText);
    void annotation             (bool startUnit = false);
    //void author                 ();
    //void binary                 ();
    void body                   (Unit::BodyType bodyType);
    //void book_name              ();
    //void book_title             ();
    void cite                   ();
    //void city                   ();
    void code                   (std::string *plainText);
    void coverpage              ();
    //void custom_info            ();
    //void date                   ();
    void description            ();
    //void document_info          ();
    //void email                  ();
    void emphasis               (std::string *plainText);
    void empty_line             ();
    void epigraph               ();
    //void first_name             ();
    //void genre                  ();
    //void history                ();
    //void home_page              ();
    //void id                     ();
    //void isbn                   ();
    void image                  (bool in_line, Unit::Type unitType = Unit::UNIT_NONE);
    //void keywords               ();
    //void lang                   ();
    //void last_name              ();
    //void middle_name            ();
    //void nickname               ();
    //void output_document_class  ();
    //void output                 ();
    void p                      (std::string *plainText = NULL);
    //void part                   ();
    void poem                   ();
    //void program_used           ();
    //void publish_info           ();
    //void publisher              ();
    void section                (int parent);
    //void sequence               ();
    //void src_lang               ();
    //void src_ocr                ();
    //void src_title_info         ();
    //void src_url                ();
    void stanza                 ();
    void strikethrough          (std::string *plainText);
    void strong                 (std::string *plainText);
    void style                  (std::string *plainText);
    //void stylesheet             ();
    void sub                    (std::string *plainText);
    void subtitle               (std::string *plainText = NULL);
    void sup                    (std::string *plainText);
    void table                  ();
    void td                     ();
    void text_author            (std::string *plainText = NULL);
    void th                     ();
    void title                  (std::string *plainText = NULL, bool startUnit = false);
    void title_info             ();
    void tr                     ();
    //void translator             ();
    void v                      (std::string *plainText = NULL);
    //void version                ();
    //void year                   ();
};

//-----------------------------------------------------------------------
void ConverterPass1::Scan()
{
    s_->SkipXMLDeclaration();
    FictionBook();
}

//-----------------------------------------------------------------------
const std::string* ConverterPass1::AddId(const AttrMap &attrmap, bool setFileId)
{
    AttrMap::const_iterator cit = attrmap.find("id");
    if(cit == attrmap.end())
        return NULL;

    if(allRefIds_.find(cit->second) != allRefIds_.end())
        return NULL;    // ignore second instance

    units_->back().refIds_.push_back(cit->second);
    if(setFileId)
        units_->back().fileId_ = cit->second;

    return &cit->second;
}

//-----------------------------------------------------------------------
std::string ConverterPass1::Findhref(const AttrMap &attrmap) const
{
    std::set<std::string>::const_iterator cit = xlns_.begin(), cit_end = xlns_.end();
    for(; cit != cit_end; ++cit)
    {
        AttrMap::const_iterator ait = attrmap.find(cit->empty() ? std::string("href") : (*cit)+":href");
        if(ait != attrmap.end())
            return ait->second;
    }
    return "";
}

//-----------------------------------------------------------------------
void ConverterPass1::ParseTextAndEndElement(const std::string &element, std::string *plainText)
{
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
            s_->GetToken();
            units_->back().size_ += t.size_;
            if(plainText)
                *plainText += t.s_;
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <a>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong(plainText);
            else if(!t.s_.compare("emphasis"))
                emphasis(plainText);
            else if(!t.s_.compare("style"))
                style(plainText);
            else if(!t.s_.compare("a"))
                a(plainText);
            else if(!t.s_.compare("strikethrough"))
                strikethrough(plainText);
            else if(!t.s_.compare("sub"))
                sub(plainText);
            else if(!t.s_.compare("sup"))
                sup(plainText);
            else if(!t.s_.compare("code"))
                code(plainText);
            else if(!t.s_.compare("image"))
                image(true);
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <" << element + ">";
                Error(ss.str().c_str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </a>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}


//-----------------------------------------------------------------------
void ConverterPass1::FictionBook()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("FictionBook", &attrmap);

    // namespaces
    AttrMap::const_iterator cit = attrmap.begin(), cit_end = attrmap.end();
    bool has_fb = false, has_emptyfb = false;
    for(; cit != cit_end; ++cit)
    {
        static const std::string xmlns = "xmlns";
        static const std::size_t xmlns_len = xmlns.length();
        static const std::string fbID = "http://www.gribuser.ru/xml/fictionbook/2.0", xlID = "http://www.w3.org/1999/xlink";

        if(!cit->second.compare(fbID))
        {
            if(!cit->first.compare(xmlns))
                has_emptyfb = true;
            else if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                Error("bad FictionBook namespace definition");
            has_fb = true;
        }
        else if(!cit->second.compare(xlID))
        {
            if(cit->first.compare(0, xmlns_len+1, xmlns+":"))
                Error("bad xlink namespace definition");
            xlns_.insert(cit->first.substr(xmlns_len+1));
        }
    }
    if(!has_fb)
        Error("missing FictionBook namespace definition");
    if(!has_emptyfb)
        Error("non-empty FictionBook namespace not implemented");

    //<stylesheet>
    s_->SkipAll("stylesheet");
    //</stylesheet>

    //<description>
    description();
    //</description>

    //<body>
    body(Unit::MAIN);
    if(s_->IsNextElement("body"))
        body(Unit::NOTES);
    if(s_->IsNextElement("body"))
        body(Unit::COMMENTS);
    //</body>
}

//-----------------------------------------------------------------------
void ConverterPass1::a(std::string *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("a", &attrmap);

    std::string id = Findhref(attrmap);
    if(!id.empty() && id[0] == '#')
        units_->back().refs_.insert(id.substr(1));  // collect internal references

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
            s_->GetToken();
            units_->back().size_ += t.size_;
            if(plainText)
                *plainText += t.s_;
            continue;

        case LexScanner::START:
            //<strong>, <emphasis>, <stile>, <strikethrough>, <sub>, <sup>, <code>, <image>
            if(!t.s_.compare("strong"))
                strong(plainText);
            else if(!t.s_.compare("emphasis"))
                emphasis(plainText);
            else if(!t.s_.compare("style"))
                style(plainText);
            else if(!t.s_.compare("strikethrough"))
                strikethrough(plainText);
            else if(!t.s_.compare("sub"))
                sub(plainText);
            else if(!t.s_.compare("sup"))
                sup(plainText);
            else if(!t.s_.compare("code"))
                code(plainText);
            else if(!t.s_.compare("image"))
                image(true);
            else
            {
                std::ostringstream ss;
                ss << "<" << t.s_ << "> unexpected in <a>";
                Error(ss.str().c_str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}

//-----------------------------------------------------------------------
void ConverterPass1::annotation(bool startUnit)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("annotation", &attrmap);
    if(startUnit)
        units_->push_back(Unit(bodyType_, Unit::ANNOTATION, 0, -1));
    AddId(attrmap, startUnit);
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
            Error(ss.str().c_str());
        }
        //</p>, </poem>, </cite>, </subtitle>, </empty-line>, </table>
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::body(Unit::BodyType bodyType)
{
    s_->BeginNotEmptyElement("body");

    bodyType_ = bodyType;

    //<image>
    if(s_->IsNextElement("image"))
        image(false, Unit::IMAGE);
    //</image>

    //<title>
    if(s_->IsNextElement("title"))
    {
        title(NULL, true);
    }
    //</title>

    //<title>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</title>

    do
    {
        //<section>
        section(-1);
        //</section>
    }
    while(s_->IsNextElement("section"));

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::cite()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("cite", &attrmap);
    AddId(attrmap);
    if(!notempty)
        return;

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
            Error(ss.str().c_str());
        }
        //</p>, </subtitle>, </empty-line>, </poem>, </table>
    }

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::code(std::string *plainText)
{
    if(s_->BeginElement("code"))
        ParseTextAndEndElement("code", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::coverpage()
{
    s_->BeginNotEmptyElement("coverpage");
    units_->push_back(Unit(bodyType_, Unit::COVERPAGE, 0, -1));
    do
        image(true);
    while(s_->IsNextElement("image"));
    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::description()
{
    s_->BeginNotEmptyElement("description");
    
    //<title-info>
    title_info();
    //</title-info>

    //<src-title-info>
    if(s_->IsNextElement("src-title-info"))
        s_->SkipElement();
    //</src-title-info>

    s_->SkipRestOfElementContent(); // skip rest of <description>
}

//-----------------------------------------------------------------------
void ConverterPass1::emphasis(std::string *plainText)
{
    if(s_->BeginElement("emphasis"))
        ParseTextAndEndElement("emphasis", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::empty_line()
{
    if(s_->BeginElement("empty-line"))
        s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::epigraph()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("epigraph", &attrmap);
    AddId(attrmap);
    if(!notempty)
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
            Error(ss.str().c_str());
        }
        //</p>, </poem>, </cite>, </empty-line>
    }

    //<text-author>
    while(s_->IsNextElement("text-author"))
        text_author();
    //</text-author>

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::image(bool in_line, Unit::Type unitType)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("image", in_line ? NULL : &attrmap);

    if(unitType != Unit::UNIT_NONE)
        units_->push_back(Unit(bodyType_, unitType, 0, -1));
    if(!in_line)
        AddId(attrmap, unitType != Unit::UNIT_NONE);
    if(notempty)
    {
        ClrScannerDataMode clrDataMode(s_);
        s_->EndElement();
    }
}

//-----------------------------------------------------------------------
void ConverterPass1::p(std::string *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("p", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("p", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::poem()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("poem", &attrmap);
    AddId(attrmap);

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
        s_->SkipElement();
    //</data>

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::section(int parent)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("section", &attrmap);

    int idx = units_->size();
    units_->push_back(Unit(bodyType_, Unit::SECTION, sectionCnt_++, parent));
    const std::string *id = AddId(attrmap, true);
    if(!notempty)
        return;

    //<title>
    if(s_->IsNextElement("title"))
    {
        // check if it has anchor
        if((bodyType_ == Unit::NOTES || bodyType_ == Unit::COMMENTS) && id && !id->empty())
            units_->back().noteRefId_ = *id;

        std::string plainText;
        title(&plainText);
        units_->back().title_ = plainText;
    }
    //</title>

    //<epigraph>
    while(s_->IsNextElement("epigraph"))
        epigraph();
    //</epigraph>

    //<image>
    if(s_->IsNextElement("image"))
        image(false);
    //</image>

    //<annotation>
    if(s_->IsNextElement("annotation"))
        annotation();
    //</annotation>

    if(s_->IsNextElement("section"))
        do
        {
            //<section>
            section(idx);
            //</section>
        }
        while(s_->IsNextElement("section"));
    else
        for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
        {
            //<p>, <image>, <poem>, <subtitle>, <cite>, <empty-line>, <table>
            if(!t.s_.compare("p"))
                p();
            else if(!t.s_.compare("image"))
                image(false);
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
                Error(ss.str().c_str());
            }
            //</p>, </image>, </poem>, </subtitle>, </cite>, </empty-line>, </table>
        }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::stanza()
{
    s_->BeginNotEmptyElement("stanza");

    //<title>
    if(s_->IsNextElement("title"))
        title();
    //</title>

    //<title>
    if(s_->IsNextElement("subtitle"))
        subtitle();
    //</title>

    do
        v();
    while(s_->IsNextElement("v"));

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::strikethrough(std::string *plainText)
{
    if(s_->BeginElement("strikethrough"))
        ParseTextAndEndElement("strikethrough", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::strong(std::string *plainText)
{
    if(s_->BeginElement("strong"))
        ParseTextAndEndElement("strong", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::style(std::string *plainText)
{
    if(s_->BeginElement("style"))
        ParseTextAndEndElement("style", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::sub(std::string *plainText)
{
    if(s_->BeginElement("sub"))
        ParseTextAndEndElement("sub", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::subtitle(std::string *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("subtitle", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("subtitle", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::sup(std::string *plainText)
{
    if(s_->BeginElement("sup"))
        ParseTextAndEndElement("sup", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::table()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("table", &attrmap);
    AddId(attrmap);
    do
    {
        //<tr>
        tr();
        //</tr>
    }
    while(s_->IsNextElement("tr"));
    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::td()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("td", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("td", NULL);
}

//-----------------------------------------------------------------------
void ConverterPass1::text_author(std::string *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("text-author", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("text-author", plainText);
}

//-----------------------------------------------------------------------
void ConverterPass1::th()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("th", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("th", NULL);
}

//-----------------------------------------------------------------------
void ConverterPass1::title(std::string *plainText, bool startUnit)
{
    if(!s_->BeginElement("title"))
        return;

    std::string buf;
    if(startUnit)
    {
        units_->push_back(Unit(bodyType_, Unit::TITLE, 0, -1));
        if(!plainText)
            plainText = &buf;
    }

    for(LexScanner::Token t = s_->LookAhead(); t.type_ == LexScanner::START; t = s_->LookAhead())
    {
        if(!t.s_.compare("p"))
        {
            //<p>
            if(!plainText)
                p();
            else
            {
                std::string text;
                p(&text);
                *plainText = Concat(*plainText, " ", text);
            }
            //</p>
        }
        else if(!t.s_.compare("empty-line"))
        {
            //<empty-line>
            empty_line();
            if(plainText)
                *plainText += " ";
            //</empty-line>
        }
        else
        {
            std::ostringstream ss;
            ss << "<" << t.s_ << "> unexpected in <title>";
            Error(ss.str().c_str());
        }
    }

    if(startUnit)
        units_->back().title_ = *plainText;

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::title_info()
{
    s_->BeginNotEmptyElement("title-info");

    //<genre>
    s_->CheckAndSkipElement("genre");
    s_->SkipAll("genre");
    //</genre>

    //<author>
    s_->CheckAndSkipElement("author");
    s_->SkipAll("author");
    //<author>
    
    //<book-title>
    s_->CheckAndSkipElement("book-title");
    //</book-title>

    //<annotation>
    if(s_->IsNextElement("annotation"))
        annotation(true);
    //</annotation>

    //<keywords>
    if(s_->IsNextElement("keywords"))
        s_->SkipElement();
    //</keywords>

    //<date>
    if(s_->IsNextElement("date"))
        s_->SkipElement();
    //<date>

    //<coverpage>
    if(s_->IsNextElement("coverpage"))
        coverpage();
    //</coverpage>

    s_->SkipRestOfElementContent(); // skip rest of <title-info>
}

//-----------------------------------------------------------------------
void ConverterPass1::tr()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("tr", &attrmap);
    if(!notempty)
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

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass1::v(std::string *plainText)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("v", &attrmap);
    AddId(attrmap);
    if(notempty)
        ParseTextAndEndElement("v", plainText);
}


//-----------------------------------------------------------------------
void FB2TOEPUB_DECL DoConvertionPass1(LexScanner *scanner, UnitArray *units)
{
    Ptr<ConverterPass1> conv = new ConverterPass1(scanner, units);
    conv->Scan();
}


};  //namespace Fb2ToEpub
