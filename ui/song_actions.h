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





#ifndef HEADER_SEEN_SONG_ACTIONS_H
#define HEADER_SEEN_SONG_ACTIONS_H




#include <qaction.h>
#include "database/program_base.h"
#include "ui/mainwin.h"




class tSelfAction : public QAction
{
    Q_OBJECT;

  public:
    tSelfAction(const QString &menuText, QObject *parent)
      : QAction(menuText, QKeySequence(), parent)
      {
	connect(this, SIGNAL(activated()),
		this, SLOT(go()));
      }

  protected slots:
    virtual void go() = 0;
};




class tSetPlaylistIndexAction : public tSelfAction
{
    int Index;
  public:
    tSetPlaylistIndexAction(int index, const QString &menuText, QObject *parent)
      : tSelfAction(menuText, parent), Index(index)
      {
      }
    void go()
      {
	tProgramBase::preferences().Player.setPlayListIndex(Index);
      }
};





class tRemovePlaylistIndexAction : public tSelfAction
{
    Q_OBJECT

    int Index;

  public:
    tRemovePlaylistIndexAction(int index, const QString &menuText, QObject *parent)
      : tSelfAction(menuText, parent), Index(index)
      {
      }
    void go()
      {
	tProgramBase::preferences().Player.removePlayListIndex(Index);
        emit songRemoved();
      }

    signals:
      void songRemoved();
};





class tRateAction : public tSelfAction
{
    tUniqueId UniqueId;
    int Rating;

  public:
    tRateAction(tUniqueId uid, int rating, const QString &menuText, QObject *parent)
      : tSelfAction(menuText, parent), UniqueId(uid), Rating(rating)
      {
      }
    void go()
      {
	tSong *song = tProgramBase::database().SongCollection.getByUniqueId(UniqueId);
	song->setRating(Rating);
      }
};




class tHighlightAction : public tSelfAction
{
    tMainWindow *MainWindow;
    tUniqueId UniqueId;

  public:
    tHighlightAction(tMainWindow *mw, tUniqueId uid, const QString &menuText, QObject *parent)
      : tSelfAction(menuText, parent), MainWindow(mw), UniqueId(uid)
      {
      }
    void go()
      {
	tSong *song = tProgramBase::database().SongCollection.getByUniqueId(UniqueId);
	MainWindow->highlightSong(song);
      }
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
