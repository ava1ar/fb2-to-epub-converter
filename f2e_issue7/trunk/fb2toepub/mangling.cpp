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

#include "mangling.h"

namespace Fb2ToEpub
{


//-----------------------------------------------------------------------
// InManglingStm
//-----------------------------------------------------------------------
class InManglingStm : public InStm, Noncopyable
{
    Ptr<InStm>          stm_;
    const unsigned char *key_;
    size_t              keySize_, maxSize_, keyPos_, pos_;

public:
    InManglingStm(InStm *stm, const unsigned char *key, size_t keySize, size_t maxSize)
        : stm_(stm), key_(key), keySize_(keySize), maxSize_(maxSize), keyPos_(0), pos_(0) {}

    //virtuals
    bool        IsEOF() const                       {return stm_->IsEOF();}
    char        GetChar();
    size_t      Read(void *buffer, size_t max_cnt);
    void        UngetChar(char c)                   {Error("InManglingStm unget not implemented");}
    void        Rewind()                            {stm_->Rewind(); keyPos_ = pos_ = 0;}
};

//-----------------------------------------------------------------------
char InManglingStm::GetChar()
{
    if(pos_ >= maxSize_)
        return stm_->GetChar();
    else
    {
        char c = stm_->GetChar() ^ key_[keyPos_++];
        if(keyPos_ >= keySize_)
            keyPos_= 0;
        ++pos_;
        return c;
    }
}

//-----------------------------------------------------------------------
size_t InManglingStm::Read(void *buffer, size_t max_cnt)
{
    size_t cnt = stm_->Read(buffer, max_cnt);

    char *cb = reinterpret_cast<char*>(buffer);
    for(;;)
    {
        if(pos_ >= maxSize_)
            return cnt;

        size_t to_mangle = keySize_ - keyPos_;
        if(to_mangle > maxSize_ - pos_)
            to_mangle = maxSize_ - pos_;
        if(to_mangle > cnt)
            to_mangle = cnt;

        for(size_t u = to_mangle; u-- > 0;)
        {
            *cb = *cb ^ key_[keyPos_++];
            ++cb;
        }

        pos_ += to_mangle;
        if(keyPos_ >= keySize_)
            keyPos_ = 0;
    }
}


//-----------------------------------------------------------------------
Ptr<InStm> CreateManglingStm(InStm *stm, const unsigned char *key, size_t keySize, size_t maxSize)
{
    return new InManglingStm(stm, key, keySize, maxSize);
}


};  //namespace Fb2ToEpub
