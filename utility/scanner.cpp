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




#include <stdexcept>
#include "base.h"
#include "scanner.h"




void tScanner::addTokenDescriptor( int type, const QString &pattern, bool minimal, bool ignore, unsigned flags )
{
  tTokenDescriptor d;
  d.TokenType = type;
  d.RegExp = "^"+pattern;
  d.RegExp.setMinimal( minimal );
  d.Ignore = ignore;
  d.Flags = flags;
  TokenDescriptorList.push_back( d );
}




void tScanner::scan( const QString &victim, tTokenList &tokenlist )
{
  int index = 0;

  tokenlist.clear();
  while ( index < (int) victim.length() )
  {
    bool found_a_token = false;
    FOREACH_CONST( first, TokenDescriptorList, tTokenDescriptorList )
    {
      if ( first->RegExp.search( victim.mid( index ) ) != -1 )
      {
	if ( !first->Ignore )
	{
	  tToken token;
	  token.TokenType = first->TokenType;
	  token.Flags = first->Flags;
	  token.Text = victim.mid( index, first->RegExp.matchedLength() );
	  tokenlist.push_back( token );
	}
	index += first->RegExp.matchedLength();
	found_a_token = first->RegExp.matchedLength() > 0;
	if ( found_a_token )
	  break;
      }
    }

    if (!found_a_token)
      throw runtime_error( "invalid token at: " + QString2string( victim.mid( index, 20 ) ) );
  }
}




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

