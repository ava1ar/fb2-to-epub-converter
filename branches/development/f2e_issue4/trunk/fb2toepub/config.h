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


#ifndef FB2TOEPUB__CONFIG_H
#define FB2TOEPUB__CONFIG_H

#define FB2TOEPUB_DECL

//-----------------------------------------------------------------------
// SUPPRESS EMPTY TITLES IN TABLE OF CONTENTS
// If the value is nonzero, fb2 section without title don't appear in TOC,
// and their nested section are moved one level up.
// Otherwise, every section without title is assigned a title "- - - - -"
//-----------------------------------------------------------------------
#ifndef FB2TOEPUB_SUPPRESS_EMPTY_TITLES
#define FB2TOEPUB_SUPPRESS_EMPTY_TITLES 1
#endif

//-----------------------------------------------------------------------
// ENABLE IDS IN TABLE OF CONTENTS
// If the value is nonzero, TOC contains references to file only
// Otherwise, TOC contains "file#id" references.
//-----------------------------------------------------------------------
#ifndef FB2TOEPUB_TOC_REFERS_FILES_ONLY
#define FB2TOEPUB_TOC_REFERS_FILES_ONLY 1
#endif


#endif
