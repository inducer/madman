/* -*- C++ -*-
 *
 * noncopyable.h Copyright 2002 Shawn Willden <shawn@willden.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef _HEADER_SEEN_NONCOPYABLE_H
#define _HEADER_SEEN_NONCOPYABLE_H

/**
 * Derive your class from Noncopyable in order to make it noncopyable.
 */
class tNoncopyable
{
  public:
    // Silence gcc warning.
    tNoncopyable() { }
  private:
    // These methods are private and unimplemented, to prevent copying
    tNoncopyable(const tNoncopyable&);
    void operator=(const tNoncopyable&);
};




#endif // _HEADER_SEEN_NONCOPYABLE_H




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
