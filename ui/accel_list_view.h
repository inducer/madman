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




#ifndef ACCEL_LIST_VIEW_H_SEEN
#define ACCEL_LIST_VIEW_H_SEEN




#include <qkeysequence.h>
#include <qlistview.h>
#include <qtable.h>
#include <vector>
#include "utility/base.h"




class tKeyboardShortCut : public QObject
{
    Q_OBJECT
    QKeySequence	Keys;

  public:
    tKeyboardShortCut() 
    { }
    tKeyboardShortCut(const QKeySequence &keys);
    tKeyboardShortCut(const QKeySequence &keys, QObject *o, const char *slot);
    bool processKeyEvent(QKeyEvent *e);

  signals:
    void triggered();
};




class tAcceleratorListView : public QListView
{
    typedef QListView			super;
    typedef vector<tKeyboardShortCut *>	tKeysList;
    tKeysList				KeysList;

  public:
    tAcceleratorListView(QWidget *parent = 0, const char *name = "", WFlags f = 0 );
    ~tAcceleratorListView();

    void addShortCut(tKeyboardShortCut *sc);
    void keyPressEvent (QKeyEvent * e);
};




class tAcceleratorTable : public QTable
{
    typedef QTable			super;
    typedef vector<tKeyboardShortCut *>	tKeysList;
    tKeysList				KeysList;

  public:
    tAcceleratorTable(QWidget *parent = 0, const char *name = "");
    ~tAcceleratorTable();

    void addShortCut(tKeyboardShortCut *sc);
    void keyPressEvent (QKeyEvent * e);
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
