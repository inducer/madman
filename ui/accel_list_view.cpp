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




#include "accel_list_view.h"
#include "utility/base.h"




tKeyboardShortCut::tKeyboardShortCut(const QKeySequence &keys)
  : Keys(keys)
{
}




tKeyboardShortCut::tKeyboardShortCut(const QKeySequence &keys, QObject *o, const char *slot)
  : Keys(keys)
{
  connect(this, SIGNAL(triggered()), o, slot);
}




bool tKeyboardShortCut::processKeyEvent(QKeyEvent *e)
{
  int accel_state = 0;
  if (e->state() & ShiftButton)
    accel_state |= SHIFT;
  if (e->state() & ControlButton)
    accel_state |= CTRL;
  if (e->state() & MetaButton)
    accel_state |= META;
  if (e->state() & AltButton)
    accel_state |= ALT;

  if (QKeySequence(accel_state | e->key()) == Keys)
  {
    emit triggered();
    return true;
  }
  else
    return false;
}




// tAcceleratorListView -------------------------------------------------------
tAcceleratorListView::tAcceleratorListView(QWidget *parent, const char *name, WFlags f)
  : super(parent, name, f)
{
}




tAcceleratorListView::~tAcceleratorListView()
{
  FOREACH(first, KeysList, tKeysList)
    delete *first;
}




void tAcceleratorListView::addShortCut(tKeyboardShortCut *sc)
{
  KeysList.push_back(sc);
}





void tAcceleratorListView::keyPressEvent (QKeyEvent * e)
{
  if (hasFocus())
  {
    FOREACH(first, KeysList, tKeysList)
      if ((*first)->processKeyEvent(e))
	return;
  }

  super::keyPressEvent(e);
}




// tAcceleratorTable ----------------------------------------------------------
tAcceleratorTable::tAcceleratorTable(QWidget *parent, const char *name)
  : super(parent, name)
{
}




tAcceleratorTable::~tAcceleratorTable()
{
  FOREACH(first, KeysList, tKeysList)
    delete *first;
}




void tAcceleratorTable::addShortCut(tKeyboardShortCut *sc)
{
  KeysList.push_back(sc);
}





void tAcceleratorTable::keyPressEvent(QKeyEvent * e)
{
  if (hasFocus())
  {
    FOREACH(first, KeysList, tKeysList)
      if ((*first)->processKeyEvent(e))
	return;
  }

  super::keyPressEvent(e);
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
