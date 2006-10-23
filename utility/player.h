/*
madman - a music manager
Copyright (C) 2003  Andreas Kloeckner <ak@ixion.net>
              2005 Pauli Virtanen <pauli.virtanen@hut.fi>

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




#include <qtimer.h>
#include <qsettings.h>
#include "utility/base.h"




class tPlayerPreferences
{
  public:
    virtual void loadYourself(QSettings &settings) = 0;
    virtual void saveYourself(QSettings &settings) = 0;
    virtual void showUI(QWidget *parent) = 0;
};




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
    virtual unsigned getPlayListLength() = 0;
    virtual void setPlayListIndex(int index) = 0;
    virtual void removePlayListIndex(int index) = 0;
    virtual void clearPlaylist() = 0;

    virtual bool canGetValidStatus() = 0;
    virtual void ensureValidStatus() = 0;

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

    /** Return a tPlayerPreferences instance.

    The instance is assumed to be a member of the tPlayer and is not
    destroyed by the caller.

    May return NULL to indicate that there is nothing to configure,
    which is what the default implementation does.
    */
    
    virtual tPlayerPreferences *preferences() { return 0; }

  signals:
    void currentSongChanged(tFilename last_song, float play_time);
    void stateChanged();
};




class tPollingPlayer : public tPlayer
{
    Q_OBJECT

    bool LastPaused, LastPlaying;
    tFilename LastFilename;
    float LastSongTime;
    int LastPlaylistIndex;
    
    float AccumulatedPlayTime;
    time_t PlayStartTime;

    QTimer Timer;

  public:
    tPollingPlayer();

  protected slots:
    virtual void timer();

  protected:
    void resetState();

  private:
    void checkForStateChange();
    void checkForSongChange();
    void updatePlayTime();
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
    unsigned getPlayListLength();
    void setPlayListIndex(int index);
    void removePlayListIndex(int index);
    void clearPlaylist();

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

    tPlayerPreferences *preferences();

  protected slots:
    void slotCurrentSongChanged(tFilename last_song, float play_time);
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
