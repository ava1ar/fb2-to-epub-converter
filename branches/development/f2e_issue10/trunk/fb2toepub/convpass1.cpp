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

#include "converter.h"
#include "fb2parser.h"
#include <sstream>
#include <set>

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
// PASS 1 ENGINE
//-----------------------------------------------------------------------
class Engine : Noncopyable
{
public:
    Engine(UnitArray *units)
        : units_(units), bodyType_(Unit::BODY_NONE), sectionCnt_(0), currSection_(-1)
    {
    }

    const String* AddRefId(Fb2Host *host)
    {
        const AttrMap &attrmap = host->GetAttributes();
        AttrMap::const_iterator cit = attrmap.find("id");
        if(cit == attrmap.end())
            return NULL;

        if(allRefIds_.find(cit->second) != allRefIds_.end())
            return NULL;    // ignore second instance

        allRefIds_.insert(cit->second);
        units_->back().refIds_.push_back(cit->second);
        return &cit->second;
    }

    void AddRef(Fb2Host *host)
    {
        String id = host->Findhref();
        if(!id.empty() && id[0] == '#')
            units_->back().refs_.insert(id.substr(1));  // collect internal references
    }

    void StartUnit(Unit::Type unitType)
    {
#if defined(_DEBUG)
        if(unitType == Unit::SECTION)
            InternalError(__FILE__, __LINE__, "unexpected section unit begin");
#endif
        units_->push_back(Unit(bodyType_, unitType, 0, -1));
    }

    void AddUnitSize(size_t size)
    {
        units_->back().size_ += size;
    }

    void BeginBody()
    {
        switch(bodyType_)
        {
        case Unit::BODY_NONE:   bodyType_ = Unit::MAIN; break;
        case Unit::MAIN:        bodyType_ = Unit::NOTES; break;
        default:                bodyType_ = Unit::COMMENTS; break;
        }
    }

    void BeginSection(Fb2Host *host)
    {
        sections_.push_back(currSection_);
        {
            int parent = currSection_;
            currSection_ = units_->size();
            units_->push_back(Unit(bodyType_, Unit::SECTION, sectionCnt_++, parent));
        }

        const String *id = AddRefId(host);

        // check if it has anchor
        if (host->Scanner()->IsNextElement("title") &&
            (bodyType_ == Unit::NOTES || bodyType_ == Unit::COMMENTS) &&
            id &&
            !id->empty())
        {
            units_->back().noteRefId_ = *id;
        }
    }

    void SetSectionTitle(const String &title)
    {
        units_->back().title_ = title;
    }

    void SwitchUnitIfSizeAbove(std::size_t size)
    {
        if(units_->back().size_ > size)
            units_->push_back(Unit(bodyType_, Unit::SECTION, sectionCnt_++, sections_.back()));
    }

    void EndSection()
    {
        currSection_ = sections_.back();
        sections_.pop_back();
    }

private:
    UnitArray           *units_;
    std::set<String>    allRefIds_;
    Unit::BodyType      bodyType_;
    int                 sectionCnt_;
    int                 currSection_;
    std::vector<int>    sections_;
};


//-----------------------------------------------------------------------
// BASE PASS 1 HANDLER (JUST ADD REF IDS OR HREF IF NECESSARY)
//-----------------------------------------------------------------------
template <bool skipRest = false>
class BaseHandlerP1 : public Fb2BaseEHandler<skipRest>
{
    Engine  *engine_;
    String  *text_;
protected:
    Engine* GetEngine() const   {return engine_;}
    String* GetText() const     {return text_;}
public:
    BaseHandlerP1(Engine *engine, String *text = NULL)
        : engine_(engine), text_(text) {}

    //virtuals
    bool StartTag(Fb2EType type, Fb2Host *host)
    {
        const Fb2ElementInfo *info = Fb2GetElementInfo(type);
        if(info->refid_)
            engine_->AddRefId(host);
        else if(info->ref_)
            engine_->AddRef(host);
        return Fb2BaseEHandler<skipRest>::StartTag(type, host);
    }
    void Data(const String &data, size_t size)
    {
        GetEngine()->AddUnitSize(size);
        if(text_)
            *text_ += data;
        Fb2BaseEHandler<skipRest>::Data(data, size);
    }
};


//-----------------------------------------------------------------------
// HANDLERS TO START UNIT
//-----------------------------------------------------------------------
template <bool skipRest = false>
class StartUnitHandler : public BaseHandlerP1<skipRest>
{
    Unit::Type  unitType_;
public:
    StartUnitHandler(Engine *engine, Unit::Type unitType)
        : BaseHandlerP1<skipRest>(engine), unitType_(unitType) {}

    //virtuals
    bool StartTag(Fb2EType type, Fb2Host *host)
    {
        GetEngine()->StartUnit(unitType_);
        return BaseHandlerP1<skipRest>::StartTag(type, host);
    }
};


//-----------------------------------------------------------------------
// TITLE-INFO HANDLER AND CONTEXT
//-----------------------------------------------------------------------
class TitleInfoCtxt : public Fb2Ctxt, Noncopyable
{
    Ptr<Fb2Ctxt>    oldCtxt_;
    Engine          *engine_;
public:
    TitleInfoCtxt(Fb2Ctxt *oldCtxt, Engine *engine) : oldCtxt_(oldCtxt), engine_(engine) {}

    //virtual
    void GetNext(Fb2EType type, Ptr<Fb2EHandler> *h, Ptr<Fb2Ctxt> *ctxt)
    {
        if(!h)
        {
            oldCtxt_->GetNext(type, NULL, ctxt);
            return;
        }

        switch(type)
        {
        case E_ANNOTATION:
            oldCtxt_->GetNext(type, NULL, ctxt);
            *h = new StartUnitHandler<>(engine_, Unit::ANNOTATION);
            break;

        case E_COVERPAGE:
            oldCtxt_->GetNext(type, NULL, ctxt);
            *h = new StartUnitHandler<>(engine_, Unit::COVERPAGE);
            break;

        default:
            oldCtxt_->GetNext(type, h, ctxt);
            break;
        }
    }
};


//-----------------------------------------------------------------------
// BODY TITLE AND SECTION TITLE HANDLERS AND CONTEXT
//-----------------------------------------------------------------------
class BodyTitle : public StartUnitHandler<>
{
    String *title_;
public:
    BodyTitle(Engine *engine, String *title)
        : StartUnitHandler<>(engine, Unit::TITLE), title_(title) {}

    //virtuals
    bool EndTag(bool empty, Fb2Host *host)
    {
        GetEngine()->SetSectionTitle(*title_);
        title_->clear();
        return StartUnitHandler<>::EndTag(empty, host);
    }
};
//-----------------------------------------------------------------------
class SectionTitle : public BaseHandlerP1<>
{
    String *title_;
public:
    SectionTitle(Engine *engine, String *title)
        : BaseHandlerP1<>(engine), title_(title) {}

    //virtuals
    bool EndTag(bool empty, Fb2Host *host)
    {
        GetEngine()->SetSectionTitle(*title_);
        title_->clear();
        return BaseHandlerP1<>::EndTag(empty, host);
    }
};
//-----------------------------------------------------------------------
class BSTitleCtxt : public Fb2Ctxt, Noncopyable
{
    Engine          *engine_;
    String          *title_;
    String          paragraphBuf_;

    class P : public BaseHandlerP1<>
    {
        String *title_;
    public:
        P(Engine *engine, String *title, String *paragraph)
            : BaseHandlerP1<>(engine, paragraph), title_(title) {}

        //virtuals
        bool EndTag(bool empty, Fb2Host *host)
        {
            *title_ = Concat(*title_, " ", *GetText());
            GetText()->clear();
            return BaseHandlerP1<>::EndTag(empty, host);
        }
    };
    class EmptyLine : public Fb2BaseEHandler<>
    {
        String *title_;
    public:
        EmptyLine(String *title) : title_(title) {}
        //virtuals
        bool EndTag(bool empty, Fb2Host *host)
        {
            *title_ += " ";
            return Fb2BaseEHandler<>::EndTag(empty, host);
        }
    };

public:
    BSTitleCtxt(Engine *engine, String *title) : engine_(engine), title_(title) {}

    //virtual
    void GetNext(Fb2EType type, Ptr<Fb2EHandler> *h, Ptr<Fb2Ctxt> *ctxt)
    {
        if(ctxt)
            *ctxt = this;
        if(!h)
            return;

        switch(type)
        {
        case E_P:           *h = new P(engine_, title_, &paragraphBuf_); return;
        case E_EMPTY_LINE:  *h = new EmptyLine(title_); return;
        default:            *h = new BaseHandlerP1<>(engine_, &paragraphBuf_); return;
        }
    }
};


//-----------------------------------------------------------------------
// BODY HANDLER AND CONTEXT
//-----------------------------------------------------------------------
class Body : public Fb2BaseEHandler<>
{
    Engine  *engine_;
public:
    Body(Engine *engine) : engine_(engine) {}

    //virtuals
    bool StartTag(Fb2EType, Fb2Host*)
    {
        engine_->BeginBody();
        return false;
    }
};
//-----------------------------------------------------------------------
class BodyCtxt : public Fb2Ctxt, Noncopyable
{
    Ptr<Fb2Ctxt>    oldCtxt_;
    Engine          *engine_;
    String          titleBuf_;
public:
    BodyCtxt(Fb2Ctxt *oldCtxt, Engine *engine) : oldCtxt_(oldCtxt), engine_(engine) {}

    //virtual
    void GetNext(Fb2EType type, Ptr<Fb2EHandler> *h, Ptr<Fb2Ctxt> *ctxt)
    {
        switch(type)
        {
        case E_IMAGE:
            oldCtxt_->GetNext(type, NULL, ctxt);
            if(h)
                *h = new StartUnitHandler<>(engine_, Unit::IMAGE);
            return;

        case E_TITLE:
            if(h)
                *h = new BodyTitle(engine_, &titleBuf_);
            if(ctxt)
                *ctxt = new BSTitleCtxt(engine_, &titleBuf_);
            return;

        default:
            oldCtxt_->GetNext(type, h, ctxt);
            return;
        }
    }
};


//-----------------------------------------------------------------------
// SECTION HANDLER AND CONTEXT
//-----------------------------------------------------------------------
class Section : public Fb2BaseEHandler<>
{
    Engine  *engine_;
public:
    Section(Engine *engine) : engine_(engine) {}

    //virtuals
    bool StartTag(Fb2EType, Fb2Host *host)  {engine_->BeginSection(host); return false;}
    bool EndTag(bool, Fb2Host*)             {engine_->EndSection(); return false;}
};
//-----------------------------------------------------------------------
class SectionCtxt : public Fb2Ctxt, Noncopyable
{
    Ptr<Fb2Ctxt>    oldCtxt_;
    Engine          *engine_;
    String          titleBuf_;

    //-----------------------------------------------------------------------
    class SizeSwitch : public BaseHandlerP1<>
    {
        size_t size_;
    public:
        SizeSwitch(Engine *engine, size_t size)
            : BaseHandlerP1<>(engine), size_(size) {}

        // virtuals
        bool StartTag(Fb2EType type, Fb2Host *host)
        {
            if(size_)
                GetEngine()->SwitchUnitIfSizeAbove(size_);
            return BaseHandlerP1<>::StartTag(type, host);
        }
        // virtuals
        bool EndTag(bool empty, Fb2Host *host)
        {
            bool ret = BaseHandlerP1<>::EndTag(empty, host);
            GetEngine()->SwitchUnitIfSizeAbove(MAX_UNIT_SIZE);
            return ret;
        }
    };

    void GetNextForSizeSwitch(Fb2EType type, Ptr<Fb2EHandler> *h, Ptr<Fb2Ctxt> *ctxt, size_t size)
    {
        oldCtxt_->GetNext(type, NULL, ctxt);
        if(h)
            *h = new SizeSwitch(engine_, size);
    }

public:
    //-----------------------------------------------------------------------
    SectionCtxt(Fb2Ctxt *oldCtxt, Engine *engine)
        :   oldCtxt_(oldCtxt), engine_(engine) {}

    //-----------------------------------------------------------------------
    //virtual
    void GetNext(Fb2EType type, Ptr<Fb2EHandler> *h, Ptr<Fb2Ctxt> *ctxt)
    {
        switch(type)
        {
        case E_P:           GetNextForSizeSwitch(type, h, ctxt, 0); return;
        case E_SUBTITLE:    GetNextForSizeSwitch(type, h, ctxt, UNIT_SIZE0); return;
        case E_IMAGE:
        case E_POEM:
        case E_TABLE:       GetNextForSizeSwitch(type, h, ctxt, UNIT_SIZE1); return;
        case E_CITE:
        case E_EMPTY_LINE:  GetNextForSizeSwitch(type, h, ctxt, UNIT_SIZE2); return;

        case E_TITLE:
            if(h)
                *h = new SectionTitle(engine_, &titleBuf_);
            if(ctxt)
                *ctxt = new BSTitleCtxt(engine_, &titleBuf_);
            return;

        default:            oldCtxt_->GetNext(type, h, ctxt); return;
        }
    }
};


//-----------------------------------------------------------------------
// Pass 1
//-----------------------------------------------------------------------
void FB2TOEPUB_DECL DoConvertionPass1(LexScanner *scanner, UnitArray *units)
{
    Engine engine(units);
    Ptr<Fb2StdCtxt> ctxt = CreateFb2StdCtxt();

    Ptr<Fb2EHandler> skip = CreateSkipEHandler();

    // <FictionBook>
    ctxt->RegisterHandler(E_FICTIONBOOK, CreateRootEHandler());

    // <title-info>
    {
        Ptr<Fb2Ctxt> pc = new TitleInfoCtxt(ctxt, &engine);
        ctxt->RegisterCtxt(E_TITLE_INFO, pc);
    }

    // skip rest of <description>
    ctxt->RegisterHandler(E_SRC_TITLE_INFO, skip);
    ctxt->RegisterHandler(E_DOCUMENT_INFO,  skip);
    ctxt->RegisterHandler(E_PUBLISH_INFO,   skip);
    ctxt->RegisterHandler(E_CUSTOM_INFO,    skip);

    // <body>
    {
        Ptr<Fb2Ctxt> pc = new BodyCtxt(ctxt, &engine);
        Ptr<Fb2EHandler> ph = new Body(&engine);
        ctxt->RegisterCtxtHandler(E_BODY, pc, ph);
    }

    // <section>
    {
        Ptr<Fb2Ctxt> pc = new SectionCtxt(ctxt, &engine);
        Ptr<Fb2EHandler> ph = new Section(&engine);
        ctxt->RegisterCtxtHandler(E_SECTION, pc, ph);
    }

    // refids, refs, unit sizes 
    {
        Ptr<Fb2EHandler> ph = new BaseHandlerP1<>(&engine);
        ctxt->RegisterHandler(E_A,                  ph);
        ctxt->RegisterHandler(E_ANNOTATION,         ph);
        ctxt->RegisterHandler(E_CITE,               ph);
        ctxt->RegisterHandler(E_CODE,               ph);
        ctxt->RegisterHandler(E_EMPHASIS,           ph);
        ctxt->RegisterHandler(E_EPIGRAPH,           ph);
        ctxt->RegisterHandler(E_HISTORY,            ph);
        ctxt->RegisterHandler(E_IMAGE,              ph);
        ctxt->RegisterHandler(E_IMAGE_SECTION_TOP,  ph);
        ctxt->RegisterHandler(E_P,                  ph);
        ctxt->RegisterHandler(E_POEM,               ph);
        ctxt->RegisterHandler(E_STRIKETHROUGH,      ph);
        ctxt->RegisterHandler(E_STRONG,             ph);
        ctxt->RegisterHandler(E_STYLE,              ph);
        ctxt->RegisterHandler(E_SUB,                ph);
        ctxt->RegisterHandler(E_SUBTITLE,           ph);
        ctxt->RegisterHandler(E_SUP,                ph);
        ctxt->RegisterHandler(E_TABLE,              ph);
        ctxt->RegisterHandler(E_TD,                 ph);
        ctxt->RegisterHandler(E_TEXT_AUTHOR,        ph);
        ctxt->RegisterHandler(E_TH,                 ph);
        ctxt->RegisterHandler(E_V,                  ph);
    }

    // skip rest of <FictionBook>
    ctxt->RegisterHandler(E_BINARY, CreateExitEHandler());

    // parsing
    CreateFb2Parser(scanner)->Parse(ctxt);
}


};  //namespace Fb2ToEpub
