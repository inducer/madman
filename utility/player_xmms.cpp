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
#include <qinputdialog.h>
#include <qapplication.h>
#include <limits.h>

#include "utility/base.h"
#include "utility/player_xmms.h"
#include "database/song.h"




// tXMMSPlayerPreferences -----------------------------------------------------
void tXMMSPlayerPreferences::loadYourself(QSettings &settings)
{
  Session = settings.readNumEntry("/madman/xmms/session_number", 0);
}




void tXMMSPlayerPreferences::saveYourself(QSettings &settings)
{
  settings.writeEntry("/madman/xmms/session_number", Session);
}




void tXMMSPlayerPreferences::showUI(QWidget *parent)
{
  bool ok;
  int result = QInputDialog::getInteger(
      qApp->translate("tXMMSPlayerPreferences", "madman - XMMS Player"),
      qApp->translate("tXMMSPlayerPreferences", "Session Number"),
      Session, 0, INT_MAX, 1, &ok, parent);
  if (ok)
    Session = result;
}




// tXMMSPlayer ----------------------------------------------------------------
tXMMSPlayer::tXMMSPlayer()
{
  Session = 0;

  CurrentSongValid = false;
}




void tXMMSPlayer::ensureRunning()
{
  if (!xmms_remote_is_running(Session))
  {
    system("xmms &");
    unsigned tries = 5;

    while (tries-- && !xmms_remote_is_running(Session))
      sleep(1);

    if (!xmms_remote_is_running(Session))
      throw runtime_error("failed to start xmms.");
  }
}




void tXMMSPlayer::playNow(tSongList const &songlist)
{
  ensureRunning();
  if (songlist.size() == 0)
    return;

  gint start_playlist_pos;
  if (isPlaying() || isPaused())
    start_playlist_pos = enqueue(songlist);
  else
    start_playlist_pos = enqueueAtEnd(songlist);

  xmms_remote_set_playlist_pos(Session, start_playlist_pos);
  if (!xmms_remote_is_playing(Session))
    xmms_remote_play(Session);
}




void tXMMSPlayer::playNext(tSongList const &songlist)
{
  ensureRunning();

  if (songlist.size() == 0)
    return;

  enqueue(songlist);
}




void tXMMSPlayer::playEventually(tSongList const &songlist)
{
  ensureRunning();

  if (songlist.size() == 0)
    return;

  enqueueAtEnd(songlist);
}




void tXMMSPlayer::setPlayList(tSongList const &songlist)
{
  ensureRunning();
  xmms_remote_playlist_clear(Session);
  gint playlist_pos = 0;
  FOREACH_CONST(first, songlist, tSongList)
  {
    xmms_remote_playlist_ins_url_string(Session, 
	const_cast<char *>((*first)->filename().c_str()), 
	playlist_pos++);
  }
}




void tXMMSPlayer::getPlayList(vector<tFilename> &songs)
{
  ensureRunning();

  songs.clear();

  int length = xmms_remote_get_playlist_length(Session);
  for (int i = 0; i < length; i++)
  {
    char *song = xmms_remote_get_playlist_file(Session, i);
    if (song)
      songs.push_back(song);
  }
}




int tXMMSPlayer::getPlayListIndex()
{
  gint playlist_pos = xmms_remote_get_playlist_pos(Session);
  if (isValidPlaylistPosition(playlist_pos))
    return playlist_pos;
  else
    return -1;
}




unsigned tXMMSPlayer::getPlayListLength()
{
  gint len = xmms_remote_get_playlist_length(Session);
  if (len < 0)
    throw tRuntimeError(tr("Error getting playlist length, negative value from XMMS"));
  else
    return (unsigned) len;
}




tFilename tXMMSPlayer::currentFilename()
{
  if (!haveValidPlaylistPosition())
    return "";

  const char *name = xmms_remote_get_playlist_file(Session, 
      xmms_remote_get_playlist_pos(Session));
  if (name == NULL)
    return "";
  else
    return name;
}




QString tXMMSPlayer::currentTitle()
{
  if (!haveValidPlaylistPosition())
    return "";

  const char *name = xmms_remote_get_playlist_title(Session, 
      xmms_remote_get_playlist_pos(Session));
  if (name == NULL)
    return "";
  else
    return name;
}




float tXMMSPlayer::currentTime()
{
  if (isPlaying() || isPaused())
    return xmms_remote_get_output_time(Session) / 1000.;
  else
    return 0;
}




float tXMMSPlayer::totalTime()
{
  if (!haveValidPlaylistPosition())
    return 0;

  return xmms_remote_get_playlist_time(Session, 
      xmms_remote_get_playlist_pos(Session)) / 1000.;
}




void tXMMSPlayer::play()
{
  ensureRunning();
  xmms_remote_play(Session);
  timer();
}




void tXMMSPlayer::stop()
{
  ensureRunning();
  xmms_remote_stop(Session);
  timer();
}




void tXMMSPlayer::pause()
{
  ensureRunning();
  if (!isPaused())
  {
    xmms_remote_pause(Session);
    timer();
  }
}




void tXMMSPlayer::skipForward()
{
  ensureRunning();
  xmms_remote_playlist_next(Session);
  timer();
}




void tXMMSPlayer::skipBack()
{
  ensureRunning();
  xmms_remote_playlist_prev(Session);
  timer();
}




void tXMMSPlayer::skipToSeconds(float seconds)
{
  xmms_remote_jump_to_time(Session, int(seconds * 1000));
}




tPlayerPreferences *tXMMSPlayer::preferences()
{
  return &Preferences;
}




// private stuff --------------------------------------------------------------
bool tXMMSPlayer::haveValidPlaylistPosition()
{
  return isValidPlaylistPosition(xmms_remote_get_playlist_pos(Session));
}




bool tXMMSPlayer::isValidPlaylistPosition(int pos)
{
  int length = xmms_remote_get_playlist_length(Session);
  if (length == 0)
    return false;

  return pos >= 0 && pos < length;
}




int tXMMSPlayer::enqueue(const tSongList &songlist)
{
  if (xmms_remote_get_playlist_length(Session) == 0) 
    return enqueueAt(songlist, 0);
  else 
    return enqueueAt(songlist, 
                     xmms_remote_get_playlist_pos(Session) + 1);
}




int tXMMSPlayer::enqueueAtEnd(const tSongList &songlist)
{
  if (xmms_remote_get_playlist_length(Session) == 0) 
    return enqueueAt(songlist, 0);
  else 
    return enqueueAt(songlist, 
                     xmms_remote_get_playlist_length(Session));
}




int tXMMSPlayer::enqueueAt(const tSongList &songlist, gint insert_pos)
{
  gint start_pos = insert_pos;
  FOREACH_CONST(first, songlist, tSongList)
    xmms_remote_playlist_ins_url_string(Session, 
        const_cast<char *>((*first)->filename().c_str()), insert_pos++);
  return start_pos;
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

