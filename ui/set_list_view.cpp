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




#include <qdragobject.h>
#include <qurl.h>

#include "database/song_set_tree.h"
#include "ui/set_list_view.h"
#include "ui/mainwin.h"




// tPlaylistNodeListViewItem ---------------------------------------------------
tPlaylistNodeListViewItem::tPlaylistNodeListViewItem(QListView *view, tPlaylistNode *node)
: QListViewItem(view, node->name()), Node(node)
{
  setRenameEnabled(0, true);
  setDropEnabled(true);
  setDragEnabled(true);
}




tPlaylistNodeListViewItem::tPlaylistNodeListViewItem(QListViewItem *item, tPlaylistNode *node)
    : QListViewItem(item, node->name()), Node(node)
{
  setRenameEnabled(0, true);
  setDropEnabled(true);
  setDragEnabled(true);
}




tPlaylistNode *tPlaylistNodeListViewItem::node()
{
  return Node;
}




bool tPlaylistNodeListViewItem::acceptDrop(const QMimeSource *mime) const
{
  return QUriDrag::canDecode(mime);
}




void tPlaylistNodeListViewItem::dropped(QDropEvent *event)
{
  if (QUriDrag::canDecode(event))
  {
    QStrList uri_list;
    QUriDrag::decode(event, uri_list);

    tPlaylist *set = Node->data();

    vector<tFilename> filenames;
    char *uri = uri_list.first();
    while (uri)
    {
      QString filename = QUriDrag::uriToLocalFile(uri);
      if (!filename.isNull())
      {
	try
	{
	  set->add((const char *) filename.utf8());
	}
	catch (exception &ex)
	{
	  qWarning("%s", ex.what());
	}
      }
      else
      {
	QUrl url(uri);
	if (url.protocol() == "madmanset" && event->source() == listView())
	  emit dropNode(url.path().mid(1), Node);
      }
      uri = uri_list.next();
    }
  }
}




// tSetListView ---------------------------------------------------------------
tSetListView::tSetListView(QWidget *parent, const char *name, WFlags f)
: super(parent, name, f)
{
  addColumn("Set");
  setRootIsDecorated(true);

}




QDragObject *tSetListView::dragObject()
{
  tPlaylistNodeListViewItem *item =
    (tPlaylistNodeListViewItem *)(currentItem());

  if (item == NULL)
    return NULL;

  QUrl url;
  url.setProtocol("madmanset");
  url.setHost("localhost");
  url.setPath(item->node()->qualifiedName());

  QStrList uri_list;
  uri_list.append(strdup(url.toString(true).latin1()));
  return new QUriDrag(uri_list, this);




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
}
