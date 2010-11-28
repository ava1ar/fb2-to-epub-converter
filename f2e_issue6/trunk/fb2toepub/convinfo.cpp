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

#include "fb2toepubconv.h"
#include "scanner.h"
#include "converter.h"
#include <vector>

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
class ConverterInfo : public Object, Noncopyable
{
public:
    ConverterInfo (LexScanner *scanner) : s_(scanner) {}

    void Scan()
    {
        s_->SkipXMLDeclaration();
        FictionBook();

        // author(s)
        String authors;
        {
            strvector::const_iterator cit = authors_.begin(), cit_end = authors_.end();
            for(; cit < cit_end; ++cit)
                authors = Concat(authors, ", ", *cit);
        }

        printf("author=%s\n", authors.c_str());
        printf("title=%s\n", title_.c_str());
        if(!title_info_date_.empty())
            printf("date=%s\n", title_info_date_.c_str());
    }

private:
    Ptr<LexScanner>         s_;
    String                  title_, lang_, title_info_date_, isbn_;
    strvector               authors_;

    // FictionBook elements
    void FictionBook            ();
    //void a                      ();
    //void annotation             ();
    void author                 ();
    //void binary                 ();
    //void body                   ();
    //void book_name              ();
    void book_title             ();
    //void cite                   ();
    //void city                   ();
    //void code                   ();
    //void coverpage              ();
    //void custom_info            ();
    //void date                   ();
    String date__textonly       ();
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
    String isbn                 ();
    //void keywords               ();
    void lang                   ();
    //void last_name              ();
    //void middle_name            ();
    //void nickname               ();
    //void output_document_class  ();
    //void output                 ();
    //void p                      ();
    //void part                   ();
    //void poem                   ();
    //void program_used           ();
    void publish_info           ();
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
    void title_info             ();
    //void tr                     ();
    //void translator             ();
    //void v                      ();
    //void version                ();
    //void year                   ();
};

//-----------------------------------------------------------------------
void ConverterInfo::FictionBook()
{
    s_->BeginNotEmptyElement("FictionBook");

    //<stylesheet>
    s_->SkipAll("stylesheet");
    //</stylesheet>

    //<description>
    description();
    //</description>
}

//-----------------------------------------------------------------------
void ConverterInfo::author()
{
    s_->BeginNotEmptyElement("author");

    String author;
    if(s_->IsNextElement("first-name"))
    {
        author = s_->SimpleTextElement("first-name");

        if(s_->IsNextElement("middle-name"))
            author = Concat(author, " ", s_->SimpleTextElement("middle-name"));

        author = Concat(author, " ", s_->SimpleTextElement("last-name"));
    }
    else if(s_->IsNextElement("nickname"))
        author = s_->SimpleTextElement("nickname");
    else
        Error("<first-name> or <nickname> expected");

    authors_.push_back(author);
    s_->SkipRestOfElementContent();
}

//-----------------------------------------------------------------------
void ConverterInfo::book_title()
{
    title_ = s_->SimpleTextElement("book-title");
}

//-----------------------------------------------------------------------
String ConverterInfo::date__textonly()
{
    String text;

    s_->BeginNotEmptyElement("date");
    SetScannerDataMode setDataMode(s_);
    if(s_->LookAhead().type_ == LexScanner::DATA)
        text = s_->GetToken().s_;
    s_->EndElement();
    return text;
}

//-----------------------------------------------------------------------
void ConverterInfo::description()
{
    s_->BeginNotEmptyElement("description");

    //<title-info>
    title_info();
    //</title-info>

    //<src-title-info>
    if(s_->IsNextElement("src-title-info"))
        s_->SkipElement();
    //</src-title-info>

    //<document-info>
    s_->CheckAndSkipElement("document-info");
    //</document-info>

    //<publish-info>
    if(s_->IsNextElement("publish-info"))
        publish_info();
    //</publish-info>

    s_->SkipRestOfElementContent(); // skip rest of <description>
}

//-----------------------------------------------------------------------
String ConverterInfo::isbn()
{
    String text;

    s_->BeginNotEmptyElement("isbn");
    SetScannerDataMode setDataMode(s_);
    if(s_->LookAhead().type_ == LexScanner::DATA)
        text = s_->GetToken().s_;
    s_->EndElement();
    return text;
}

//-----------------------------------------------------------------------
void ConverterInfo::lang()
{
    lang_ = s_->SimpleTextElement("lang");
}

//-----------------------------------------------------------------------
void ConverterInfo::publish_info()
{
    if(!s_->BeginElement("publish-info"))
        return;

    //<book-name>
    if(s_->IsNextElement("book-name"))
        s_->SkipElement();
    //</book-name>

    //<publisher>
    if(s_->IsNextElement("publisher"))
        s_->SkipElement();
    //</publisher>

    //<city>
    if(s_->IsNextElement("city"))
        s_->SkipElement();
    //</city>

    //<year>
    if(s_->IsNextElement("year"))
        s_->SkipElement();
    //</year>

    //<isbn>
    if(s_->IsNextElement("isbn"))
        isbn_ = isbn();
    //</isbn>

    s_->SkipRestOfElementContent(); // skip rest of <publish-info>
}

//-----------------------------------------------------------------------
void ConverterInfo::title_info()
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
        s_->SkipElement();
    //</annotation>

    //<keywords>
    if(s_->IsNextElement("keywords"))
        s_->SkipElement();
    //</keywords>

    //<date>
    if(s_->IsNextElement("date"))
        title_info_date_ = date__textonly();
    //<date>

    //<coverpage>
    if(s_->IsNextElement("coverpage"))
        s_->SkipElement();
    //</coverpage>

    //<lang>
    lang();
    //</lang>

    s_->SkipRestOfElementContent(); // skip rest of <title-info>
}

void FB2TOEPUB_DECL DoPrintInfo (LexScanner *scanner)
{
    Ptr<ConverterInfo> conv = new ConverterInfo(scanner);
    conv->Scan();
}


};  //namespace Fb2ToEpub
