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
#include "streamzip.h"
#include "minizip/unzip.h"
#include "minizip/zip.h"

#include <string>
#include <time.h>

namespace Fb2ToEpub
{

//-----------------------------------------------------------------------
void OutPackStm::AddFile(InStm *pin, const char *name, bool compress)
{
    BeginFile(name, compress);
    while(!pin->IsEOF())
    {
        char buf[512];
        Write(buf, pin->Read(buf, sizeof(buf)));
    }
}

//-----------------------------------------------------------------------
// UnzipStm implementation
//-----------------------------------------------------------------------
class UnzipStm : public InStm, Noncopyable
{
    ::unzFile   uf_;
    mutable int c_; // last buffered character
    String name_;

public:
    explicit UnzipStm(const char *name);
    ~UnzipStm();

    //virtuals
    bool        IsEOF() const;
    char        GetChar();
    size_t      Read(void *buffer, size_t max_cnt);
    void        UngetChar(char c);
    void        Rewind();
};

//-----------------------------------------------------------------------
UnzipStm::UnzipStm(const char *name) : uf_(::unzOpen(name)), c_(EOF), name_(name)
{
    if(!uf_)
        Error("unzOpen error");
    if(UNZ_OK != ::unzOpenCurrentFile(uf_))
        Error("unzOpenCurrentFile error");
}

//-----------------------------------------------------------------------
UnzipStm::~UnzipStm()
{
    ::unzCloseCurrentFile(uf_);
    ::unzClose(uf_);
}

//-----------------------------------------------------------------------
bool UnzipStm::IsEOF() const
{
    return (c_ == EOF) && (::unzeof(uf_) == 1);
}

//-----------------------------------------------------------------------
char UnzipStm::GetChar()
{
    if(c_ == EOF)
    {
        char c;
        if(::unzReadCurrentFile(uf_, &c, 1) != 1)
            Error("unzReadCurrentFile EOF or read error");
        return c;
    }
    else
    {
        char cret = static_cast<char>(c_);
        c_ = EOF;
        return cret;
    }
}

//-----------------------------------------------------------------------
size_t UnzipStm::Read(void *buffer, size_t max_cnt)
{
    if(!max_cnt)
        return 0;

    char *cb = reinterpret_cast<char*>(buffer);
    size_t cnt = 0;

    if(c_ != EOF)
    {
        *cb++ = static_cast<char>(c_);
        if(max_cnt == 1)
            return 1;
        cnt = 1;
        c_ = EOF;
    }

    int cnt_read = ::unzReadCurrentFile(uf_, cb, max_cnt - cnt);
    if(cnt_read < 0)
        Error("unzReadCurrentFile read error");

    return cnt + static_cast<size_t>(cnt_read);
}

//-----------------------------------------------------------------------
void UnzipStm::UngetChar(char c)
{
    if(c_ != EOF)
        Error("zip: unget char error");
    c_ = static_cast<unsigned char>(c);
}

//-----------------------------------------------------------------------
void UnzipStm::Rewind()
{
    // reopen
    c_ = EOF;
    ::unzCloseCurrentFile(uf_);
    ::unzClose(uf_);
    uf_ = ::unzOpen(name_.c_str());
    if(!uf_)
        Error("unzOpen error");
    if(UNZ_OK != ::unzOpenCurrentFile(uf_))
        Error("unzOpenCurrentFile error");
}

//-----------------------------------------------------------------------
Ptr<InStm> CreateUnpackStm(const char *name)
{
    // check if zip
    Ptr<InStm> stm = CreateInFileStm(name);
    if (stm->GetChar() == 0x50 &&
        stm->GetChar() == 0x4B &&
        stm->GetChar() == 0x03 &&
        stm->GetChar() == 0x04)
    {
        return new UnzipStm(name);
    }
    stm->Rewind();    
    return stm;
}

//-----------------------------------------------------------------------
// ZipStm implementation
//-----------------------------------------------------------------------
class ZipStm : public OutPackStm, Noncopyable
{
    ::zipFile   zf_;
    bool        file_open;
    
public:
    explicit ZipStm(const char *name);
    ~ZipStm();

    //virtuals
    void    PutChar(char c);
    void    Write (const void *p, size_t cnt);
    void    BeginFile(const char *name, bool compress);
};

//-----------------------------------------------------------------------
ZipStm::ZipStm(const char *name) : zf_(::zipOpen(name, APPEND_STATUS_CREATE)), file_open(false)
{
    if(!zf_)
        Error("zipOpen error");
}

//-----------------------------------------------------------------------
ZipStm::~ZipStm()
{
    if(file_open)
        ::zipCloseFileInZip(zf_);
    ::zipClose(zf_, NULL);
}

//-----------------------------------------------------------------------
void ZipStm::PutChar(char c)
{
    if(!file_open)
        Error("zip: file not added to zip");
    if(::zipWriteInFileInZip(zf_, &c, 1) < 0)
        Error("zipWriteInFileInZip error");
}

//-----------------------------------------------------------------------
void ZipStm::Write (const void *p, size_t cnt)
{
    if(!file_open)
        Error("zip: file not added to zip");
    if(::zipWriteInFileInZip(zf_, p, cnt) < 0)
        Error("zipWriteInFileInZip error");
}

//-----------------------------------------------------------------------
void ZipStm::BeginFile(const char *name, bool compress)
{
    if(!file_open)
        file_open = true;
    else if(ZIP_OK != ::zipCloseFileInZip(zf_))
        Error("zipCloseFileInZip error");

    time_t ltime;
    time(&ltime);
    tm *filedate = localtime(&ltime);

    ::zip_fileinfo zi;

    zi.tmz_date.tm_sec  = filedate->tm_sec;
    zi.tmz_date.tm_min  = filedate->tm_min;
    zi.tmz_date.tm_hour = filedate->tm_hour;
    zi.tmz_date.tm_mday = filedate->tm_mday;
    zi.tmz_date.tm_mon  = filedate->tm_mon ;
    zi.tmz_date.tm_year = filedate->tm_year;
    zi.dosDate          = 0;
    zi.internal_fa      = 0;
    zi.external_fa      = 0;
    
    if(ZIP_OK != ::zipOpenNewFileInZip (zf_, name, &zi, NULL, 0, NULL, 0, NULL,
                                        compress ? Z_DEFLATED : 0,
                                        compress ? Z_BEST_COMPRESSION : Z_NO_COMPRESSION))
    {
        Error("zipOpenNewFileInZip error");
    }
}

//-----------------------------------------------------------------------
Ptr<OutPackStm> FB2TOEPUB_DECL CreatePackStm(const char *name)
{
    return new ZipStm(name);
}

};  //namespace Fb2ToEpub
