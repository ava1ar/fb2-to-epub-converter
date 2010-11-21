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

using namespace Fb2ToEpub;

/*
static int test1(int argc, char **argv)
{
    if(argc != 3)
    {
        printf("Usage: fb2toepub <input file> <output file>");
        return 1;
    }

    //Ptr<InStm> pin      = CreateInFileStm(argv[1]);
    Ptr<InStm> pin      = CreateUnpackStm(argv[1]);
    Ptr<OutStm> pout    = CreateOutFileStm(argv[2]);

    while(!pin->IsEOF())
    {
        char buf[1024];
        pout->Write(buf, pin->Read(buf, sizeof(buf)));
    }

    //while(!pin->IsEOF())
    //    pout->PutChar(pin->GetChar());

    printf("fb2toepub done!!!");
    return 0;
}

static int test2(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: fb2toepub <input file> <output file>");
        return 1;
    }

    Ptr<OutPackStm> pout = CreatePackStm(argv[argc - 1]);
    for(int i = 1; i < argc - 1; ++i)
    {
        char sz[100];
        sprintf(sz, "dir%d/file%d", i, i);
        pout->BeginFile(sz, i != 1);
        Ptr<InStm> pin = CreateUnpackStm(argv[i]);
        while(!pin->IsEOF())
        {
            char buf[1024];
            pout->Write(buf, pin->Read(buf, sizeof(buf)));
        }
    }

    printf("fb2toepub done!!!");
    return 0;
}

static int test3(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: fb2toepub <input file> <output file>");
        return 1;
    }

    Ptr<InStm> pin      = CreateUnpackStm(argv[1]);
    pin                 = CreateInConvStm(pin, "Windows-1251", "UTF-16LE");
    //pin                 = CreateInConvStm(pin, "UTF-16", "Windows-1251");
    //pin                 = CreateInConvStm(pin, "UTF-16", "UTF-8");
    //pin                 = CreateInUnicodeStm(pin);
    Ptr<OutStm> pout    = CreateOutFileStm(argv[2]);

    while(!pin->IsEOF())
    {
        char buf[1024];
        pout->Write(buf, pin->Read(buf, sizeof(buf)));
    }

    //while(!pin->IsEOF())
    //    pout->PutChar(pin->GetChar());

    printf("fb2toepub done!!!");
    return 0;
}

class MyDriver : public Driver, Noncopyable
{
    Ptr<LexScanner> scanner_;
public:
    explicit MyDriver(InStm *stm) : scanner_(CreateScanner(stm))
    {
    }

    //virtual
    int Lex(yy::parser::semantic_type* yylval, yy::parser::location_type* yylloc)
        {return LexFromScanner(scanner_, yylval, yylloc);}

    bool SetEncoding(const std::string&)  {return true;}
};

static int test4(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: fb2toepub <input file> <output file>");
        return 1;
    }

    Ptr<InStm> pin      = CreateUnpackStm(argv[1]);
    pin                 = CreateInUnicodeStm(pin);

    MyDriver driver (pin);
    yy::parser parser(driver);
    
    parser.parse();

    printf("fb2toepub done!!!");
    return 0;
}

static int test5(int argc, char **argv)
{
    if(argc < 5)
    {
        printf("Usage: fb2toepub <css directory> <font directory> <input file> <output file>");
        return 1;
    }

    // create input stream
    Ptr<InStm> pin      = CreateUnpackStm(argv[3]);
    pin                 = CreateInUnicodeStm(pin);

    // create output stream
    Ptr<OutPackStm> pout = CreatePackStm(argv[4]);

    // create translite converter
    Ptr<XlitConv> xlitConv;// = CreateXlitConverter(CreateInUnicodeStm(CreateUnpackStm("D:/Development/eBooks/TestFiles/translit.xml")));

    int ret = Convert(pin, argv[1], argv[2], xlitConv, pout);
    if(ret)
        return ret;

    printf("fb2toepub done!!!");
    return 0;
}

static int test6(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage: fb2toepub <input file> <output file>");
        return 1;
    }

    Ptr<InStm> pin      = CreateInFileStm(argv[1]);
    Ptr<OutStm> pout    = CreateOutFileStm(argv[2]);

    DecodeBase64(pin, pout);

    return 0;
}
*/

//-----------------------------------------------------------------------
const char name[]       = "FB2 to EPUB format converter";
const char version[]    = "0.10";

static void Logo()
{
    printf("%s version %s\n\n", name, version);
}

static void Usage()
{
    printf("Usage:\n");
    printf("    fb2toepub <options> <input file> <output file>\n");
    printf("Options:\n");
    printf("    -s <path>           - path to .css style directory\n");
    printf("    -f <path>           - path to .ttf/.otf font directory\n");
    printf("    -sf <path>          - path to font and style directory\n");
    printf("    -t <filename>       - path to transliteration XML file (optional, no more that one)\n");
    printf("    -h                  - help\n\n");
    printf("Options are case-sensitive.\nSpace between -s/-s1/-f/-f1/-sf/-t and filename is mandatory.\n");
}

static int ErrorExit(const char *err)
{
    Logo();
    printf("Command line error: %s\n", err);
    Usage();
    return 1;
}

static void DeleteFile(const std::string &name)
{
#if defined(WIN32)
            _unlink(name.c_str());
#else
            unlink(name.c_str());
#endif
}

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
    std::string xlit, in, out;

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
        else if(in.empty())
            in = argv[i++];
        else if(out.empty())
            out = argv[i++];
        else
            return ErrorExit("unrecognized command line switch");

    // check
    if(in.empty() || out.empty())
        return ErrorExit("input or putput file is not defined");

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
    catch(const std::string &s)
    {
        fprintf(stderr, "%s\n", s.c_str());
        fprintf(stderr, "[%d]%s\n", errno, strerror(errno));
        if(fOutputFileCreated)
            DeleteFile(out);
        return 1;
    }
    catch(...)
    {
        fprintf(stderr, "Unknown error\n");
        fprintf(stderr, "[%d]%s\n", errno, strerror(errno));
        if(fOutputFileCreated)
            DeleteFile(out);
        return 1;
    }
}
