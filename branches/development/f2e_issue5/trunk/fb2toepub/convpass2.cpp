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
#include "scandir.h"
#include "converter.h"
#include "base64.h"
//#include <streambuf>
#include <sstream>
#include <vector>
#include <map>
#include <set>

namespace Fb2ToEpub
{

const std::size_t THRESHOLD_SIZE = 0x6000UL;


// SOME DAY I WILL IMPLEMENT IT (MAYBE)
/*
class OStreambuf : public std::basic_streambuf<char>
{
public:
    explicit OStreambuf(OutStm *pout, std::size_t buff_sz = 256) :
        buffer_(buff_sz + 1)
    {
        char *base = &buffer_.front();
        setp(base, base + buffer_.size() - 1); // -1 to make overflow() easier
    }

private:
    int_type overflow(int_type ch);
    int sync();

private:
    std::vector<char> buffer_;
};
*/


//-----------------------------------------------------------------------
static String EncodeStr(const String &str)
{
    std::vector<char> buf;
    LexScanner::Encode(str.c_str(), &buf);
    return &buf[0];
}

//-----------------------------------------------------------------------
static void AddContentManifestFile(OutPackStm *pout, const String &id, const String &ref, const String &media_type)
{
    pout->WriteFmt("    <item id=\"%s\" href=\"%s\" media-type=\"%s\"/>\n", EncodeStr(id).c_str(), EncodeStr(ref).c_str(), EncodeStr(media_type).c_str());
}

//-----------------------------------------------------------------------
class ConverterPass2 : public Object, Noncopyable
{
public:
    ConverterPass2 (LexScanner *scanner,
                    const strvector &css,
                    const strvector &fonts,
                    XlitConv *xlitConv,
                    UnitArray *units,
                    OutPackStm *pout)
                        :   s_                  (scanner),
                            css_                (css),
                            fonts_              (fonts),
                            units_              (*units),
                            pout_               (pout),
                            tocLevels_          (0),
                            uniqueIdIdx_        (0),
                            unitIdx_            (0),
                            unitActive_         (false),
                            unitHasId_          (false),
                            xlitConv_           (xlitConv)
    {
        coverPgIt_ = units_.end();
    }

    void Scan()
    {
        BuildOutputLayout();
        {
            std::set<String> noteRefIds;
            BuildReferenceMaps(&noteRefIds);
            BuildAnchors(noteRefIds);
        }

#if 0
#if defined(_DEBUG)
        {
            for(std::size_t i = 0; i < units_.size(); ++i)
                printf ("%d %d-%d-%d %s size=%d, parent=%d, level = %d, %s.xhtml, noteRefId = \"%s\"\n", i, units_[i].bodyType_, units_[i].type_,
                        units_[i].id_, units_[i].title_.c_str(), units_[i].size_, units_[i].parent_, units_[i].level_, units_[i].file_.c_str(), units_[i].noteRefId_.c_str());

            for(ReferenceMap::const_iterator cit = refidToNew_.begin(), cit_end = refidToNew_.end(); cit != cit_end; ++cit)
                printf("%s -> %s\n", cit->first.c_str(), cit->second.c_str());
            for(RefidInfoMap::const_iterator cit = refidToUnit_.begin(), cit_end = refidToUnit_.end(); cit != cit_end; ++cit)
                printf("%s in %s\n", cit->first.c_str(), cit->second->file_.c_str());
            for(ReferenceMap::const_iterator cit = noteidToAnchorId_.begin(), cit_end = noteidToAnchorId_.end(); cit != cit_end; ++cit)
                printf("%s -> anchor %s\n", cit->first.c_str(), cit->second.c_str());
        }
#endif
#endif

        AddMimetype();
        AddContainer();

        // add style and fonts
        AddStyles();
        AddFonts("ttf", &ttffiles_);
        AddFonts("otf", &otffiles_);

        s_->SkipXMLDeclaration();
        FictionBook();

        MakeCoverPageFirst();
        AddContentOpf();
        AddTocNcx();
    }

private:
    Ptr<LexScanner>         s_;
    const strvector         &css_, &fonts_;
    UnitArray               &units_;
    Ptr<OutPackStm>         pout_;

    struct Binary
    {
        String file_, type_;
        Binary() {}
        Binary(const String &file, const String type) : file_(file), type_(type) {}
    };
    typedef std::vector<Binary> binvector;

    typedef std::map<String, const Unit*> RefidInfoMap;    // reference id -> Unit containing this id

    int                     tocLevels_;
    UnitArray::iterator     coverPgIt_;
    int                     uniqueIdIdx_;
    ReferenceMap            refidToNew_;        // (re)mapping of original reference id to unique reference id
    RefidInfoMap            refidToUnit_;       // mapping unique reference id to unit containing this id
    ReferenceMap            noteidToAnchorId_;  // mapping of note ref id to anchor ref id
    std::set<String>        usedAnchorsids_;    // anchor ids already set
    strvector               cssfiles_, ttffiles_, otffiles_;
    binvector               binaries_;
    std::set<String>        xlns_;              // xlink namespaces
    std::set<String>        allRefIds_;         // all ref ids
    String                  authors_, title_, lang_, id_;

    String                  prevUnitFile_;
    int                     unitIdx_;
    bool                    unitActive_;
    bool                    unitHasId_;
    Ptr<XlitConv>           xlitConv_;


    void AdjustUnitSizes        ();
    void CalcTocLevels          ();
    int CalcLevelToSplit        ();
    String MakeUniqueId         (bool anchor = false);
    void BuiltFileLayout        (int levelToSplit);
    void BuildOutputLayout      ();
    void BuildReferenceMaps     (std::set<String> *noteRefIds);
    void BuildAnchors           (const std::set<String> &noteRefIds);

    String Findhref             (const AttrMap &attrmap) const;

    void StartUnit              (Unit::Type unitType, AttrMap *attrmap = NULL);
    void EndUnit                ();

    void AddMimetype            ();
    void AddContainer           ();
    void AddStyles              ();
    void AddFonts               (const char *ext, strvector *fontfiles);
    void MakeCoverPageFirst     ();
    void AddContentOpf          ();
    void AddTocNcx              ();
    const String* AddId         (const AttrMap &attrmap);
    void ParseTextAndEndElement (const String &element);
    void CopyAttribute          (const String &attr, const AttrMap &attrmap);
    bool AddAnchorid            (const String &anchorid);

    // FictionBook elements
    void FictionBook            ();
    void a                      ();
    void annotation             (bool startUnit = false);
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
    //void first_name             ();
    //void genre                  ();
    //void history                ();
    //void home_page              ();
    void id                     ();
    //void isbn                   ();
    void image                  (bool fb2_inline, bool html_inline, bool scale, Unit::Type unitType = Unit::UNIT_NONE);
    //void keywords               ();
    void lang                   ();
    //void last_name              ();
    //void middle_name            ();
    //void nickname               ();
    //void output_document_class  ();
    //void output                 ();
    void p                      (const char *pelement = "p", const char *cls = NULL);
    //void part                   ();
    void poem                   ();
    //void program_used           ();
    //void publish_info           ();
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
    void title                  (bool startUnit, const String &anchorid = "");
    void title_info             ();
    void tr                     ();
    //void translator             ();
    void v                      ();
    //void version                ();
    //void year                   ();
};

//-----------------------------------------------------------------------
void ConverterPass2::AdjustUnitSizes()
{
    for(int i = units_.size(); --i >= 0;)
    {
        Unit &unit = units_[i];
        if(unit.parent_ < 0)
            continue;
#if defined(_DEBUG)
        if(unit.parent_ > i)
            Error("internal error - wrong unit order");
#endif
        units_[unit.parent_].size_ += unit.size_;
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::CalcTocLevels()
{
    int levels = 0;
    UnitArray::iterator it = units_.begin(), it_end = units_.end();
    for(; it < it_end; ++it)
    {
#if !FB2TOEPUB_SUPPRESS_EMPTY_TITLES
        if(it->title_.empty() && it->bodyType_ != Unit::BODY_NONE)
            it->title_ = "- - - - -";
#endif
        if(it->parent_ < 0)
            it->level_ = 0;
        else
        {
            int parentLevel = units_[it->parent_].level_;
            int level = units_[it->parent_].title_.empty() ? parentLevel : parentLevel + 1;
            it->level_ = level;
            if(levels < level)
                levels = level;
        }
    }

    tocLevels_ = levels+1;
}

typedef std::vector<std::size_t> SizeVector;

//-----------------------------------------------------------------------
int ConverterPass2::CalcLevelToSplit()
{
    // calc max size for each level
    SizeVector maxLevelSize(tocLevels_, 0);
    {
        UnitArray::iterator it = units_.begin(), it_end = units_.end();
        for(; it < it_end; ++it)
        {
#if defined(_DEBUG)
            if(it->level_ >= tocLevels_)
                Error("internal error: incorrect level");
#endif
            if(maxLevelSize[it->level_] < it->size_)
                maxLevelSize[it->level_] = it->size_;
        }
    }

    for(int i = maxLevelSize.size(); --i >= 0;)
        if(maxLevelSize[i] > THRESHOLD_SIZE)
            return i;
    return 0;
}

//-----------------------------------------------------------------------
static String MakeFileName(const String &prefix, int idx)
{
    std::ostringstream fileName;
    fileName << prefix;
    fileName.width(4);
    fileName.fill('0');
    fileName << idx;
    return fileName.str();
}

//-----------------------------------------------------------------------
String ConverterPass2::MakeUniqueId(bool anchor)
{
    std::ostringstream fileId;
    fileId << (anchor ? "anchor" : "id") << uniqueIdIdx_++;
    return fileId.str();
}

//-----------------------------------------------------------------------
void ConverterPass2::BuiltFileLayout(int levelToSplit)
{
    // find cover page
    UnitArray::iterator it = units_.begin(), it_end = units_.end();
    for(; it < it_end; ++it)
        switch(it->type_)
        {
        case Unit::COVERPAGE:   coverPgIt_ = it; break;
        //case Unit::IMAGE:       coverPgIt_ = it; break;
        case Unit::SECTION:     break;
        default:                continue;
        }

    // build layout
    int fileIdx = 0;
    String file;
    int prevLevel = -1;
    Unit::Type prevType = Unit::UNIT_NONE;
    for(it = units_.begin(); it < it_end; ++it)
    {
#if FB2TOEPUB_TOC_REFERS_FILES_ONLY
        if ((it->type_ != prevType && (prevType != Unit::TITLE || it->type_ != Unit::SECTION)) || it->level_ <= levelToSplit)
#else
        if ((it->type_ != prevType && (prevType != Unit::TITLE || it->type_ != Unit::SECTION)) ||
            (it->level_ <= levelToSplit && prevLevel >= levelToSplit) ||
            (it->level_ <= prevLevel && prevLevel <= levelToSplit))
#endif
        {
            if(it == coverPgIt_)
                file = "cover";
            else
                file = MakeFileName("txt", fileIdx++);
        }

#if FB2TOEPUB_TOC_REFERS_FILES_ONLY
        // force exclusion from TOC for all units above split level
        if(it->level_ > levelToSplit)
            it->title_.clear();
#endif

        // assing unit file name
        it->file_ = file;

        // make unique id
        String id = MakeUniqueId();

#if !FB2TOEPUB_TOC_REFERS_FILES_ONLY
        // assing new unit id
        it->fileId_ = id;
#endif

        prevLevel = it->level_;
        prevType = it->type_;
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::BuildOutputLayout()
{
    // adjust sizes
    AdjustUnitSizes();

    // calculate # of toc levels
    CalcTocLevels();

    // determine which level will be splitted to different files, and build file layout
    BuiltFileLayout(CalcLevelToSplit());
}

//-----------------------------------------------------------------------
void ConverterPass2::BuildReferenceMaps(std::set<String> *noteRefIds)
{
    ReferenceMap::iterator refidToNew_end = refidToNew_.end();
    RefidInfoMap::iterator refidToUnit_end = refidToUnit_.end();

    UnitArray::const_iterator cit = units_.begin(), cit_end = units_.end();
    for(; cit < cit_end; ++cit)
    {
        strvector::const_iterator cit1 = cit->refIds_.begin(), cit1_end = cit->refIds_.end();
        for(; cit1 < cit1_end; ++cit1)
        {
            const String &id = *cit1;

            // map original id to new id
            String newId;
            {
                ReferenceMap::iterator it = refidToNew_.lower_bound(id);
                if(it != refidToNew_end && it->first == id)
                    newId = it->second;
                else
                {
                    newId = MakeUniqueId();
                    refidToNew_.insert(it, ReferenceMap::value_type(id, newId));
                }
            }

            // map new unique id to unit
            {
                // The input unit array should not contatin duplicate ids.
                // Actually we are supposed to skip all dup refs in AddId method, pass 1.
                // So if we found one then it is internal error.
                RefidInfoMap::iterator it = refidToUnit_.lower_bound(newId);
#if defined(_DEBUG)
                if(it != refidToUnit_end && it->first == id)
                    Error("internal error: duplicate reference id");
#endif
                refidToUnit_.insert(it, RefidInfoMap::value_type(newId, &*cit));
            }
        }

        // store note id
        if(!cit->noteRefId_.empty())
            noteRefIds->insert(cit->noteRefId_);
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::BuildAnchors(const std::set<String> &noteRefIds)
{
    UnitArray::const_iterator cit = units_.begin(), cit_end = units_.end();
    for(; cit < cit_end; ++cit)
    {
        std::set<String>::const_iterator cit1 = cit->refs_.begin(), cit1_end = cit->refs_.end();
        for(; cit1 != cit1_end; ++cit1)
            if(noteRefIds.find(*cit1) != noteRefIds.end())
            {
                // this is id to note/comment section
                String id = refidToNew_[*cit1];

                // make sure that this id appears first time
                ReferenceMap::iterator it = noteidToAnchorId_.lower_bound(id);
                if(it != noteidToAnchorId_.end() && it->first == id)
                    continue;   // already has anchor

                // create new unique anchor id
                String anchorid = MakeUniqueId(true);
                refidToUnit_[anchorid] = &*cit;
                noteidToAnchorId_.insert(it, ReferenceMap::value_type(id, anchorid));
            }
    }
}

//-----------------------------------------------------------------------
String ConverterPass2::Findhref(const AttrMap &attrmap) const
{
    std::set<String>::const_iterator cit = xlns_.begin(), cit_end = xlns_.end();
    for(; cit != cit_end; ++cit)
    {
        AttrMap::const_iterator ait = attrmap.find(cit->empty() ? String("href") : (*cit)+":href");
        if(ait != attrmap.end())
            return ait->second;
    }
    return "";
}

//-----------------------------------------------------------------------
void ConverterPass2::StartUnit(Unit::Type unitType, AttrMap *attrmap)
{
    // close previous section
    if(unitActive_)
    {
        if(unitHasId_)
            pout_->WriteFmt("</div>\n");        // <div id=...> - original id
#if !FB2TOEPUB_TOC_REFERS_FILES_ONLY
        pout_->WriteFmt("</div>\n");            // <div id=...> - file id
#endif
        if(units_[unitIdx_].type_ == Unit::SECTION)
            pout_->WriteFmt("</div>\n");    // <div class="section...>
        ++unitIdx_;
    }

    const Unit &unit = units_[unitIdx_];
    if(prevUnitFile_ != unit.file_)
    {
        prevUnitFile_ = unit.file_;

        // close previous file
        if(unitActive_)
        {
            if(units_[unitIdx_-1].bodyType_ != Unit::BODY_NONE)
                pout_->WriteFmt("</div>\n");    // <div class="body...>
            pout_->WriteFmt("</body>\n");
            pout_->WriteFmt("</html>\n");
        }

        // begin new file
        pout_->BeginFile((String("OPS/") + unit.file_ + ".xhtml").c_str(), true);
        pout_->WriteFmt("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        pout_->WriteFmt("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
        pout_->WriteFmt("<head>\n");
        pout_->WriteFmt("<title/>\n");

        strvector::const_iterator cit = cssfiles_.begin(), cit_end = cssfiles_.end();
        for(; cit < cit_end; ++cit)
            pout_->WriteFmt("<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\"/>\n", EncodeStr(*cit).c_str());

        pout_->WriteFmt("</head>\n");
        pout_->WriteFmt("<body>\n");

        switch(unit.bodyType_)
        {
        case Unit::MAIN:        pout_->WriteStr("<div class=\"body_main\">"); break;
        case Unit::NOTES:       pout_->WriteStr("<div class=\"body_notes\">"); break;
        case Unit::COMMENTS:    pout_->WriteStr("<div class=\"body_comments\">"); break;
        case Unit::BODY_NONE:   break;
        default:                Error("StartUnit internal error");
        }
    }
    if(unit.type_ == Unit::SECTION)
        pout_->WriteFmt("<div class=\"section%d\">\n", unit.level_+1);
#if !FB2TOEPUB_TOC_REFERS_FILES_ONLY
    pout_->WriteFmt("<div id=\"%s\">\n", unit.fileId_.c_str()); // file id
#endif

    unitHasId_ = false;
    if(attrmap)
    {
        String id = (*attrmap)["id"];
        if(!id.empty())
        {
            unitHasId_ = true;
            pout_->WriteFmt("<div id=\"%s\">\n", refidToNew_[id].c_str()); // original id (remapped)
        }
    }
    unitActive_ = true;
}

//-----------------------------------------------------------------------
void ConverterPass2::EndUnit()
{
    if(unitActive_)
    {
        // close last section and file
        if(unitHasId_)
            pout_->WriteFmt("</div>\n");    // <div id=...> - original id
#if !FB2TOEPUB_TOC_REFERS_FILES_ONLY
        pout_->WriteFmt("</div>\n");        // <div id=...> - file id
#endif
        if(units_[unitIdx_].type_ == Unit::SECTION)
            pout_->WriteFmt("</div>\n");    // <div class="section...">
        if(units_[unitIdx_].bodyType_ != Unit::BODY_NONE)
            pout_->WriteFmt("</div>\n");    // <div class="body...">
        pout_->WriteFmt("</body>\n");
        pout_->WriteFmt("</html>\n");

        unitActive_ = false;
        ++unitIdx_;
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::AddMimetype()
{
    static const char contents[] = "application/epub+zip";
    pout_->BeginFile("mimetype", false);
    pout_->Write(contents, sizeof(contents)/sizeof(char)-1);
}

//-----------------------------------------------------------------------
void ConverterPass2::AddContainer()
{
    static const char contents[] =  "<?xml version=\"1.0\"?>\n"
                                    "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
                                    "  <rootfiles>\n"
                                    "    <rootfile full-path=\"OPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
                                    "  </rootfiles>\n"
                                    "</container>";

    pout_->BeginFile("META-INF/container.xml", true);
    pout_->Write(contents, sizeof(contents)/sizeof(char)-1);
}

//-----------------------------------------------------------------------
void ConverterPass2::AddStyles()
{
    strvector::const_iterator cit = css_.begin(), cit_end = css_.end();
    for(; cit < cit_end; ++cit)
    {
        Ptr<ScanDir> sd = CreateScanDir(cit->c_str(), "css");
        String fname;
        for(String ospath = sd->GetNextFile(&fname); !ospath.empty(); ospath = sd->GetNextFile(&fname))
        {
            fname = String("css/") + fname;
            pout_->AddFile(CreateInFileStm(ospath.c_str()), (String("OPS/") + fname).c_str(), true);
            cssfiles_.push_back(fname);
        }
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::AddFonts(const char *ext, strvector *fontfiles)
{
    strvector::const_iterator cit = fonts_.begin(), cit_end = fonts_.end();
    for(; cit < cit_end; ++cit)
    {
        Ptr<ScanDir> sd = CreateScanDir(cit->c_str(), ext);
        String fname;
        for(String ospath = sd->GetNextFile(&fname); !ospath.empty(); ospath = sd->GetNextFile(&fname))
        {
            fname = String("fonts/") + fname;
            pout_->AddFile(CreateInFileStm(ospath.c_str()), (String("OPS/") + fname).c_str(), true);
            fontfiles->push_back(fname);
        }
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::MakeCoverPageFirst()
{
    if(coverPgIt_ != units_.end())
    {
        // move cover to begin
        Unit coverUnit = *coverPgIt_;
        units_.erase(coverPgIt_);
        units_.insert(units_.begin(), coverUnit);
        coverPgIt_ = units_.begin();
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::AddContentOpf()
{
    strvector files;
    {
        // build file array
        String prevFile;
        UnitArray::const_iterator cit = units_.begin(), cit_end = units_.end();
        for(; cit < cit_end; ++cit)
            if(prevFile != cit->file_)
            {
                prevFile = cit->file_;
                files.push_back(cit->file_);
            }
    }

    pout_->BeginFile("OPS/content.opf", true);

    pout_->WriteStr("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    pout_->WriteStr("<package xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"dcidid\" version=\"2.0\">\n\n");

    pout_->WriteStr("  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n");
    pout_->WriteStr("    xmlns:dcterms=\"http://purl.org/dc/terms/\"\n");
    pout_->WriteStr("    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    pout_->WriteStr("    xmlns:opf=\"http://www.idpf.org/2007/opf\">\n");
    pout_->WriteFmt("    <dc:title>%s</dc:title>\n", (xlitConv_ ? xlitConv_->Convert(title_) : title_).c_str());
    pout_->WriteFmt("    <dc:language>%s</dc:language>\n", lang_.c_str());
    pout_->WriteFmt("    <dc:identifier id=\"dcidid\" opf:scheme=\"ID\">%s</dc:identifier>\n", id_.c_str());
    pout_->WriteFmt("    <dc:creator opf:role=\"aut\">%s</dc:creator>\n", (xlitConv_ ? xlitConv_->Convert(authors_) : authors_).c_str());
    pout_->WriteStr("  </metadata>\n\n");

    pout_->WriteStr("  <manifest>\n");
    AddContentManifestFile(pout_, "ncx", "toc.ncx", "application/x-dtbncx+xml");
    {
        int i = 0;
        binvector::const_iterator cit = binaries_.begin(), cit_end = binaries_.end();
        for(; cit < cit_end; ++cit)
            AddContentManifestFile(pout_, MakeFileName("bin", i++).c_str(), cit->file_.c_str(), cit->type_.c_str());
    }
    {
        int i;
        strvector::const_iterator cit, cit_end;

        for(cit = cssfiles_.begin(), cit_end = cssfiles_.end(), i = 0; cit < cit_end; ++cit)
            AddContentManifestFile(pout_, MakeFileName("css", i++).c_str(), cit->c_str(), "text/css");

        for(cit = ttffiles_.begin(), cit_end = ttffiles_.end(), i = 0; cit < cit_end; ++cit)
            AddContentManifestFile(pout_, MakeFileName("ttf", i++).c_str(), cit->c_str(), "application/x-font-ttf");

        for(cit = otffiles_.begin(), cit_end = otffiles_.end(), i = 0; cit < cit_end; ++cit)
            AddContentManifestFile(pout_, MakeFileName("otf", i++).c_str(), cit->c_str(), "application/x-font-otf");

        for(cit = files.begin(), cit_end = files.end(); cit < cit_end; ++cit)
            AddContentManifestFile(pout_, cit->c_str(), (*cit + ".xhtml").c_str(), "application/xhtml+xml");
    }
    pout_->WriteStr("  </manifest>\n\n");

    pout_->WriteStr("  <spine toc=\"ncx\">\n");
    {
        strvector::const_iterator cit = files.begin(), cit_end = files.end();
        for(; cit < cit_end; ++cit)
            pout_->WriteFmt("    <itemref idref=\"%s\"/>\n", cit->c_str());
    }
    pout_->WriteStr("  </spine>\n\n");
    pout_->WriteStr("</package>\n");
}

//-----------------------------------------------------------------------
void ConverterPass2::AddTocNcx()
{
    pout_->BeginFile("OPS/toc.ncx", true);
    
    pout_->WriteStr("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    pout_->WriteStr("<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n");

    pout_->WriteStr("<head>\n");
    pout_->WriteFmt("  <meta name=\"dtb:uid\" content=\"%s\"/>\n", id_.c_str());
    pout_->WriteFmt("  <meta name=\"dtb:depth\" content=\"%d\"/>\n", tocLevels_);
    pout_->WriteStr("  <meta name=\"dtb:totalPageCount\" content=\"0\"/>\n");
    pout_->WriteStr("  <meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n");
    pout_->WriteStr("</head>\n");
    pout_->WriteStr("<docTitle>\n");
    pout_->WriteFmt("  <text>%s</text>\n", (xlitConv_ ? xlitConv_->Convert(title_) : title_).c_str());
    pout_->WriteStr("</docTitle>\n");
    pout_->WriteStr("<navMap>\n");

    int navPoint = 1;
    int level = 0;
    bool first = true;
    {
        UnitArray::const_iterator cit = units_.begin(), cit_end = units_.end();
        for(; cit < cit_end; ++cit)
        {
            if(cit->title_.empty())
                continue;

            if (level > cit->level_)
            {
                // close previous
                for(int i = level - cit->level_; --i >= 0;)
                    pout_->WriteFmt("</navPoint>\n");
                pout_->WriteFmt("</navPoint>\n");
            }
            else if(level == cit->level_)
            {
                // close previous
                if(!first)
                    pout_->WriteFmt("</navPoint>\n");
                first = false;
            }
            pout_->WriteFmt("<navPoint id=\"navPoint-%d\" playOrder=\"%d\">\n", navPoint, navPoint);
            pout_->WriteFmt("<navLabel><text>%s</text></navLabel>", (xlitConv_ ? xlitConv_->Convert(cit->title_) : cit->title_).c_str());

#if FB2TOEPUB_TOC_REFERS_FILES_ONLY
            String fullId = cit->file_ + ".xhtml";
#else
            String fullId = cit->file_ + ".xhtml#" + cit->fileId_;
#endif
            pout_->WriteFmt("<content src=\"%s\"/>\n", fullId.c_str());

            level = cit->level_;
            ++navPoint;
        }
        while(--level >= 0)
            pout_->WriteFmt("</navPoint>\n");
        if(!first)
            pout_->WriteFmt("</navPoint>\n");
    }

    pout_->WriteStr("  </navMap>\n");
    pout_->WriteStr("</ncx>\n");
}

//-----------------------------------------------------------------------
const String* ConverterPass2::AddId(const AttrMap &attrmap)
{
    AttrMap::const_iterator cit = attrmap.find("id");
    if(cit == attrmap.end())
        return NULL;

    if(allRefIds_.find(cit->second) != allRefIds_.end())
        return NULL;    // ignore second instance

    String id = cit->second;

    // remap it to our new id
    id = refidToNew_[id];
    if(id.empty())
        Error("AddId internal error");

    pout_->WriteFmt(" id=\"%s\"", EncodeStr(id).c_str());
    return &cit->second;
}

//-----------------------------------------------------------------------
void ConverterPass2::ParseTextAndEndElement (const String &element)
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
            pout_->WriteStr(s_->GetToken().s_.c_str());
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
                image(true, true, false);
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
void ConverterPass2::CopyAttribute(const String &attr, const AttrMap &attrmap)
{
    AttrMap::const_iterator cit = attrmap.find(attr);
    if(cit != attrmap.end())
        pout_->WriteFmt(" %s=\"%s\"", attr.c_str(), EncodeStr(cit->second).c_str());
}

//-----------------------------------------------------------------------
void ConverterPass2::FictionBook()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("FictionBook", &attrmap);

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

    s_->SkipRestOfElementContent(); // skip rest of <FictionBook>
}

//-----------------------------------------------------------------------
bool ConverterPass2::AddAnchorid(const String &anchorid)
{
    std::set<String>::iterator it = usedAnchorsids_.lower_bound(anchorid);
    if(it != usedAnchorsids_.end() && *it == anchorid)
        return false;   // already processed

    usedAnchorsids_.insert(it, anchorid);
    return true;
}

//-----------------------------------------------------------------------
void ConverterPass2::a()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("a", &attrmap);

    String id = Findhref(attrmap);
    if(id.empty())
        Error("<a> should have href attribute");

    bool anchorSet = false;
    if(id[0] != '#')
    {
        // external reference
        pout_->WriteFmt("<a class=\"e_a\" href=\"%s\"", EncodeStr(id).c_str());
        if(!notempty)
        {
            pout_->WriteStr("/>");
            return;
        }
    }
    else
    {
        // internal reference
        id = id.substr(1);
        String file = refidToUnit_[refidToNew_[id]]->file_;

        // remap it to our new id
        id = refidToNew_[id];
        if(id.empty())
            Error("a() internal error");

        String anchorid = noteidToAnchorId_[id];
        if(!anchorid.empty() && AddAnchorid(anchorid))
        {
            anchorSet = true;
            pout_->WriteFmt("<span id=\"%s\">", anchorid.c_str());
        }

        pout_->WriteFmt("<a href=\"%s.xhtml#%s\"", file.c_str(), id.c_str());
        if(!notempty)
        {
            pout_->WriteStr("/>");
            if(anchorSet)
                pout_->WriteFmt("</span>");
            return;
        }
    }
    pout_->WriteStr(">");

    SetScannerDataMode setDataMode(s_);
    for(;;)
    {
        LexScanner::Token t = s_->LookAhead();
        switch(t.type_)
        {
        default:
            s_->EndElement();
            pout_->WriteStr("</a>");
            if(anchorSet)
                pout_->WriteFmt("</span>");
            return;

        case LexScanner::DATA:
            pout_->WriteStr(s_->GetToken().s_.c_str());
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
                Error(ss.str().c_str());
            }
            continue;
            //</strong>, </emphasis>, </stile>, </strikethrough>, </sub>, </sup>, </code>, </image>
        }
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::annotation(bool startUnit)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("annotation", &attrmap);
    if(startUnit)
        StartUnit(Unit::ANNOTATION);

    pout_->WriteStr("<div class=\"annotation\"");
    AddId(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

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

    pout_->WriteStr("</div>\n");
    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass2::author()
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

    authors_ = Concat(authors_, ", ", author);
    s_->SkipRestOfElementContent();
}

//-----------------------------------------------------------------------
void ConverterPass2::binary()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("binary", &attrmap);

    // store binary attributes
    Binary b(attrmap["id"], attrmap["content-type"]);
    //if(b.file_.empty() || (b.type_ != "image/jpeg" && b.type_ != "image/png"))
    if(b.file_.empty() || b.type_.empty())
        Error("invalid <binary> attributes");
    b.file_ = String("bin/") + b.file_;
    binaries_.push_back(b);

    // store binary file
    {
        SetScannerDataMode setDataMode(s_);
        LexScanner::Token t = s_->GetToken();
        if(t.type_ != LexScanner::DATA)
            Error("<binary> data expected");

        pout_->BeginFile((String("OPS/") + b.file_).c_str(), true);
        DecodeBase64(t.s_.c_str(), pout_);
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass2::body()
{
    s_->BeginNotEmptyElement("body");

    //<image>
    if(s_->IsNextElement("image"))
        image(false, false, true, Unit::IMAGE);
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

    EndUnit();

    s_->SkipRestOfElementContent(); // skip rest of <body>
}

//-----------------------------------------------------------------------
void ConverterPass2::book_title()
{
    title_ = s_->SimpleTextElement("book-title");
}

//-----------------------------------------------------------------------
void ConverterPass2::cite()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("cite", &attrmap);
    pout_->WriteStr("<div class=\"citation\"");
    AddId(attrmap);
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
            Error(ss.str().c_str());
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
void ConverterPass2::code()
{
    if(s_->BeginElement("code"))
    {
        pout_->WriteStr("<code class=\"e_code\">");
        ParseTextAndEndElement("code");
        pout_->WriteStr("</code>");
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::coverpage()
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
void ConverterPass2::date()
{
    if(s_->BeginElement("date"))
    {
        SetScannerDataMode setDataMode(s_);
        if(s_->LookAhead().type_ == LexScanner::DATA)
            pout_->WriteFmt("<p class=\"date\">%s</p>\n", s_->GetToken().s_.c_str());
        s_->EndElement();
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::description()
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
    document_info();
    //</document-info>

    s_->SkipRestOfElementContent(); // skip rest of <description>
}

//-----------------------------------------------------------------------
void ConverterPass2::document_info()
{
    s_->BeginNotEmptyElement("document-info");

    //<author>
    s_->CheckAndSkipElement("author");
    s_->SkipAll("author");
    //</author>

    //<program-used>
    if(s_->IsNextElement("program-used"))
        s_->SkipElement();
    //</program-used>

    //<date>
    s_->CheckAndSkipElement("date");
    //</date>

    //<src-url>
    s_->SkipAll("src-url");
    //</src-url>

    //<src-ocr>
    if(s_->IsNextElement("src-ocr"))
        s_->SkipElement();
    //</src-ocr>

    //<id>
    id();
    //<id>

    s_->SkipRestOfElementContent(); // skip rest of <document-info>
}

//-----------------------------------------------------------------------
void ConverterPass2::emphasis()
{
    if(s_->BeginElement("emphasis"))
    {
        pout_->WriteStr("<em class=\"emphasis\">");
        ParseTextAndEndElement("emphasis");
        pout_->WriteStr("</em>");
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::empty_line()
{
    bool notempty = s_->BeginElement("empty-line");
    pout_->WriteStr("<p class=\"empty-line\"> </p>\n");
    if(notempty)
        s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass2::epigraph()
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
            Error(ss.str().c_str());
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
void ConverterPass2::id()
{
    id_ = s_->SimpleTextElement("id");;
}

//-----------------------------------------------------------------------
void ConverterPass2::image(bool fb2_inline, bool html_inline, bool scale, Unit::Type unitType)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("image", &attrmap);

    if(unitType != Unit::UNIT_NONE)
        StartUnit(unitType);
    bool has_id = !fb2_inline && attrmap.find("id") != attrmap.end();
    if(has_id)
    {
        pout_->WriteStr("<div");
        AddId(attrmap);
        pout_->WriteStr(">");
    }

    // get file href
    String href = Findhref(attrmap), alt = attrmap["alt"];
    if(!href.empty())
    {
        String group = html_inline ? "span" : "div";

        pout_->WriteFmt("<%s class=\"image\">", group.c_str());
        if(scale)
            pout_->WriteFmt("<img style=\"height: 100%%;\" alt=\"%s\" src=\"bin/%s\"/>", EncodeStr(alt).c_str(), EncodeStr(href).c_str()+1);
        else
            pout_->WriteFmt("<img alt=\"%s\" src=\"bin/%s\"/>", EncodeStr(alt).c_str(), EncodeStr(href).c_str()+1);

        if(!fb2_inline)
        {
            if(html_inline)
                Error("<image> internal error");
            AttrMap::const_iterator cit = attrmap.find("title");
            if(cit != attrmap.end())
                pout_->WriteFmt("<p>%s</p>\n", EncodeStr(cit->second).c_str());
        }
        pout_->WriteFmt("</%s>", group.c_str());
    }
    if(has_id)
        pout_->WriteStr("</div>\n");
    if(!notempty)
        return;
    ClrScannerDataMode clrDataMode(s_);
    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass2::lang()
{
    lang_ = s_->SimpleTextElement("lang");
}

//-----------------------------------------------------------------------
void ConverterPass2::p(const char *pelement, const char *cls)
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("p", &attrmap);

    pout_->WriteFmt("<%s", pelement);
    if(cls)
        pout_->WriteFmt(" class=\"%s\"", cls);
    AddId(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

    ParseTextAndEndElement("p");
    pout_->WriteFmt("</%s>\n", pelement);
}

//-----------------------------------------------------------------------
void ConverterPass2::poem()
{
    AttrMap attrmap;
    s_->BeginNotEmptyElement("poem", &attrmap);
    pout_->WriteStr("<div class=\"poem\"");
    AddId(attrmap);
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
void ConverterPass2::section()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("section", &attrmap);

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
                image(false, false, false);
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
    }

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass2::stanza()
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
void ConverterPass2::strikethrough()
{
    if(s_->BeginElement("strikethrough"))
    {
        pout_->WriteStr("<del class=\"strikethrough\">");
        ParseTextAndEndElement("strikethrough");
        pout_->WriteStr("</del>");
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::strong()
{
    if(s_->BeginElement("strong"))
    {
        pout_->WriteStr("<strong class=\"e_strong\">");
        ParseTextAndEndElement("strong");
        pout_->WriteStr("</strong>");
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::style()
{
    s_->SkipElement();
    //if(s_->BeginElement("style"))
    //    ParseTextAndEndElement("style");
}

//-----------------------------------------------------------------------
void ConverterPass2::sub()
{
    if(s_->BeginElement("sub"))
    {
        pout_->WriteStr("<sub class=\"e_sub\">");
        ParseTextAndEndElement("sub");
        pout_->WriteStr("</sub>");
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::subtitle()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("subtitle", &attrmap);

    pout_->WriteStr("<h2 class=\"e_h2\"");
    AddId(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

    ParseTextAndEndElement("subtitle");
    pout_->WriteStr("</h2>\n");
}

//-----------------------------------------------------------------------
void ConverterPass2::sup()
{
    if(s_->BeginElement("sup"))
    {
        pout_->WriteStr("<sup class=\"e_sup\">");
        ParseTextAndEndElement("sup");
        pout_->WriteStr("</sup>");
    }
}

//-----------------------------------------------------------------------
void ConverterPass2::table()
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
void ConverterPass2::td()
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
    CopyAttribute("xml::lang", attrmap);

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
void ConverterPass2::text_author()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("text-author", &attrmap);

    pout_->WriteFmt("<div class=\"text_author\"");
    AddId(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

    ParseTextAndEndElement("text-author");
    pout_->WriteStr("</div>\n");
}

//-----------------------------------------------------------------------
void ConverterPass2::th()
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
    CopyAttribute("xml::lang", attrmap);

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
void ConverterPass2::title(bool startUnit, const String &anchorid)
{
    if(!s_->BeginElement("title"))
        return;

    if(startUnit)
        StartUnit(Unit::TITLE);

    pout_->WriteFmt("<div class=\"title\">\n");
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
            Error(ss.str().c_str());
        }
    }
    if(!anchorid.empty())
        pout_->WriteFmt("<h1><span class=\"anchor\"><a href=\"%s\">[&lt;-]</a></span></h1>", anchorid.c_str());
    pout_->WriteStr("</div>\n");

    s_->EndElement();
}

//-----------------------------------------------------------------------
void ConverterPass2::title_info()
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

    //<lang>
    lang();
    //</lang>

    s_->SkipRestOfElementContent(); // skip rest of <title-info>
}

//-----------------------------------------------------------------------
void ConverterPass2::tr()
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
void ConverterPass2::v()
{
    AttrMap attrmap;
    bool notempty = s_->BeginElement("v", &attrmap);

    pout_->WriteStr("<p class=\"v\"");
    AddId(attrmap);
    if(!notempty)
    {
        pout_->WriteStr("/>");
        return;
    }
    pout_->WriteStr(">");

    ParseTextAndEndElement("v");
    pout_->WriteStr("</p>\n");
}


void FB2TOEPUB_DECL DoConvertionPass2  (LexScanner *scanner,
                                        const strvector &css,
                                        const strvector &fonts,
                                        XlitConv *xlitConv,
                                        UnitArray *units,
                                        OutPackStm *pout)
{
    Ptr<ConverterPass2> conv = new ConverterPass2(scanner, css, fonts, xlitConv, units, pout);
    conv->Scan();
}


};  //namespace Fb2ToEpub
