#
#  Copyright (C) 2010, 2011 Alexey Bobkov
#
#  This file is part of Fb2toepub converter.
#
#  Fb2toepub converter is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Fb2toepub converter is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Fb2toepub converter.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Fb2toepub converter makefile for Microsoft Visual C++
#
# There are build problems for Windows with scanner.l and scanner.cpp.
# The cause is that there is no flex for Windows working properly in C++ mode and compatible with modern C++.
# Possible solutions:
# - Install cygwin and use cygwin flex. File FlexLexer.h deployed with cygwin-1.7.7-1 is compatible with MSVC++.
#   Copy it to some directory and add this directory to Visual C++ include directory list.
# - If you don't want to install cygwin, you can use existent scanner.cpp and the header file FlexLexer.h
#   in source directory fb2toepub/win_FlexLexer/. However keep in mind that this file is from cygwin-1.7.7-1 package
# and it may not be refreshed later if I decide to switch to next version of cygwin.
#
# And don't forget to add FlexLexer.h directory to INCLUDE environment variable!
#
# To build, use the following command:
# 	nmake -f msvc.mak
# or
# 	nmake -f msvc.mak VERSION=<version>
#
#

all: setversion
	vcbuild /nologo /useenv fb2toepub.sln "Release Static|Win32"

setversion:
!IFDEF VERSION
	set CL=/DFB2TOEPUB_VERSION#$(VERSION)
!ELSE
	@echo 	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo	WARNING: Version not defined!
	@echo 	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo
	set CL=/DFB2TOEPUB_VERSION#"Custom Windows Build"
!ENDIF

clean:
	vcbuild /nologo /clean