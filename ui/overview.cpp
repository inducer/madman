/*
madman - a music manager
Copyright (C) 2003  Andreas Kloeckner <mathem@tiker.net>

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




#include "utility/player.h"
#include "database/database.h"
#include "database/song_list_tools.h"
#include "ui/overview.h"
#include "ui/mainwin.h"
#include <qpopupmenu.h>
#include <qlistview.h>
#include <qdir.h>
#include <qtextcodec.h>
#include <algorithm>




// private helpers ------------------------------------------------------------
namespace
{
  QString normalize(const QString &victim)
  {
    return victim.lower();
  }
}




// public helpers -------------------------------------------------------------
QString getArtistCriterion(const QString &artist)
{
  return QString("~artist(complete:%1)").arg(quoteString(normalize(artist)));
}




QString getAlbumCriterion(const QString &album)
{
  return QString("~album(complete:%1)").arg(quoteString(normalize(album)));
}




QString getArtistAlbumCriterion(const QString &artist, const QString &album)
{
  return QString("~artist(complete:%1)&~album(complete:%2)")
    .arg(quoteString(normalize(artist)))
    .arg(quoteString(normalize(album)));
}




// tOverviewItem --------------------------------------------------------------
tOverviewItem::tOverviewItem(QListView *view,  QListViewItem *after, const QString &label, const QString &criterion)
: QListViewItem(view, after, label), Criterion(criterion)
{ 
}




tOverviewItem::tOverviewItem(QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion)
: QListViewItem(parent, after, label), Criterion(criterion)
{
}





tOverviewItem *tOverviewItem::findItemByCriterion(const QString &criterion)
{
  if (Criterion == criterion)
    return this;

  tOverviewItem *oi = dynamic_cast<tOverviewItem *>(firstChild());
  while (oi)
  {
    tOverviewItem *result = oi->findItemByCriterion(criterion);
    if (result)
      return result;
    oi = dynamic_cast<tOverviewItem *>(oi->nextSibling());
  }

  return NULL;
}




// tExpandableOverviewItem --------------------------------------------------------------
tExpandableOverviewItem::tExpandableOverviewItem(QListView *view,  QListViewItem *after, const QString &label, const QString &criterion)
: tOverviewItem(view, after, label, criterion)
{ 
  setExpandable(true);
  HasExpanded = false;
}




tExpandableOverviewItem::tExpandableOverviewItem(QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion)
: tOverviewItem(parent, after, label, criterion)
{
  setExpandable(true);
  HasExpanded = false;
}




void tExpandableOverviewItem::setOpen(bool o)
{
  if (!HasExpanded && o)
  {
    listView()->setUpdatesEnabled(false);
    addExpansion();
    listView()->setUpdatesEnabled(true);
    HasExpanded = true;
  }
  tOverviewItem::setOpen(o);
}




void tExpandableOverviewItem::addExpansion()
{
}




tOverviewItem *tExpandableOverviewItem::findItemByCriterion(const QString &criterion)
{
  if (!HasExpanded)
  {
    listView()->setUpdatesEnabled(false);
    addExpansion();
    listView()->setUpdatesEnabled(true);
    HasExpanded = true;
  }

  return tOverviewItem::findItemByCriterion(criterion);
}




// tool routine ---------------------------------------------------------------
namespace
{
  QChar firstLetterOrNumber(const QString &str)
  {
    for (unsigned i = 0; i < str.length(); i++)
      if (str[i].isLetter() || str[i].isNumber())
        return str[i].upper();
    return QChar(0);
  }



  int buildOverview(tSongList &songs, tSongField field1, tSongField field2, QListViewItem *parent, const QString & critprefix = "", 
                           const bool group_alphanumerically = false)
  {
    sort(songs, field1);

    QListViewItem *last_item = NULL;
    QListViewItem *alphanum_item = NULL;
    QString last_entry = QString::null,
            last_alphanum = QString::null;

    int distinct_count = 0;

    tSongList::iterator first = songs.begin(), last = songs.end();
    while (first != last)
    {
      bool belongs_to_none = false;

      QString this_nonnormalized_entry = (*first)->fieldText(field1);
      QString this_entry = normalize(this_nonnormalized_entry);
      if (this_nonnormalized_entry == "")
      {
	this_nonnormalized_entry = qApp->translate("Overview", "<none>");
        belongs_to_none = true;
      }

      tSongList this_songs;
      while (first != last && this_entry == normalize((*first)->fieldText(field1)))
	this_songs.push_back(*first++);

      QString criterion = QString("%1~%2(complete:%3)")
	.arg(critprefix)
	.arg(getFieldIdentifier(field1)) 
	.arg(quoteString(this_entry));
	
      if (!group_alphanumerically || belongs_to_none)
        alphanum_item = parent;
      else 
      {
        QChar first_char = firstLetterOrNumber(this_nonnormalized_entry);
        
	if (first_char.isLetter())
	{      
          if (last_alphanum != first_char)
          {      
            alphanum_item = new tOverviewItem(parent, alphanum_item, first_char, QString::null);
	    last_alphanum = first_char;
	  }
	}
	else
	{
	  if (last_alphanum != "0..9")
	  {
	    alphanum_item = new tOverviewItem(parent, alphanum_item, "0..9", QString::null);
	    last_alphanum = "0..9";
	  }
	}
      }          
         
      QListViewItem *entry_item = new tOverviewItem(
	  alphanum_item, last_item,
	  QString("%1 (%2)")
	    .arg(this_nonnormalized_entry)
	    .arg(this_songs.size()), 
	    criterion);

      if (field2 != FIELD_COUNT)
	buildOverview(this_songs, field2, FIELD_COUNT, entry_item, criterion+"&", false);

      last_item = entry_item;
      distinct_count++;
    }
    return distinct_count;
  }
}




// tArtistsOverviewItem -------------------------------------------------------
tArtistsOverviewItem::tArtistsOverviewItem(tDatabase &database, QListView *view,  QListViewItem *after, const QString &label, const QString &criterion, bool group_alphanumerically)
: tExpandableOverviewItem(view, after, label, criterion), 
  Database(database),
  GroupAlphanumerically(group_alphanumerically)
{ 
}




tArtistsOverviewItem::tArtistsOverviewItem(tDatabase &database, QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion, bool group_alphanumerically)
: tExpandableOverviewItem(parent, after, label, criterion),
  Database(database),
  GroupAlphanumerically(group_alphanumerically)
{ 
}




void tArtistsOverviewItem::addExpansion()
{
  tSongList songs;
  FOREACH_CONST(first, Database.SongCollection, tSongCollection)
    songs.push_back(*first);
  buildOverview(songs, FIELD_ARTIST, FIELD_ALBUM, this, "", GroupAlphanumerically);
}




// tAlbumOverviewItem -------------------------------------------------------
tAlbumOverviewItem::tAlbumOverviewItem(tDatabase &database, QListView *view,  QListViewItem *after, const QString &label, const QString &criterion, bool group_alphanumerically)
: tExpandableOverviewItem(view, after, label, criterion), 
  Database(database),
  GroupAlphanumerically(group_alphanumerically)
{ 
}




tAlbumOverviewItem::tAlbumOverviewItem(tDatabase &database, QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion, bool group_alphanumerically)
: tExpandableOverviewItem(parent, after, label, criterion),
  Database(database),
  GroupAlphanumerically(group_alphanumerically)
{ 
}




void tAlbumOverviewItem::addExpansion()
{
  tSongList songs;
  FOREACH_CONST(first, Database.SongCollection, tSongCollection)
    songs.push_back(*first);
  buildOverview(songs, FIELD_ALBUM, FIELD_COUNT, this, "", GroupAlphanumerically);
}





// tGenreOverviewItem -------------------------------------------------------
tGenreOverviewItem::tGenreOverviewItem(tDatabase &database, QListView *view,  QListViewItem *after, const QString &label, const QString &criterion)
: tExpandableOverviewItem(view, after, label, criterion), 
  Database(database)
{ 
}




tGenreOverviewItem::tGenreOverviewItem(tDatabase &database, QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion)
: tExpandableOverviewItem(parent, after, label, criterion),
  Database(database)
{ 
}




void tGenreOverviewItem::addExpansion()
{
  tSongList songs;
  FOREACH_CONST(first, Database.SongCollection, tSongCollection)
    songs.push_back(*first);
  buildOverview(songs, FIELD_GENRE, FIELD_COUNT, this, false);
}




// tPathOverviewItem -----------------------------------------------------------
tPathOverviewItem::tPathOverviewItem(const QString &path,
		      QListViewItem *parent, QListViewItem *after, bool show_full_path)
  : tExpandableOverviewItem(parent, NULL, "", 
			    QString("~filename(startswith:%1)").arg(quoteString(path+"/"))),
    Path(path),
    ShowFullPath(show_full_path)
{
  if (show_full_path)
    setText(0, path);
  else
  {
    QDir dir(path);
    setText(0, dir.dirName());
  }
}




void tPathOverviewItem::addExpansion()
{
  QDir dir(Path);
  dir.setSorting(QDir::DirsFirst | QDir::Name);
  QStringList entries = dir.entryList();

  QListViewItem *last_item = NULL;
  FOREACH(first, entries, QStringList)
  {
    QString full_path = Path + "/" + *first;
    QDir subdir(full_path);
    if (!subdir.exists() || *first == "." || *first == "..")
      continue;
    last_item = new tPathOverviewItem(subdir.absPath(), this, last_item);
  }
}




// tOverviewManager -----------------------------------------------------------
tOverviewManager::tOverviewManager(QListView &listview)
  : ListView(listview)
{
  connect(&ListView, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
           this, SLOT(contextMenuRequested(QListViewItem *, const QPoint &, int)));
  Menu = buildPopupMenu();
}




QPopupMenu *tOverviewManager::buildPopupMenu()
{
  QPopupMenu *menu = new QPopupMenu();

  QPopupMenu *play_menu = new QPopupMenu(&ListView);
  play_menu->insertItem(tr("&Play now"), this, SLOT(slotPlayNow()));
  play_menu->insertItem(tr("Play &next"), this, SLOT(slotPlayNext()));
  play_menu->insertItem(tr("Play &eventually"), this, SLOT(slotPlayEventually()));
  menu->insertItem(tr("&Play"), play_menu);

  return menu;
}




void tOverviewManager::contextMenuRequested (QListViewItem * item, const QPoint & pos, int col)
{
  Menu->popup(pos);
}




void tOverviewManager::slotPlayNow()
{
  tOverviewItem *my_item = dynamic_cast<tOverviewItem *>(ListView.selectedItem());
  if (my_item)
  {
    tPlaylist set;
    set.setSongCollection(&tProgramBase::database().SongCollection);
    set.setCriterion(my_item->criterion());
    set.reevaluateCriterion();

    tSongList list;
    set.render(list);

    tProgramBase::preferences().Player.playNow(list);
  }
}




void tOverviewManager::slotPlayNext()
{
  tOverviewItem *my_item = dynamic_cast<tOverviewItem *>(ListView.selectedItem());
  if (my_item)
  {
    tPlaylist set;
    set.setSongCollection(&tProgramBase::database().SongCollection);
    set.setCriterion(my_item->criterion());
    set.reevaluateCriterion();

    tSongList list;
    set.render(list);

    tProgramBase::preferences().Player.playNext(list);
  }
}




void tOverviewManager::slotPlayEventually()
{
  tOverviewItem *my_item = dynamic_cast<tOverviewItem *>(ListView.selectedItem());
  if (my_item)
  {
    tPlaylist set;
    set.setSongCollection(&tProgramBase::database().SongCollection);
    set.setCriterion(my_item->criterion());
    set.reevaluateCriterion();

    tSongList list;
    set.render(list);

    tProgramBase::preferences().Player.playEventually(list);
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

