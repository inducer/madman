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




#ifndef HEADER_SEEN_PLAYER_H
#define HEADER_SEEN_PLAYER_H
#include "utility/base.h"




class tPlayer : public QObject
{
    Q_OBJECT
  public:
    virtual ~tPlayer();

  public slots:
    virtual QString name() = 0;
    virtual void playNow(tSongList const &songlist) = 0;
    virtual void playNext(tSongList const &songlist) = 0;
    virtual void playEventually(tSongList const &songlist) = 0;
    virtual void setPlayList(tSongList const &songlist) = 0;
    virtual tFilename currentFilename() = 0;
    virtual QString currentTitle() = 0;
    virtual void getPlayList(vector<tFilename> &songs) = 0;
    virtual int getPlayListIndex() = 0;
    virtual void setPlayListIndex(int index) = 0;
    virtual void removePlayListIndex(int index) = 0;
    virtual void clearPlaylist() = 0;

    virtual bool isPlaying() = 0;
    virtual bool isPaused() = 0;
    virtual float currentTime() = 0;
    virtual float totalTime() = 0;

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void skipForward() = 0;
    virtual void skipBack() = 0;
    virtual void skipToSeconds(float seconds) = 0;
    void skipTo(float percentage);

  signals:
    void currentSongChanged();
    void stateChanged();
};




class tPlayerFacade : public tPlayer
{
    Q_OBJECT

    auto_ptr<tPlayer>	Backend;
  public slots:
    void setBackend(tPlayer *backend);

    QString name();
    void playNow(tSongList const &songlist);
    void playNext(tSongList const &songlist);
    void playEventually(tSongList const &songlist);
    void setPlayList(tSongList const &songlist);
    tFilename currentFilename();
    QString currentTitle();
    void getPlayList(vector<tFilename> &songs);
    int getPlayListIndex();
    void setPlayListIndex(int index);
    void removePlayListIndex(int index);
    void clearPlaylist();

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

  protected slots:
    void slotCurrentSongChanged();
    void slotStateChanged();
};




void listPlayers(vector<QString> &players);
tPlayer *createPlayer(QString const &name);




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
