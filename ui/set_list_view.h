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




#ifndef _HEADER_SEEN_SET_LIST_VIEW_H
#define _HEADER_SEEN_SET_LIST_VIEW_H




#include <qlistview.h>




class tPlaylistNode;




class tPlaylistNodeListViewItem : public QObject, public QListViewItem
{
    Q_OBJECT
    tPlaylistNode *Node;

  public:
    tPlaylistNodeListViewItem(QListView *view, tPlaylistNode *node);
    tPlaylistNodeListViewItem(QListViewItem *item, tPlaylistNode *node);
    tPlaylistNode *node();

    bool acceptDrop(const QMimeSource *mime) const;
    void dropped(QDropEvent *drop);

  signals:
    void dropNode(const QString &node, tPlaylistNode *onto);
};




class tSetListView : public QListView
{
    typedef QListView super;

  public:
    tSetListView(QWidget *parent = 0, const char *name = "", WFlags f = 0);
    QDragObject *dragObject();
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
