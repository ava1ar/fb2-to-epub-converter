/*
    Copyright (C) 2010 Alexey Bobkov

    This file is part of Fb2toepub converter.

    Fb2toepub converter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Fb2toepub converter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Fb2toepub converter.  If not, see <http://www.gnu.org/licenses/>.
*/

%option c++ 8bit nodefault noyywrap never-interactive


%{
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "scanner.h"
#include <vector>

namespace Fb2ToEpub
{
    class ScannerImpl : public Fb2ToEpub::LexScanner, public yyFlexLexer, Noncopyable
    {
        Ptr<InStm>	                stm_;
        strvector                   tagStack_;
        std::vector<Token>          tokenStack_;
        bool                        skipMode_;
        bool                        dataMode_;
        int                         doctypeCnt_;
        Token                       last_;      // for debug purposes

        Token ScanToken();

    public:
        explicit ScannerImpl(InStm *stm) : stm_(stm), skipMode_(false), dataMode_(false), doctypeCnt_(0), last_(STOP) {}

        //virtual
        Token GetToken()
        {
            if(!tokenStack_.size())
                return last_ = ScanToken();
            else
            {
                Token t = tokenStack_.back();
                tokenStack_.pop_back();
                return last_ = t;
            }
        }
        
        //virtual
        void UngetToken(const Token &t)
        {
            tokenStack_.push_back(t);
        }

        //virtual
        bool SetSkipMode(bool newMode)
        {
            bool old = skipMode_;
            skipMode_ = newMode;
            return old;
        }

        //virtual
        bool SetDataMode(bool newMode)
        {
            bool old = dataMode_;
            dataMode_ = newMode;
            return old;
        }

        //virtual
        int LexerInput(char* buf, int max_size);
    };
};

#define yyterminate() return Fb2ToEpub::LexScanner::STOP
#define YY_DECL	 Fb2ToEpub::LexScanner::Token Fb2ToEpub::ScannerImpl::ScanToken()

%}


nl              (\r\n|\r|\n)
ws              [ \t\r\n]+

xmlver          "<?xml"{ws}"version"{ws}?"="{ws}?
version         "1."[0-9]+
encoding        "encoding"{ws}?"="{ws}?
standalone      "standalone"{ws}?"="{ws}?
encname         [A-Za-z]([A-Za-z0-9._]|"-")*
sdname          "yes"|"no"
xmlname         {encname}|{sdname}
xmlclose        "?>"

chardata        ([^<&\]]|"]"[^<&\]]|"]]"[^<&>])*("]"|"]]")?
letter          [A-Za-z]
digit           [0-9]
namestart       ({letter}|"_"|":")
namechar        ({letter}|{digit}|"."|"-"|"_"|":")
name            {namestart}{namechar}*
entityref       &{name};
charref         ("&#"[0-9]+;)|("&#x"[0-9a-fA-F]+;)
reference       {entityref}|{charref}
data            {chardata}?({reference}{chardata}?)*

stagstart       "<"{name}
etagstart       "</"{name}
attrvalue       \"([^<&"]|{reference})*\"|\'([^<&']|{reference})*\'

/*
 * Ignored stuff
 */
comment         "<!--"([^-]|"-"[^-])*"-->"
cdatablock      "<![CDATA["([^\]]|"]"[^\]]|"]]"[^>])*"]]>"
xmlreserved     "<?xml"([^\?]|"?"[^>])*("?")?"?>"

/*
 * The CONTENT mode is used for the content of elements, i.e.,
 * between the ">" and "<" of element tags.
 * The INITIAL mode is used outside the top level element
 * and inside markup.
 */

%s XML XML1 XML2
%s OUTSIDE CONTENT DOCTYPE


%%

    /* XML declaration */

{xmlver}                        {BEGIN(XML); return XMLDECL;}
<XML>{encoding}                 {return ENCODING;}
<XML>{standalone}               {return STANDALONE;}
<XML>"'"                        {BEGIN(XML1); return '"';}
<XML>"\""                       {BEGIN(XML2); return '"';}
<XML1,XML2>{version}            {return Token(VERSION, yytext);}
<XML1,XML2>{xmlname}            {return Token(VALUE, yytext);}
<XML1>"'"{ws}?                  {BEGIN(XML); return '"';}
<XML2>"\""{ws}?                 {BEGIN(XML); return '"';}
<XML>{xmlclose}                 {BEGIN(OUTSIDE); return CLOSE;}
<XML,XML1,XML2>.                {Fb2ToEpub::Error("xmldecl: closing expected"); yyterminate();}


    /* Ignored or not implemented */

<CONTENT,OUTSIDE>{comment}      {/* skip comment */}
<OUTSIDE>{ws}                   {/* skip whitespaces outside main element */}
<CONTENT,OUTSIDE>{cdatablock}   {/* skip CDATA block */}
<CONTENT,OUTSIDE>{xmlreserved}  {/* skip reserved xml element */}
<CONTENT,OUTSIDE>"<!"           {Fb2ToEpub::Error("not implemented 1"); yyterminate();}


    /* Skip DOCTYPE */

<OUTSIDE>"<!DOCTYPE"            {doctypeCnt_ = 1; BEGIN(DOCTYPE);}
<DOCTYPE>"<"                    {++doctypeCnt_;}
<DOCTYPE>">"                    {if(--doctypeCnt_ <= 0) BEGIN(OUTSIDE);}
<DOCTYPE>[^<>]*                 {}


    /* Content */

<CONTENT>{data}	                {
                                    // return DATA only in data mode, otherwise ignore
                                    if(dataMode_)
                                    {
                                        if(skipMode_)
                                            return Token(DATA, strlen(yytext));
                                        //std::vector<char> buf;
                                        //Decode(yytext, &buf, false, false);
                                        //return Token(DATA, &buf[0]);
                                        return Token(DATA, yytext, strlen(yytext));
                                    }
                                }
<CONTENT,OUTSIDE>{stagstart}    {
                                    char *tagName = &yytext[1];
                                    tagStack_.push_back(tagName);
                                    BEGIN(INITIAL);
                                    return Token(START, tagName);
                                }
<CONTENT>{etagstart}            {
                                    char *tagName = &yytext[2];
                                    if(!tagStack_.size())
                                        Error("tag stack is empty #0");
                                    if(tagStack_.back().compare(tagName))
                                        Error("tag mismatch");
                                    tagStack_.pop_back();
                                    BEGIN(INITIAL);
                                    return Token(END, tagName);
                                }

<OUTSIDE>.                      {Fb2ToEpub::Error("char outside elements"); yyterminate();}


    /* Markup */

<INITIAL>{ws}                   {/* skip whitespace */}
<INITIAL>"="                    {return EQ;}
<INITIAL>{name}	                {return Token(NAME, yytext);}
<INITIAL>{attrvalue}            {
                                    if(skipMode_)
                                        return Token(VALUE);
                                    yytext[strlen(yytext)-1] = '\0';
                                    std::vector<char> buf;
                                    Decode(yytext+1, &buf, true, true);
                                    return Token(VALUE, &buf[0]);
                                    //return Token(VALUE, yytext);
                                }
<INITIAL>"/>"                   {
                                    if(!tagStack_.size())
                                        Error("tag stack is empty #1");
                                    tagStack_.pop_back();
                                    BEGIN(tagStack_.size() ? CONTENT : OUTSIDE);
                                    return SLASHCLOSE;
                                }
<INITIAL>">"                    {
                                    BEGIN(tagStack_.size() ? CONTENT : OUTSIDE);
                                    return CLOSE;
                                }


    /* Default */

.|{nl}                          {Fb2ToEpub::Error("default: unrecognized char"); yyterminate();}

%%

namespace Fb2ToEpub
{
    int ScannerImpl::LexerInput(char* buf, int max_size)
    {
        return stm_->Read(buf, max_size);
    }


    Ptr<LexScanner> CreateScanner(InStm *stm)
    {
        return new ScannerImpl(stm);
    }
};

int yyFlexLexer::yylex()
{
    return -1;
}
