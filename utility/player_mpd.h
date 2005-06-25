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

/* player_mpd - Music Player Daemon back end
 * Pauli Virtanen <pauli.virtanen@hut.fi>
 */



#ifndef HEADER_SEEN_PLAYER_MPD_H
#define HEADER_SEEN_PLAYER_MPD_H




#include <qtimer.h>
#include "libmpdclient.h"
#include "utility/player.h"




class tMPDPlayer : public tPlayer
{
    Q_OBJECT

    tFilename MusicPath;
    
    QString Host;
    int Port;

    bool PreviousPaused, PreviousPlaying;
    tFilename CurrentSongFilename;

    QTimer Timer;

    mpd_Connection* Conn;

  public:
    tMPDPlayer();
    QString name()
      {
        return "MPD";
      }
    void playNow(tSongList const &songlist);
    void playNext(tSongList const &songlist);
    void playEventually(tSongList const &songlist);
    void setPlayList(tSongList const &songlist);
    void getPlayList(vector<tFilename> &songs);
    int getPlayListIndex();
    unsigned getPlayListLength();
    void setPlayListIndex(int index);
    void removePlayListIndex(int index);
    void clearPlaylist();
    
    tFilename currentFilename();
    QString currentTitle();

    bool canGetValidStatus();
    void ensureValidStatus();
    
    bool isPlaying();
    bool isPaused();

    float currentTime();
    float totalTime();

    void play();
    void stop();
    void pause();
    void skipForward();
    void skipBack();

    void skipToSeconds(float seconds);

  private:
    void spyMusicPath();
    void ensureConnection();
    tFilename convertFileFromMPD(tFilename const& file);
    tFilename convertFileToMPD(tFilename const& file); 
    int enqueue(const tSongList &songlist);
    bool haveValidPlaylistPosition();
    bool isValidPlaylistPosition(int i);

    void finishCommand();
    void closeConnection();

   
  private slots:
    void timer();
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
