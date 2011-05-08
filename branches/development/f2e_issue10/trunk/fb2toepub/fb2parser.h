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
    enum Fb2EType
    {
        E_ANY = -1,

        E_FICTIONBOOK = 0,
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
        E_IMAGE,            // <image> in <body>, <section> (except top image)
        E_IMAGE_INLINE,     // <image> in <coverpage>, <p>, <a>, <v>, <subtitle>, <th>, <td>, <text-author>
        E_IMAGE_SECTION_TOP,// <image> in the top of the <section>
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
        E_PUBLISHER_PI,     // <publisher> in <publish-info>
        E_PUBLISHER_DI,     // <publisher> in <document-info>
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
    // NAMESPACE LOOKUP
    // (Curremtly supports only "href" resolution. Eventually can support
    // namespaces for all elements and attributes.)
    //-----------------------------------------------------------------------
    class Fb2NsLookup : public Object
    {
    public:
        virtual String Findhref(const AttrMap &attrmap) const = 0;
    };

    //-----------------------------------------------------------------------
    // HOST FOR ELEMENT HANDLER
    //-----------------------------------------------------------------------
    class Fb2Host
    {
    public:
        virtual LexScanner* Scanner() const                         = 0;
        virtual size_t      GetTypeStackSize() const                = 0;
        virtual Fb2EType    GetTypeStackAt(int i) const             = 0;
        virtual String      Findhref(const AttrMap &attrmap) const  = 0;

        virtual void        RegisterNsLookup(Fb2NsLookup *lookup)   = 0;    // registered by FictionBook handler

        //helpers
        void Error(const String &what)
        {
            Scanner()->Error(what);
        }
        Fb2EType GetParentType() const
        {
            return GetTypeStackAt(GetTypeStackSize() - 1);
        }
    };

    //-----------------------------------------------------------------------
    // ELEMENT HANDLER
    //-----------------------------------------------------------------------
    class Fb2EHandler : public Object
    {
    public:
        virtual bool            StartTag(Fb2EType type, LexScanner *s, Fb2Host *host)   = 0;
        virtual void            Data    (const String &data, size_t size)               = 0;
        virtual void            EndTag  (LexScanner *s)                                 = 0;
    };

    //-----------------------------------------------------------------------
    // SUB-HANDLER WITH PROCESSING OF ATTRIBUTES
    //-----------------------------------------------------------------------
    class Fb2AttrHandler : public Object
    {
    public:
        virtual void Begin      (Fb2EType type, AttrMap &attrmap, Fb2Host *host)    = 0;
        virtual void Contents   (const String &data, size_t size)                   = 0;
        virtual void End        ()                                                  = 0;
    };
    //-----------------------------------------------------------------------
    // SUB-HANDLER WITHOUT PROCESSING OF ATTRIBUTES
    //-----------------------------------------------------------------------
    class Fb2NoAttrHandler : public Object
    {
    public:
        virtual void Begin      (Fb2EType type, Fb2Host *host)      = 0;
        virtual void Contents   (const String &data, size_t size)   = 0;
        virtual void End        ()                                  = 0;
    };

    // create Fb2EHandler object from (user-implemented) sub-handler object
    Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateEHandler(Ptr<Fb2AttrHandler> ph, bool skipRest = false);
    Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateEHandler(Ptr<Fb2NoAttrHandler> ph, bool skipRest = false);


    //-----------------------------------------------------------------------
    // CONTEXT
    //-----------------------------------------------------------------------
    class Fb2Ctxt : public Object
    {
    public:
        virtual Ptr<Fb2EHandler>    GetHandler(Fb2EType type)           = 0;
        virtual Ptr<Fb2Ctxt>        GetCtxt(Fb2EType type)              = 0;
    };


    //-----------------------------------------------------------------------
    // FB2 SYNTAX PARSER
    //-----------------------------------------------------------------------
    class Fb2Parser : public Object
    {
    public:
        virtual void Parse(Fb2Ctxt *ctxt) = 0;
    };
    //-----------------------------------------------------------------------
    Ptr<Fb2Parser> FB2TOEPUB_DECL CreateFb2Parser(LexScanner *scanner);


    //-----------------------------------------------------------------------
    // HELPER STANDARD CONTEXT
    //-----------------------------------------------------------------------
    class Fb2StdCtxt : public Fb2Ctxt
    {
    public:
        virtual void RegisterCtxt   (Fb2EType type, Fb2Ctxt *ctxt)  = 0;
        virtual void RegisterHandler(Fb2EType type, Fb2EHandler *h) = 0;

        // helpers
        void RegisterCtxtHandler(Fb2EType type, Fb2Ctxt *ctxt, Fb2EHandler *h)
        {
            RegisterCtxt(type, ctxt);
            RegisterHandler(type, h);
        }
        void RegisterSubHandler(Fb2EType type, Fb2AttrHandler *sh, bool skipRest = false)
        {
            RegisterHandler(type, CreateEHandler(sh, skipRest));
        }
        void RegisterSubHandler(Fb2EType type, Fb2NoAttrHandler *sh, bool skipRest = false)
        {
            RegisterHandler(type, CreateEHandler(sh, skipRest));
        }
        void RegisterCtxtSubHandler(Fb2EType type, Fb2Ctxt *ctxt, Fb2AttrHandler *sh, bool skipRest = false)
        {
            RegisterCtxt(type, ctxt);
            RegisterHandler(type, CreateEHandler(sh, skipRest));
        }
        void RegisterCtxtSubHandler(Fb2EType type, Fb2Ctxt *ctxt, Fb2NoAttrHandler *sh, bool skipRest = false)
        {
            RegisterCtxt(type, ctxt);
            RegisterHandler(type, CreateEHandler(sh, skipRest));
        }
    };
    //-----------------------------------------------------------------------
    // Return the implementation of Fb2StdCtxt.
    // By default, all handlers returned by RegisterHandler are set to default handler,
    // and all ctxts returned by RegisterCtxt are set to "this".
    Ptr<Fb2StdCtxt> FB2TOEPUB_DECL CreateFb2StdCtxt();


    //-----------------------------------------------------------------------
    // HELPER INTERFACES AND METHODS
    // (helps to implement different kinds of Fb2EHandler)
    //-----------------------------------------------------------------------

    //-----------------------------------------------------------------------
    // NAME BY TYPE
    const String& FB2TOEPUB_DECL Fb2EName(Fb2EType type);

    //-----------------------------------------------------------------------
    // HANDLER FOR RECIRSIVE PROCESSING
    // (Do nothing but allow recursive processing.)
    //
    // HANDLER TO SKIP WHOLE ELEMENT CONTENTS WITH XMP PARSING
    // (Do nothing and skip the whole XML element (i.e. no recursive processing).
    //
    // HANDLER TO SKIP WHOLE ELEMENT CONTENTS WITHOUT XMP PARSING
    // (Do nothing, don't parse at all.)
    //-----------------------------------------------------------------------
    Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateRecursiveEHandler();
    Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateSkipEHandler();
    Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateNopEHandler();

    //-----------------------------------------------------------------------
    // SIMPLE TEXT HANDLER
    //-----------------------------------------------------------------------
    class Fb2TextHandler : public Fb2EHandler
    {
    public:
        virtual void            Reset()         = 0;
        virtual const String&   Text() const    = 0;
    };

    //-----------------------------------------------------------------------
    Ptr<Fb2TextHandler> FB2TOEPUB_DECL CreateTextEHandler(const String &concatDivider = "", String *text = NULL);
    Ptr<Fb2TextHandler> FB2TOEPUB_DECL CreateTextEHandler(String *text);


    //-----------------------------------------------------------------------
    // STANDARD ROOT ELEMENT HANDLER
    // It is a handler of "FictionBook" element. It processes XMP namespaces
    // and registers appropriate namespace lookup object in host.
    // (Note that by default fb2 parser doesn't have special FictionBook handler
    // processing namespaces!)
    //-----------------------------------------------------------------------
    Ptr<Fb2EHandler> FB2TOEPUB_DECL CreateRootEHandler();


};  //namespace Fb2ToEpub

#endif
