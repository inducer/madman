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




#include <unistd.h>
#include <cstdlib>
#include <stdexcept>
#include "utility/player_xmms.h"




// tPlayer --------------------------------------------------------------------
tPlayer::~tPlayer()
{
}




void tPlayer::skipTo(float percentage)
{
  float total = totalTime();
  if (total == 0)
    return;

  skipToSeconds(int(total * percentage / 100));
}




// tPlayerFacade --------------------------------------------------------------
void tPlayerFacade::setBackend(tPlayer *backend)
{
  if (Backend.get())
    disconnect(Backend.get(), NULL, backend, NULL);
  auto_ptr<tPlayer> be(backend);
  Backend = be;
  if (Backend.get())
  {
    connect(Backend.get(), SIGNAL(currentSongChanged()), this, SLOT(slotCurrentSongChanged()));
    connect(Backend.get(), SIGNAL(stateChanged()), this, SLOT(slotStateChanged()));
  }
}




QString tPlayerFacade::name()
{
  if (Backend.get()) return Backend->name();
  else return QString();
}
void tPlayerFacade::playNow(tSongList const &songlist)
{
  if (Backend.get()) Backend->playNow(songlist);
}
void tPlayerFacade::playNext(tSongList const &songlist)
{
  if (Backend.get()) Backend->playNext(songlist);
}
void tPlayerFacade::playEventually(tSongList const &songlist)
{
  if (Backend.get()) Backend->playEventually(songlist);
}
void tPlayerFacade::setPlayList(tSongList const &songlist)
{
  if (Backend.get()) Backend->setPlayList(songlist);
}
tFilename tPlayerFacade::currentFilename()
{
  if (Backend.get()) return Backend->currentFilename();
  else return "";
}
QString tPlayerFacade::currentTitle()
{
  if (Backend.get()) return Backend->currentTitle();
  else return QString::null;
}
void tPlayerFacade::getPlayList(vector<tFilename> &songs)
{
  if (Backend.get()) Backend->getPlayList(songs);
  else songs.clear();
}
int tPlayerFacade::getPlayListIndex()
{
  if (Backend.get()) return Backend->getPlayListIndex();
  else return -1;
}
unsigned tPlayerFacade::getPlayListLength()
{
  if (Backend.get()) return Backend->getPlayListLength();
  else return 0;
}
void tPlayerFacade::setPlayListIndex(int index)
{
  if (Backend.get()) Backend->setPlayListIndex(index);
}
void tPlayerFacade::removePlayListIndex(int index)
{
  if (Backend.get()) Backend->removePlayListIndex(index);
}
void tPlayerFacade::clearPlaylist()
{
  if (Backend.get()) Backend->clearPlaylist();
}
bool tPlayerFacade::canGetValidStatus()
{
  if (Backend.get()) return Backend->canGetValidStatus();
  else return false;
}
void tPlayerFacade::ensureValidStatus()
{
  if (Backend.get()) Backend->ensureValidStatus();
  else throw tRuntimeError(tr("Failed to ensure that "
                              "valid player status "
                              "information can be obtained."));
}
bool tPlayerFacade::isPlaying()
{
  if (Backend.get()) return Backend->isPlaying();
  else return false;
}
bool tPlayerFacade::isPaused()
{
  if (Backend.get()) return Backend->isPaused();
  else return false;
}
float tPlayerFacade::currentTime()
{
  if (Backend.get()) return Backend->currentTime();
  else return 0;
}
float tPlayerFacade::totalTime()
{
  if (Backend.get()) return Backend->totalTime();
  else return 0;
}
void tPlayerFacade::play()
{
  if (Backend.get()) Backend->play();
}
void tPlayerFacade::stop()
{
  if (Backend.get()) Backend->stop();
}
void tPlayerFacade::pause()
{
  if (Backend.get()) Backend->pause();
}
void tPlayerFacade::skipForward()
{
  if (Backend.get()) Backend->skipForward();
}
void tPlayerFacade::skipBack()
{
  if (Backend.get()) Backend->skipBack();
}
void tPlayerFacade::skipToSeconds(float seconds)
{
  if (Backend.get()) Backend->skipToSeconds(seconds);
}
void tPlayerFacade::slotCurrentSongChanged()
{
  emit currentSongChanged();
}
void tPlayerFacade::slotStateChanged()
{
  emit stateChanged();
}




// factory interface ----------------------------------------------------------
void listPlayers(vector<QString> &players)
{
  players.clear();
  players.push_back("XMMS");
}




tPlayer *createPlayer(QString const &name)
{
  if (name == "XMMS")
  {
    return new tXMMSPlayer;
  }
  else
    return NULL;
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
