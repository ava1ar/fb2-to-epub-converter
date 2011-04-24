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

#include "scanner.h"
#include "streamconv.h"
#include "streamzip.h"
#include "fb2parser.h"
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
static void PrintInfo(const String &name, const String &value)
{
    if(!value.empty())
    {
        std::vector<char> buf;
        LexScanner::Decode(value.c_str(), &buf, true, true);
        printf("%s=%s\n", name.c_str(), &buf[0]);
    }
}

//-----------------------------------------------------------------------
// HANDLERS
//-----------------------------------------------------------------------
class AuthorHandler : public Fb2NoAttrHandler, Noncopyable
{
public:
    AuthorHandler() : pname_(CreateTextEHandler(" ")) {}

    Ptr<Fb2EHandler> GetNameHandler() const {return static_cast<Fb2TextHandler*>(pname_);}
    void Print()
    {
        String authors;
        strvector::const_iterator cit = authors_.begin(), cit_end = authors_.end();
        for(; cit < cit_end; ++cit)
            authors = Concat(authors, ", ", *cit);
        PrintInfo("author", authors);
    }

    //virtuals
    void            Begin(Fb2EType, Fb2Host*)   {pname_->Reset();}
    void            Contents(const String&)     {}
    void            End()                       {authors_.push_back(pname_->Text());}

private:
    Ptr<Fb2TextHandler> pname_;
    strvector           authors_;
};

//-----------------------------------------------------------------------
class SeqAttrHandler : public Fb2AttrHandler, Noncopyable
{
public:
    void Print()
    {
        // sequence, number
        if(!sequences_.empty())
        {
            PrintInfo("sequence", sequences_[0].first);
            PrintInfo("number", sequences_[0].second);

            // if more than one sequence?
            /*
            for(int i = 1; i < sequences_.size(); ++i)
            {
                std::ostringstream index;
                index.width(4);
                index.fill('0');
                index << i;
                PrintInfo(String("sequence") + index.str(), sequences_[i].first);
                PrintInfo(String("number") + index.str(), sequences_[i].second);
            }
            */
        }
    }

    //virtuals
    void Begin(Fb2EType, AttrMap &attrmap, Fb2Host*)
    {
        String name = attrmap["name"];
        if(!name.empty())
            sequences_.push_back(seqvector::value_type(name, attrmap["number"]));
    }
    void            Contents(const String&)     {}
    void            End()                       {}

private:
    typedef std::vector<std::pair<String, String> > seqvector;
    seqvector sequences_;
};

//-----------------------------------------------------------------------
class RootEHandler : public Fb2EHandler
{
public:
    //virtuals
    bool StartTag(Fb2EType, LexScanner *s, Fb2Host*)
    {
        s->BeginNotEmptyElement("FictionBook");
        return false;
    }
    void    Data    (const String &data)    {}
    void    EndTag  (LexScanner *s)         {}
};


//-----------------------------------------------------------------------
void FB2TOEPUB_DECL DoPrintInfo (const String &in)
{
    std::size_t size = 0;
    {
        struct stat st;
        ::stat(in.c_str(), &st);
        size = st.st_size;
    }

    Ptr<Fb2StdCtxt> ctxt = CreateFb2StdCtxt(CreateRecursiveEHandler());

    Ptr<Fb2EHandler> skip = CreateSkipEHandler();
    Ptr<Fb2EHandler> nop = CreateNopEHandler();

    // <title-info><author>
    Ptr<AuthorHandler> author = new AuthorHandler();
    ctxt->RegisterSubHandler(E_AUTHOR, author, true);

    // <title-info><author> contents
    Ptr<Fb2EHandler> authorname = author->GetNameHandler();
    ctxt->RegisterHandler(E_FIRST_NAME,     authorname);
    ctxt->RegisterHandler(E_MIDDLE_NAME,    authorname);
    ctxt->RegisterHandler(E_LAST_NAME,      authorname);
    ctxt->RegisterHandler(E_NICKNAME,       authorname);

    // <title-info><book-title>, <title-info><date>
    String title, date;
    ctxt->RegisterHandler(E_BOOK_TITLE, CreateTextEHandler(&title));
    ctxt->RegisterHandler(E_DATE, CreateTextEHandler(&date));

    // <title-info><lang>
    //String lang;
    //ctxt->RegisterHandler(E_LANG, CreateTextEHandler(&lang));

    // <title-info><translator> - skip (to avoid interference with <author>)
    ctxt->RegisterHandler(E_TRANSLATOR, skip);

    // <title-info><sequence>
    Ptr<SeqAttrHandler> sequence = new SeqAttrHandler();
    ctxt->RegisterSubHandler(E_SEQUENCE, sequence);

    // skip rest without scanning
    ctxt->RegisterHandler(E_SRC_TITLE_INFO, nop);
    ctxt->RegisterHandler(E_DOCUMENT_INFO,  nop);
    ctxt->RegisterHandler(E_PUBLISH_INFO,   nop);
    ctxt->RegisterHandler(E_CUSTOM_INFO,    nop);
    ctxt->RegisterHandler(E_BODY,           nop);
    ctxt->RegisterHandler(E_BINARY,         nop);

    // drop rest of FictionBook contents without scanning
    ctxt->RegisterHandler(E_FICTIONBOOK, Ptr<Fb2EHandler>(new RootEHandler()));

    // parsing
    Ptr<InStm> pin = CreateInUnicodeStm(CreateUnpackStm(in.c_str()));
    CreateFb2Parser(CreateScanner(pin))->Parse(ctxt);

    // print info
    author->Print();                // author
    PrintInfo("title", title);      // title
    PrintInfo("date", date);        // date
    //PrintInfo("lang", lang);      // lang
    {
        // size
        std::ostringstream sizeStr;
        sizeStr << size;
        PrintInfo("size", sizeStr.str());
    }
    sequence->Print();                  // sequence, number
}


};  //namespace Fb2ToEpub
