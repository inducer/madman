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




#include "database/program_base.h"



template<>
// tProgramBase ------------------------------------------------------------
tProgramBase *tSingleton<tProgramBase>::instance_m = NULL;




tProgramBase::tProgramBase()
  : AutoDJ(tProgramBase::preferences().AutoDJPreferences, &AllSongSet)
{
  setDatabase(new tDatabase());

  connect(&preferences().Player, SIGNAL(currentSongChanged()), 
      this, SLOT(noticeSongChanged()));
  connect(&preferences().Player, SIGNAL(stateChanged()), 
      this, SLOT(noticeStateChanged()));

  WasPlaying = false;
  AccumulatedPlayTime = 0;

  AllSongSet.setSongCollection(&tProgramBase::database().SongCollection);
  AllSongSet.setCriterion("~all");
  AllSongSet.reevaluateCriterion();
}




tProgramBase::~tProgramBase()
{
}




void tProgramBase::setDatabase(tDatabase *db)
{
  auto_ptr<tDatabase> new_db(db);
  Database = new_db;
  AllSongSet.setSongCollection(&Database->SongCollection);

  emit songChanged();
}




tSong *tProgramBase::currentSong()
{
  if (instance())
    return database().SongCollection.getByFilename(instance()->CurrentSongFilename);
  else
    return NULL;
}




void tProgramBase::noticeStateChanged()
{
  time_t now = time(NULL);
  if (WasPlaying)
  {
    AccumulatedPlayTime += now - PlayStartTime;
    PlayStartTime = now;
  }

  WasPlaying = preferences().Player.isPlaying() && !preferences().Player.isPaused();
}




void tProgramBase::noticeSongChanged()
{
  // bring accumulated play time up to date
  noticeStateChanged();

  if (preferences().CollectHistory)
  {
    tSong *prev_song = database().SongCollection.getByFilename(CurrentSongFilename);
    if (prev_song)
    {
      prev_song->played(time(NULL), AccumulatedPlayTime > 0.6 * prev_song->duration());
      database().History.played(prev_song->uniqueId(), 
				time(NULL), AccumulatedPlayTime);
    }
  }

  CurrentSongFilename = preferences().Player.currentFilename();
  tFilename::size_type idx;
  while ((idx = CurrentSongFilename.find("//")) != tFilename::npos)
  {
    CurrentSongFilename.replace(idx, 2, "/");
  }

  emit songChanged();

  PlayStartTime = time(NULL);
  AccumulatedPlayTime = 0;
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
