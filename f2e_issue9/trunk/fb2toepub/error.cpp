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

#include "error.h"
#include <sstream>

namespace Fb2ToEpub
{


//-----------------------------------------------------------------------
// Internal error exception implementation
class InternalExceptionImpl : public ExceptionImpl<InternalException>
{
public:
    InternalExceptionImpl(const std::string &file, int line, const std::string &what)
                            : file_(file), line_(line)
    {
        std::ostringstream txt;
        txt << "internal converter error, " << file << "(" << line << ") : " << what;
        Init(txt.str());
    }

    //virtuals
    const std::string&  File() const    {return file_;}
    int                 Line() const    {return line_;}

private:
    std::string     file_;
    int             line_;
};
void InternalException::Raise(const std::string &file, int line, const std::string &what)
{
    throw InternalExceptionImpl(file, line, what);
}


//-----------------------------------------------------------------------
// External error exception implementation
void ExternalException::Raise(const std::string &what) {throw ExceptionImpl<ExternalException>(what);}


//-----------------------------------------------------------------------
// IO error exception implementation
void IOException::Raise(const std::string &what) {throw ExceptionImpl<IOException>(what);}


//-----------------------------------------------------------------------
// Parser error exception
class ParserExceptionImpl : public ExceptionImpl<ParserException>
{
public:
    ParserExceptionImpl(const std::string &file, const Loc &loc, const std::string &what)
                        : file_(file), loc_(loc)
    {
        std::ostringstream txt;
        txt << file;
        txt << "(" << loc.fstLn_ << "," << loc.fstCol_;
        txt << "-" << loc.lstLn_ << "," << loc.lstCol_;
        txt << ") : parser error: " << what;
        Init(txt.str());
    }

    //virtuals
    const std::string&  File() const        {return file_;}
    const Loc&          Location() const    {return loc_;}

private:
    std::string     file_;
    Loc             loc_;
};
void ParserException::Raise(const std::string &file, const Loc &loc, const std::string &what)
{
    throw ParserExceptionImpl(file, loc, what);
}

};  //namespace Fb2ToEpub
