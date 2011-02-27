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


#ifndef FB2TOEPUB__ERROR_H
#define FB2TOEPUB__ERROR_H

#include <string>
#include <stdarg.h>
#include "types.h"

namespace Fb2ToEpub
{

    //-----------------------------------------------------------------------
    // Generic error exception
    struct Exception
    {
        virtual ~Exception() {}
        virtual const String&  What() const = 0;
    };


    //-----------------------------------------------------------------------
    // Internal error exception
    struct InternalException : Exception
    {
        virtual const String&   File() const = 0;
        virtual int             Line() const = 0;

        static void Raise(const String &file, int line, const String &what);
    };


    //-----------------------------------------------------------------------
    // Generic external error exception
    struct ExternalException : Exception
    {
        static void Raise(const String &what);
    };


    //-----------------------------------------------------------------------
    // IO error exception
    struct IOException : ExternalException
    {
        virtual const String& File() const = 0;

        static void Raise(const String &file, const String &what);
    };


    //-----------------------------------------------------------------------
    // Parser error exception
    struct ParserException : ExternalException
    {
        struct Loc
        {
            int fstLn_, lstLn_, fstCol_, lstCol_;
            Loc()                                               : fstLn_(1), lstLn_(1), fstCol_(1), lstCol_(1) {}
            Loc(int fstLn, int lstLn, int fstCol, int lstCol)   : fstLn_(fstLn), lstLn_(lstLn), fstCol_(fstCol), lstCol_(lstCol) {}
        };

        virtual const String&   File() const        = 0;
        virtual const Loc&      Location() const    = 0;

        static void Raise(const String &file, const Loc &loc, const String &what);
    };


    //-----------------------------------------------------------------------
    // font error exception
    struct FontException : ExternalException
    {
        virtual const String& File() const = 0;

        static void Raise(const String &file, const String &what);
    };



    //-----------------------------------------------------------------------
    // Useful helper: exception implementation template
    //-----------------------------------------------------------------------
    template <class T> class ExceptionImpl : public T
    {
    public:
        ExceptionImpl(const String &what)   : what_(what) {}

        //virtual
        const String& What() const          {return what_;}

    protected:
        ExceptionImpl()                     {}
        void Init(const String &what)       {what_ = what;}

    private:
        String what_;
    };


    //-----------------------------------------------------------------------
    inline void ExternalError(const String &what)
        {ExternalException::Raise(what);}
    inline void InternalError(const String &file, int line, const String &what)
        {InternalException::Raise(file, line, what);}
    inline void IOError(const String &file, const String &what)
        {IOException::Raise(file, what);}
    inline void ParserError(const String &file, const ParserException::Loc &loc, const String &what)
        {ParserException::Raise(file, loc, what);}
    inline void FontError(const String &file, const String &what)
        {FontException::Raise(file, what);}



};  //namespace Fb2ToEpub

#endif
