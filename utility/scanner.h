/*
madman - a music manager
Copyright (C) 2003  Andreas Kloeckner <ak@ixion.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/




#ifndef _HEADER_SEEN_SCANNER_H
#define _HEADER_SEEN_SCANNER_H




#include <vector>
#include <qregexp.h>




struct tToken
{
  int		TokenType;
  unsigned	Flags;
  QString	Text;
};




struct tTokenDescriptor
{
  int		TokenType;
  QRegExp	RegExp;
  bool		Ignore;
  unsigned	Flags;
};




typedef vector<tToken> tTokenList;
typedef vector<tToken> tTokenList;
typedef vector<tTokenDescriptor> tTokenDescriptorList;

class tScanner
{
    tTokenDescriptorList		TokenDescriptorList;

  public:
    void addTokenDescriptor(int type, const QString &pattern, bool minimal = false, bool ignore = false, unsigned flags = 0);
    void scan(const QString &victim, tTokenList &tokenlist);
};




#endif




// EMACS-FORMAT-TAG
//
// Local Variables:
// mode: C++
// eval: (c-set-style "stroustrup")
// eval: (c-set-offset 'access-label -2)
// eval: (c-set-offset 'inclass '++)
// c-basic-offset: 2
// tab-width: 8
// End:
