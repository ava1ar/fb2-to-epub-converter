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
    //-----------------------------------------------------------------------
    class ScannerImpl : public Fb2ToEpub::LexScanner, public yyFlexLexer, Noncopyable
    {
        Ptr<InStm>	                stm_;
        strvector                   tagStack_;
        std::vector<Token>          tokenStack_;
        bool                        skipMode_;
        bool                        dataMode_;
        int                         doctypeCnt_;
        int                         lineNo_;
        int                         stateCaller_;
        Token                       last_;      // for debug purposes

        Token ScanToken();

    public:
        explicit ScannerImpl(InStm *stm) : stm_(stm), skipMode_(false), dataMode_(false), doctypeCnt_(0), lineNo_(1), stateCaller_(0), last_(STOP) {}

        //-----------------------------------------------------------------------
        //virtual
        Token GetToken()
        {
            while(tokenStack_.size())
            {
                Token t = tokenStack_.back();
                tokenStack_.pop_back();
                if(t.type_ != DATA || dataMode_)
#if defined(_DEBUG)
                    last_ = t;
#endif
                    return t;
            }

            Token t = ScanToken();
            if(t.type_ == DATA)
                for(;;)     // concatenate all DATA together
                {
                    Token t1 = ScanToken();
                    if(t1.type_ != DATA)
                    {
                        UngetToken(t1);
                        break;
                    }

                    t.s_    += t1.s_;
                    t.size_ += t1.size_;
                }

#if defined(_DEBUG)
            last_ = t;
#endif
            return t;
        }
        
        //-----------------------------------------------------------------------
        //virtual
        void UngetToken(const Token &t)
        {
            tokenStack_.push_back(t);
        }

        //-----------------------------------------------------------------------
        //virtual
        bool SetSkipMode(bool newMode)
        {
            bool old = skipMode_;
            skipMode_ = newMode;
            return old;
        }

        //-----------------------------------------------------------------------
        //virtual
        bool SetDataMode(bool newMode)
        {
            bool old = dataMode_;
            dataMode_ = newMode;
            return old;
        }

        //-----------------------------------------------------------------------
        //virtual
        int LexerInput(char* buf, int max_size);
    };
};

#define yyterminate() return Fb2ToEpub::LexScanner::STOP
#define YY_DECL	 Fb2ToEpub::LexScanner::Token Fb2ToEpub::ScannerImpl::ScanToken()

%}


nl              (\r\n|\r|\n)
ws              [ \t]+

xmlstart        "<?xml"
vervalue        \""1."[0-9]+\"|\'"1."[0-9]+\'
encname         [A-Za-z]([A-Za-z0-9._]|"-")*
encvalue        \"{encname}\"|\'{encname}\'
sdname          "yes"|"no"
sdvalue         \"{sdname}\"|\'{sdname}\'
xmlclose        "?>"

chardata        ([^<&\r\n\]]|"]"[^<&\r\n\]]|"]]"[^<&>\r\n])*("]"|"]]")?
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
commentstart    "<!--"
commentend      "-->"
cdatablock      "<![CDATA["([^\]]|"]"[^\]]|"]]"[^>])*"]]>"
xmlreserved     {xmlstart}([^\?]|"?"[^>])*("?")?{xmlclose}

/*
 * The X0-4 modes are used for XML header parsing.
 * The OUTSIDE mode is used outside XML top-level element.
 * The MARKUP mode is used inside markup.
 * The CONTENT mode is used for the content of elements, i.e.,
 * between the ">" and "<" of element tags.
 * The DOCTYPE mode is used to skip DOCTYPE definition.
 */

%s X0 X1 X2 X3 X4
%s OUTSIDE MARKUP CONTENT DOCTYPE
%s COMMENT CDB


%%

    /* XML declaration */

{xmlstart}                      {BEGIN(X0); return XMLDECL;}
<X0>"version"                   {BEGIN(X1);}
<X1>"="                         {BEGIN(X2);}
<X2>{vervalue}                  {
                                    BEGIN(X3);
                                    yytext[yyleng-1] = '\0';
                                    return Token(VERSION, yytext+1);
                                }
<X3>"encoding"                  {return ENCODING;}
<X3>"="                         {return EQ;}
<X3>{encvalue}                  {
                                    BEGIN(X4);
                                    yytext[yyleng-1] = '\0';
                                    return Token(VALUE, yytext+1);
                                }
<X3,X4>"standalone"             {BEGIN(X4); return STANDALONE;}
<X4>"="                         {return EQ;}
<X4>{sdvalue}                   {
                                    yytext[yyleng-1] = '\0';
                                    return Token(VALUE, yytext+1);
                                }
<X4>{xmlclose}                  {BEGIN(OUTSIDE); return CLOSE;}
<X0,X1,X2,X3,X4>{ws}            {}
<X0,X1,X2,X3,X4>{nl}            {++lineNo_;}
<X0,X1,X2,X3,X4>.               {Fb2ToEpub::Error("xml declaration: unexpected character"); yyterminate();}


    /* Ignored or not implemented */

<OUTSIDE>{ws}                   {}
<OUTSIDE>{nl}                   {++lineNo_;}
<CONTENT,OUTSIDE>{cdatablock}   {/* skip CDATA block */}
<CONTENT,OUTSIDE>{xmlreserved}  {/* skip reserved xml element */}
<CONTENT,OUTSIDE>"<!"           {Fb2ToEpub::Error("not implemented 1"); yyterminate();}


    /* Skip comment */

<CONTENT,OUTSIDE>{commentstart} {stateCaller_ = YY_START; BEGIN(COMMENT);}
<COMMENT>[^-\r\n]|"-"[^-\n\r]   {/* eat */}
<COMMENT>{nl}                   {++lineNo_;}
<COMMENT>{commentend}           {BEGIN(stateCaller_);}


    /* Skip DOCTYPE */

<OUTSIDE>"<!DOCTYPE"            {doctypeCnt_ = 1; BEGIN(DOCTYPE);}
<DOCTYPE>"<"                    {++doctypeCnt_;}
<DOCTYPE>">"                    {if(--doctypeCnt_ <= 0) BEGIN(OUTSIDE);}
<DOCTYPE>[^<>\r\n]*             {/* eat */}
<DOCTYPE>{nl}                   {++lineNo_;}


    /* Content */

<CONTENT>{data}	                {
                                    // return DATA only in data mode, otherwise ignore
                                    if(dataMode_)
                                    {
                                        if(skipMode_)
                                            return Token(DATA, yyleng);
                                        //std::vector<char> buf;
                                        //Decode(yytext, &buf, false, false);
                                        //return Token(DATA, &buf[0]);
                                        return Token(DATA, yytext, yyleng);
                                    }
                                }
<CONTENT>{nl}	                {
                                    ++lineNo_;
                                    // return DATA only in data mode, otherwise ignore
                                    if(dataMode_)
                                        return skipMode_ ? Token(DATA, 1) : Token(DATA, "\n", 1);
                                }
<CONTENT,OUTSIDE>{stagstart}    {
                                    char *tagName = &yytext[1];
                                    tagStack_.push_back(tagName);
                                    BEGIN(MARKUP);
                                    return Token(START, tagName);
                                }
<CONTENT>{etagstart}            {
                                    char *tagName = &yytext[2];
                                    if(!tagStack_.size())
                                        Error("tag stack is empty #0");
                                    if(tagStack_.back().compare(tagName))
                                        Error("tag mismatch");
                                    tagStack_.pop_back();
                                    BEGIN(MARKUP);
                                    return Token(END, tagName);
                                }

<OUTSIDE>.                      {/* ignore outside garbage */}
<CONTENT>.                      {
                                    // error character - try to process
                                    if(dataMode_)
                                    {
                                        //Fb2ToEpub::Error("char outside elements"); yyterminate();
                                        const char *p;
                                        switch(yytext[0])
                                        {
                                        case '<':   p = "&lt;"; break;
                                        case '>':   p = "&gt;"; break;
                                        case '&':   p = "&amp;"; break;
                                        case '\'':  p = "&apos;"; break;
                                        case '"':   p = "&quot;"; break;
                                        default:    p = "?"; break;
                                        }
                                        size_t len = strlen(p);
                                        return skipMode_ ? Token(DATA, len) : Token(DATA, p, len);
                                    }
                                }


    /* Markup */

<MARKUP>{ws}                    {}
<MARKUP>{nl}                    {++lineNo_;}
<MARKUP>"="                     {return EQ;}
<MARKUP>{name}	                {return Token(NAME, yytext);}
<MARKUP>{attrvalue}             {
                                    if(skipMode_)
                                        return Token(VALUE);
                                    yytext[yyleng-1] = '\0';
                                    std::vector<char> buf;
                                    Decode(yytext+1, &buf, true, true);
                                    return Token(VALUE, &buf[0]);
                                    //return Token(VALUE, yytext);
                                }
<MARKUP>"/>"                    {
                                    if(!tagStack_.size())
                                        Error("tag stack is empty #1");
                                    tagStack_.pop_back();
                                    BEGIN(tagStack_.size() ? CONTENT : OUTSIDE);
                                    return SLASHCLOSE;
                                }
<MARKUP>">"                     {
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
