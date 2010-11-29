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
#include "streamconv.h"
#include "scanner.h"
#include "fb2toepubconv.h"

#include "base64.h"

#include <string>
#include <stdio.h>
#include <errno.h>

#if (defined unix)
#include <unistd.h>
#endif

using namespace Fb2ToEpub;

//-----------------------------------------------------------------------
const char name[]       = "FB2 to EPUB format converter";
const char version[]    = "0.10";

//-----------------------------------------------------------------------
static void Logo()
{
    printf("%s version %s\n", name, version);
}

//-----------------------------------------------------------------------
static void Usage()
{
    printf("Usage:\n\n");
    printf("Print input fb2 file info to console and exit:\n");
    printf("    fb2toepub -i <input file>\n\n");
    printf("or\n\n");
    printf("Convert input fb2 file to output epub file:\n");
    printf("    fb2toepub <options> <input file> <output file>\n\n");
    printf("Options:\n");
    printf("    -s <path>               Path to .css style directory\n");
    printf("                              (optional, any number)\n");
    printf("    -f <path>               Path to .ttf/.otf font directory\n");
    printf("                              (optional, any number)\n");
    printf("    -sf <path>              Path to font and style directory\n");
    printf("                              (optional, any number)\n");
    printf("    -t <path>               Path to transliteration XML file\n");
    printf("                              (optional, no more than one)\n");
    printf("    -h, --help              Help and exit\n\n");
    printf("Options are case-sensitive.\nSpace between -s/-f/-sf/-t and path is mandatory.\n");
}

//-----------------------------------------------------------------------
static int ErrorExit(const char *err)
{
    fprintf(stderr, "Command line error: %s, use option -h or --help for help\n", err);
    return 1;
}

//-----------------------------------------------------------------------
static int Info(const String &in)
{
    // check
    if(in.empty())
        return ErrorExit("input file is not defined");

    try
    {
        return PrintInfo(in);
    }
    catch(const String &s)
    {
        fprintf(stderr, "%s\n[%d]%s\n", s.c_str(), errno, strerror(errno));
        return 1;
    }
    catch(...)
    {
        fprintf(stderr, "Unknown error\n[%d]%s\n", errno, strerror(errno));
        return 1;
    }
}

//-----------------------------------------------------------------------
static void DeleteFile(const String &name)
{
#if defined(WIN32)
            _unlink(name.c_str());
#else
            unlink(name.c_str());
#endif
}

//-----------------------------------------------------------------------
int main(int argc, char **argv)
{
    // parse command line
    if(argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        Logo();
        Usage();
        return 0;
    }

    strvector css, fonts;
    String xlit, in, out;
    bool infoOnly = false;

    int i = 1;
    while(i < argc)
        if(!strcmp(argv[i], "-s"))
        {
            bool isFile = (argv[i] == "-s1");
            if(++i >= argc)
                return ErrorExit("incomplete style path definition");
            css.push_back(argv[i++]);
        }
        else if(!strcmp(argv[i], "-f"))
        {
            bool isFile = (argv[i] == "-f1");
            if(++i >= argc)
                return ErrorExit("incomplete style path definition");
            fonts.push_back(argv[i++]);
        }
        else if(!strcmp(argv[i], "-sf") || !strcmp(argv[i], "-fs"))
        {
            if(++i >= argc)
                return ErrorExit("incomplete style/font path definition");
            const char *p = argv[i++];
            css.push_back(p);
            fonts.push_back(p);
        }
        else if(!strcmp(argv[i], "-t"))
        {
            if(++i >= argc)
                return ErrorExit("incomplete transliteration file definition");
            if(!xlit.empty())
                return ErrorExit("transliteration file redefinition");
            xlit = argv[i++];
        }
        else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            ++i;
        else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--info"))
        {
            infoOnly = true;
            ++i;
        }
        else if(argv[i][0] != '-' && in.empty())
            in = argv[i++];
        else if(argv[i][0] != '-' && out.empty())
            out = argv[i++];
        else
            return ErrorExit("unrecognized command line switch");

    if(infoOnly)
        return Info(in);

    // check
    if(in.empty() || out.empty())
        return ErrorExit("input or output file is not defined");

    bool fOutputFileCreated = false;
    try
    {
        // create input stream
        Ptr<InStm> pin = CreateInUnicodeStm(CreateUnpackStm(in.c_str()));

        // create output stream
        Ptr<OutPackStm> pout = CreatePackStm(out.c_str());
        fOutputFileCreated = true;

        // create translite converter
        Ptr<XlitConv> xlitConv;
        if(!xlit.empty())
            xlitConv = CreateXlitConverter(CreateInUnicodeStm(CreateUnpackStm(xlit.c_str())));

        return Convert(pin, css, fonts, xlitConv, pout);
    }
    catch(const String &s)
    {
        fprintf(stderr, "%s\n[%d]%s\n", s.c_str(), errno, strerror(errno));
        if(fOutputFileCreated)
            DeleteFile(out);
        return 1;
    }
    catch(...)
    {
        fprintf(stderr, "Unknown error\n[%d]%s\n", errno, strerror(errno));
        if(fOutputFileCreated)
            DeleteFile(out);
        return 1;
    }
}
