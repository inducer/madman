/*
madman - a music manager
Copyright (C) 2004  Andreas Kloeckner <ak@ixion.net>

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




#ifndef HEADER_SEEN_ACTION_WITH_INFO_H
#define HEADER_SEEN_ACTION_WITH_INFO_H




#include <qaction.h>
#include "utility/base.h"




class tActionWithData : public QAction
{
    tObject   *Data;
    bool      OwnData;
    
  public:
    tActionWithData(QObject *parent, tObject *data, bool own = true, const char *name = 0);
    ~tActionWithData();

  signals:
    void activated(tObject *data);
    void toggled(bool on, tObject *data);

  protected slots:
    void activatedSlot();
    void toggledSlot(bool on);
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
