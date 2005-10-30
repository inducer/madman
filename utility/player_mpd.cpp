/*
madman - a music manager
Music Player Daemon support
Copyright (C) 2005  Pauli Virtanen <pauli.virtanen@hut.fi>

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

/* 
 *
 * TODO:
 *
 * - Figure out what is the most sensible way to deal with the MusicPath
 *   issue.
 * 
 */


#include <unistd.h>
#include <qtextstream.h>
#include <qapplication.h>
#include "libmpdclient.h"

#include "utility/base.h"
#include "utility/player_mpd.h"
#include "database/song.h"
  
// MPD classes ----------------------------------------------------------------
class tMPDError : public tRuntimeError
{
  public:
    tMPDError(const QString& err, mpd_Connection* conn=0)
      : tRuntimeError(err), Code(0), At(MPD_ERROR_AT_UNK)
      {
        if (conn && conn->error)
        {
          Error = (char*)conn->errorStr;
          Code = conn->errorCode;
          At = conn->errorAt;
        }
      }
    virtual ~tMPDError() throw() {}
    
    virtual const char* what() const throw()
      { return (std::string(tRuntimeError::what())
                + " MPD: " + Error).c_str(); }

    int Code;
    int At;
    std::string Error;
};




class tMPDNoConnectionError : public tMPDError
{
  public:
    tMPDNoConnectionError()
      : tMPDError(qApp->translate("tMPDPlayer", "No connection to MPD.")) {}
};

class tMPDStatus
{
    mpd_Status* status;
    
  public:
    tMPDStatus(mpd_Connection* conn) : status(0)
      {
        if (!conn) throw tMPDNoConnectionError();
          
        mpd_sendStatusCommand(conn);
        status = mpd_getStatus(conn);
        mpd_finishCommand(conn);
        if (conn->error)
          throw tMPDError(qApp->translate("tMPDPlayer",
                                          "Unable to get status from MPD."),
                          conn);
      }
    ~tMPDStatus()
      {
        if (status) mpd_freeStatus(status);
        status = 0;
      }

    int volume() const { return status->volume; }
    int repeat() const { return status->repeat; }
    int random() const { return status->random; }
    int playlistLength() const { return status->playlistLength; }
    long long playlist() const { return status->playlist; }
    int state() const { return status->state; }
    int crossfade() const { return status->crossfade; }
    int song() const { return status->song; }
    int songid() const { return status->songid; }
    int elapsedTime() const { return status->elapsedTime; }
    int totalTime() const { return status->totalTime; }
    int bitRate() const { return status->bitRate; }
    unsigned int sampleRate() const { return status->sampleRate; }
    int bits() const { return status->bits; }
    int channels() const { return status->channels; }
    int updatingDb() const { return status->updatingDb; }
    char * error() const { return status->error; }
};



// tMPDPlayer ----------------------------------------------------------------
tMPDPlayer::tMPDPlayer() : Conn(0)
{
  char* env;

  env = getenv("MPD_HOST");
  if (env)
    Host = env;
  else
    Host = "localhost";

  env = getenv("MPD_PORT");
  if (env)
  {
    QString s = env;
    QTextIStream stream(&s);
    stream >> Port;
  }
  else
  {
    Port = 6600;
  }

  spyMusicPath();

  try
  {
    ensureConnection();
    resetState();
  }
  catch (tMPDError const& e)
  {
    // Failed ... but initialize still, we can connect later.
  }
}




tFilename tMPDPlayer::convertFileFromMPD(tFilename const& file)
{
  return MusicPath + file;
}




tFilename tMPDPlayer::convertFileToMPD(tFilename const& file)
{
  if (file.find(MusicPath) == 0)
  {
    tFilename newfile(file);
    newfile.erase(0, MusicPath.length());
    return newfile;
  }
  else
  {
    return file;
  }
}




void tMPDPlayer::ensureConnection()
{
  // check if the connection is alive
  if (Conn)
  {
    try
    {
      tMPDStatus status(Conn);
      return;
    }
    catch (tMPDError const& e)
    {
      closeConnection();
      // not ok, try to reconnect
    }
  }

  // reconnect
  try
  {
    Conn = mpd_newConnection(Host, Port, 10);
    if (Conn->error) throw tMPDError(tr("Error connecting to MPD."), Conn);
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }

  spyMusicPath();
}




void tMPDPlayer::closeConnection()
{
  if (Conn)
  {
    mpd_closeConnection(Conn);
    Conn = 0;
  }
}




void tMPDPlayer::finishCommand()
{
  if (!Conn) throw tMPDNoConnectionError();
  mpd_finishCommand(Conn);
  if (Conn->error)
  {
    tMPDError err(tr("Command failed."), Conn);
    closeConnection();
    throw err;
  }
}




void tMPDPlayer::playNow(tSongList const &songlist)
{
  try
  {
    ensureConnection();
    if (songlist.size() == 0)
      return;
    
    int start_playlist_pos = enqueue(songlist);
    
    mpd_sendPlayCommand(Conn, start_playlist_pos);
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::playNext(tSongList const &songlist)
{
  try
  {
    ensureConnection();
    
    if (songlist.size() == 0)
      return;
    
    enqueue(songlist);
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::playEventually(tSongList const &songlist)
{
  try
  {
    ensureConnection();
    
    if (songlist.size() == 0)
      return;
    
    mpd_sendCommandListBegin(Conn);
    
    FOREACH_CONST(first, songlist, tSongList)
    {
      tFilename file( convertFileToMPD((*first)->filename()) );
      mpd_sendAddCommand(Conn, const_cast<char *>(file.c_str()));
    }

    mpd_sendCommandListEnd(Conn);
    
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::setPlayList(tSongList const &songlist)
{
  try
  {
    ensureConnection();
    
    mpd_sendClearCommand(Conn);
    finishCommand();
    
    mpd_sendCommandListBegin(Conn);
    
    FOREACH_CONST(first, songlist, tSongList)
    {
      tFilename file( convertFileToMPD((*first)->filename()) );
      mpd_sendAddCommand(Conn, const_cast<char *>(file.c_str()));
    }
    
    mpd_sendCommandListEnd(Conn);
    
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::getPlayList(vector<tFilename> &songs)
{
  try
  {
    ensureConnection();

    songs.clear();
    
    mpd_sendPlaylistInfoCommand(Conn, -1);
    if (Conn->error) return;
    
    mpd_InfoEntity* entity;
    while ((entity = mpd_getNextInfoEntity(Conn)))
    {
      if (entity->type==MPD_INFO_ENTITY_TYPE_SONG)
      {
        mpd_Song * song = entity->info.song;
        songs.push_back(convertFileFromMPD(song->file));
      }
      mpd_freeInfoEntity(entity);
    }
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




int tMPDPlayer::getPlayListIndex()
{
  try
  {
    tMPDStatus status(Conn);

    if (status.song() >= 0 && status.song() < status.playlistLength())
      return status.song();
    else
      return -1;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




unsigned tMPDPlayer::getPlayListLength()
{
  try
  {
    tMPDStatus status(Conn);
    return status.playlistLength();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




tFilename tMPDPlayer::currentFilename()
{
  try
  {
    tMPDStatus status(Conn);
    
    if (status.state() == MPD_STATUS_STATE_PLAY
        || status.state() == MPD_STATUS_STATE_PAUSE)
      mpd_sendCurrentSongCommand(Conn);
    else if (status.song() >= 0 && status.song() < status.playlistLength())
      mpd_sendPlaylistInfoCommand(Conn, status.song());
    else
      return "";

    tFilename file;
  
    mpd_InfoEntity* entity;
    while ((entity = mpd_getNextInfoEntity(Conn)))
    {
      mpd_Song * song = entity->info.song;
      if (entity->type == MPD_INFO_ENTITY_TYPE_SONG)
        file = convertFileFromMPD(song->file);
      mpd_freeInfoEntity(entity);
    }

    finishCommand();
    
    return file;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




QString tMPDPlayer::currentTitle()
{
  try
  {
    tMPDStatus status(Conn);

    if (status.state() == MPD_STATUS_STATE_PLAY
        || status.state() == MPD_STATUS_STATE_PAUSE)
      mpd_sendCurrentSongCommand(Conn);
    else if (status.song() >= 0 && status.song() < status.playlistLength())
      mpd_sendPlaylistInfoCommand(Conn, status.song());
    else
      return "";

    QString title;
  
    mpd_InfoEntity* entity;
    while ((entity = mpd_getNextInfoEntity(Conn)))
    {
      mpd_Song * song = entity->info.song;
      if (entity->type == MPD_INFO_ENTITY_TYPE_SONG)
        title = song->title;
      mpd_freeInfoEntity(entity);
    }

    finishCommand();
    
    return title;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




float tMPDPlayer::currentTime()
{
  try
  {
    tMPDStatus status(Conn);

    if (status.state() == MPD_STATUS_STATE_PLAY ||
        status.state() == MPD_STATUS_STATE_PAUSE)
      return status.elapsedTime();
    else
      return 0;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




float tMPDPlayer::totalTime()
{
  try
  {
    tMPDStatus status(Conn);

    if (status.state() == MPD_STATUS_STATE_PLAY ||
        status.state() == MPD_STATUS_STATE_PAUSE)
      return status.totalTime();
    else if (status.song() >= 0 && status.song() < status.playlistLength())
    {
      mpd_sendPlaylistInfoCommand(Conn, status.song());

      int time;
  
      mpd_InfoEntity* entity;
      while ((entity = mpd_getNextInfoEntity(Conn)))
      {
        mpd_Song * song = entity->info.song;
        if (entity->type == MPD_INFO_ENTITY_TYPE_SONG)
          time = song->time;
        mpd_freeInfoEntity(entity);
      }

      finishCommand();

      if (time != MPD_SONG_NO_TIME)
        return time;
      else
        return 0;
    }
    else
      return 0;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::play()
{
  try
  {
    ensureConnection();

    tMPDStatus status(Conn);
    
    if (status.state() == MPD_STATUS_STATE_PAUSE)
    {
      mpd_sendPauseCommand(Conn, 0);
      finishCommand();
    }
    else
    {
      if (status.song() >= 0)
        mpd_sendPlayCommand(Conn, status.song());
      else
        mpd_sendPlayCommand(Conn, MPD_PLAY_AT_BEGINNING);
      finishCommand();
    }
    timer();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::stop()
{
  try
  {
    ensureConnection();

    mpd_sendStopCommand(Conn);
    finishCommand();
    timer();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::pause()
{
  try
  {
    ensureConnection();
    mpd_sendPauseCommand(Conn, 1);
    finishCommand();
    timer();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::skipForward()
{
  try
  {
    ensureConnection();

    tMPDStatus status(Conn);

    if (status.state() != MPD_STATUS_STATE_STOP)
    {
      mpd_sendNextCommand(Conn);
      finishCommand();
    }
    else if (status.song() < status.playlistLength())
    {
      // FIXME: MPD does not want to skip when stopped, hence this hack
      mpd_sendCommandListBegin(Conn);
      mpd_sendSeekCommand(Conn, status.song() + 1, 0);
      mpd_sendStopCommand(Conn);
      mpd_sendCommandListEnd(Conn);
      finishCommand();
    }
    timer();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::skipBack()
{
  try
  {
    ensureConnection();

    tMPDStatus status(Conn);
    if (status.state() != MPD_STATUS_STATE_STOP)
    {
      mpd_sendPrevCommand(Conn);
      finishCommand();
    }
    else if (status.song() > 0)
    {
      // FIXME: MPD does not want to skip when stopped, hence this hack
      mpd_sendCommandListBegin(Conn);
      mpd_sendSeekCommand(Conn, status.song() - 1, 0);
      mpd_sendStopCommand(Conn);
      mpd_sendCommandListEnd(Conn);
      finishCommand();
    }
    timer();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




void tMPDPlayer::skipToSeconds(float seconds)
{
  try
  {
    if (!Conn) return;
    int pos = getPlayListIndex();
    if (pos >= 0)
    {
      mpd_sendSeekCommand(Conn, pos, int(seconds));
      finishCommand();
    }
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}


bool tMPDPlayer::isPlaying()
{
  try
  {
    tMPDStatus status(Conn);
    return status.state() == MPD_STATUS_STATE_PLAY;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}

bool tMPDPlayer::isPaused()
{
  try
  {
    tMPDStatus status(Conn);
    return status.state() == MPD_STATUS_STATE_PAUSE;
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}

void tMPDPlayer::setPlayListIndex(int index)
{
  try
  {
    ensureConnection();
    mpd_sendSeekCommand(Conn, index, 0);
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}

void tMPDPlayer::removePlayListIndex(int index)
{
  try
  {
    ensureConnection();
    mpd_sendDeleteCommand(Conn, index);
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}

void tMPDPlayer::clearPlaylist()
{
  try
  {
    ensureConnection();
    mpd_sendClearCommand(Conn);
    finishCommand();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}

bool tMPDPlayer::canGetValidStatus()
{
  try
  {
    ensureConnection();
    return true;
  }
  catch (tRuntimeError& err)
  {
    closeConnection();
    return false;
  }
}

void tMPDPlayer::ensureValidStatus()
{
  try
  {
    ensureConnection();
  }
  catch (tRuntimeError& err)
  {
    closeConnection();
    throw;
  }
}
    



void tMPDPlayer::timer()
{
  try
  {
    tPollingPlayer::timer();
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    // No, we do not want to throw exceptions to Qt, 
    // since it will die.
  }
}



bool tMPDPlayer::haveValidPlaylistPosition()
{
  return isValidPlaylistPosition(getPlayListIndex());
}




bool tMPDPlayer::isValidPlaylistPosition(int pos)
{
  try
  {
    tMPDStatus status(Conn);
    return (status.song() >= 0 && status.song() < status.playlistLength());
  }
  catch (tMPDError const& e)
  {
    closeConnection();
    throw;
  }
}




int tMPDPlayer::enqueue(const tSongList &songlist)
{
  tMPDStatus status(Conn);
  
  int start_pos = status.song();
  if (start_pos < 0 || start_pos >= status.playlistLength())
    start_pos = 0;
  
  if (status.playlistLength() > 0)
    ++start_pos;
  
  /* first add */
  
  mpd_sendCommandListBegin(Conn);
  FOREACH_CONST(first, songlist, tSongList)
  {
    tFilename file( convertFileToMPD((*first)->filename()) );
    mpd_sendAddCommand(Conn, const_cast<char *>(file.c_str()));
  }
  mpd_sendCommandListEnd(Conn);
  finishCommand();
  
  /* then move */
  
  tMPDStatus status2(Conn);
  
  int nadded = status2.playlistLength() - status.playlistLength();
  
  mpd_sendCommandListBegin(Conn);
  for (int i = 0; i < nadded; ++i)
  {
    mpd_sendMoveCommand(Conn,
                        status2.playlistLength() - 1,
                        start_pos);
  }
  mpd_sendCommandListEnd(Conn);
  finishCommand();
  
  return start_pos;
}



// Spy MPD music path: assume it is the same as madman's first path ----------
#include "database/program_base.h"

void tMPDPlayer::spyMusicPath()
{
  tDatabase& db = tProgramBase::database();
  if (db.DirectoryList.size() >= 1)
  {
    MusicPath = db.DirectoryList[0];
    // Add slash
    if (MusicPath.length() > 0 && MusicPath[MusicPath.length()-1] != '/')
      MusicPath += '/';
  }
  else
    MusicPath = "";  // FIXME: Ugh!
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

