/* -*- C++ -*-
 *
 * singleton.h Copyright 2002 Shawn Willden <shawn@willden.org>
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

#ifndef Singleton_h
#define Singleton_h




#include "utility/noncopyable.h"




/**
 * Derive your class A from Singleton<A> to make it a singleton.
 * Create a single instance and then the rest of the program can find
 * it with A::instance().
 */
template <class T> 
class tSingleton : tNoncopyable
{
  public:
    static T   *instance() { return instance_m; }
    
  protected:
    tSingleton() { if (!instance_m) instance_m = static_cast<T*>(this); }
    ~tSingleton() { instance_m = 0; }
    
  private:
    static T* instance_m;
};




#endif // Singleton_h




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
