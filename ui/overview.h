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




#ifndef HEADER_SEEN_OVERVIEW_H
#define HEADER_SEEN_OVERVIEW_H




#include "utility/base.h"
#include <qlistview.h>




class tDatabase;
class tMainWindow;
class QPopupMenu;




QString getArtistCriterion(const QString &artist);
QString getAlbumCriterion(const QString &album);
QString getArtistAlbumCriterion(const QString &artist, const QString &album);




class tOverviewItem : public QListViewItem
{
    QString Criterion;

  public:
    tOverviewItem(QListView *view,  QListViewItem *after, const QString &label, const QString &criterion);
    tOverviewItem(QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion);
    const QString &criterion() const
    { return Criterion; }

    virtual tOverviewItem *findItemByCriterion(const QString &criterion);
};

class tExpandableOverviewItem : public tOverviewItem
{
    bool HasExpanded;
        
  public:
    tExpandableOverviewItem(QListView *view,  QListViewItem *after, const QString &label, const QString &criterion);
    tExpandableOverviewItem(QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion);
    void setOpen(bool o);
    virtual void addExpansion();

    tOverviewItem *findItemByCriterion(const QString &criterion);
};

class tArtistsOverviewItem : public tExpandableOverviewItem
{
    tDatabase	&Database;
    bool        GroupAlphanumerically;
    
  public:
    tArtistsOverviewItem(tDatabase &database, QListView *view,  QListViewItem *after, const QString &label, const QString &criterion, 
      bool group_by_alphanum);
    tArtistsOverviewItem(tDatabase &database, QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion,
      bool group_by_alphanum);
    void addExpansion();
};

class tAlbumOverviewItem : public tExpandableOverviewItem
{
    tDatabase	&Database;
    bool        GroupAlphanumerically;
    
  public:
    tAlbumOverviewItem(tDatabase &database, QListView *view,  QListViewItem *after, const QString &label, const QString &criterion, 
      bool group_by_alphanum);
    tAlbumOverviewItem(tDatabase &database, QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion, 
      bool group_by_alphanum);
    void addExpansion();
};

class tGenreOverviewItem : public tExpandableOverviewItem
{
    tDatabase	&Database;
  public:
    tGenreOverviewItem(tDatabase &database, QListView *view,  QListViewItem *after, const QString &label, const QString &criterion);
    tGenreOverviewItem(tDatabase &database, QListViewItem *parent, QListViewItem *after, const QString &label, const QString &criterion);
    void addExpansion();
};

class tPathOverviewItem : public tExpandableOverviewItem
{
    QString     Path;
    bool        ShowFullPath;

  public:
    tPathOverviewItem(const QString &path, QListViewItem *parent, 
		      QListViewItem *after, bool show_full_path = false);
    void addExpansion();
};

class tOverviewManager : public QObject
{
    Q_OBJECT;
    QListView	&ListView;
    QPopupMenu *Menu;

  public:
    tOverviewManager(QListView &listview);

    QPopupMenu *buildPopupMenu();

  public slots:
    void contextMenuRequested (QListViewItem * item, const QPoint & pos, int col);
    void slotPlayNow();
    void slotPlayNext();
    void slotPlayEventually();
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
