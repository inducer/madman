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




#ifndef HEADER_SEEN_PLAYER_XMMS_H
#define HEADER_SEEN_PLAYER_XMMS_H




#include <qtimer.h>
#include <xmmsctrl.h>
#include "utility/player.h"




class tXMMSPlayer : public tPlayer
{
    Q_OBJECT

    int Session;
    bool PreviousPaused,PreviousPlaying;

    bool CurrentSongValid;
    tFilename CurrentSongFilename;

    QTimer Timer;

  public:
    tXMMSPlayer();
    void ensureRunning();
    QString name()
    {
      return "XMMS";
    }
    void playNow(tSongList const &songlist);
    void playNext(tSongList const &songlist);
    void playEventually(tSongList const &songlist);
    void setPlayList(tSongList const &songlist);
    void getPlayList(vector<tFilename> &songs);
    int getPlayListIndex();
    unsigned getPlayListLength();
    void setPlayListIndex(int index)
      {
	xmms_remote_set_playlist_pos(Session, index);
      }
    void removePlayListIndex(int index)
      {
	xmms_remote_playlist_delete(Session, index);
      }
    void clearPlaylist() 
      {
	xmms_remote_playlist_clear(Session);
      }

    tFilename currentFilename();
    QString currentTitle();

    bool canGetValidStatus()
      {
	return xmms_remote_is_running(Session);
      }
    bool isPlaying()
      {
	return xmms_remote_is_playing(Session);
      }
    bool isPaused()
      {
	return xmms_remote_is_paused(Session);
      }

    float currentTime();
    float totalTime();

    void play();
    void stop();
    void pause();
    void skipForward();
    void skipBack();

    void skipToSeconds(float seconds);

  private slots:
    void timer();
    bool haveValidPlaylistPosition();
    bool isValidPlaylistPosition(int i);
    int enqueue(const tSongList &songlist);
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
