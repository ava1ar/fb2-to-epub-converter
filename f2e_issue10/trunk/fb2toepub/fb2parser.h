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


#ifndef FB2TOEPUB__FB2PARSER_H
#define FB2TOEPUB__FB2PARSER_H

#include "scanner.h"

namespace Fb2ToEpub
{

    //-----------------------------------------------------------------------
    // All elements
    //-----------------------------------------------------------------------
    enum ElementType
    {
        E_FICTIONBOOK,
        E_A,
        E_ANNOTATION,
        E_AUTHOR,
        E_BINARY,
        E_BODY,
        E_BOOK_NAME,
        E_BOOK_TITLE,
        E_CITE,
        E_CITY,
        E_CODE,
        E_COVERPAGE,
        E_CUSTOM_INFO,
        E_DATE,
        E_DESCRIPTION,
        E_DOCUMENT_INFO,
        E_EMAIL,
        E_EMPHASIS,
        E_EMPTY_LINE,
        E_EPIGRAPH,
        E_FIRST_NAME,
        E_GENRE,
        E_HISTORY,
        E_HOME_PAGE,
        E_ID,
        E_ISBN,
        E_IMAGE,
        E_KEYWORDS,
        E_LANG,
        E_LAST_NAME,
        E_MIDDLE_NAME,
        E_NICKNAME,
        E_OUTPUT_DOCUMENT_CLASS,
        E_OUTPUT,
        E_P,
        E_PART,
        E_POEM,
        E_PROGRAM_USED,
        E_PUBLISH_INFO,
        E_PUBLISHER,
        E_SECTION,
        E_SEQUENCE,
        E_SRC_LANG,
        E_SRC_OCR,
        E_SRC_TITLE_INFO,
        E_SRC_URL,
        E_STANZA,
        E_STRIKETHROUGH,
        E_STRONG,
        E_STYLE,
        E_STYLESHEET,
        E_SUB,
        E_SUBTITLE,
        E_SUP,
        E_TABLE,
        E_TD,
        E_TEXT_AUTHOR,
        E_TH,
        E_TITLE,
        E_TITLE_INFO,
        E_TR,
        E_TRANSLATOR,
        E_V,
        E_VERSION,
        E_YEAR,
        
        E_COUNT
    };

    //-----------------------------------------------------------------------
    class ElementHandler : public Object
    {
    public:
        virtual void    Start(const AttrMap *attrmap)   = 0;
        virtual void    Contents(const String &data)    = 0;
        virtual void    End()                           = 0;
        virtual void    Error(const String &what)       = 0;
    };

    //-----------------------------------------------------------------------
    class Fb2Parser : public Object
    {
    public:
        virtual void Register(ElementType type, ElementHandler *h)  = 0;
        virtual void Parse()                                        = 0;
    };

    //-----------------------------------------------------------------------
    Ptr<Fb2Parser> FB2TOEPUB_DECL   CreateFb2Parser(LexScanner *scanner);

};  //namespace Fb2ToEpub

#endif
