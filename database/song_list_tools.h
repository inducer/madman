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




#ifndef HEADER_SEEN_DATABASE_SONG_LIST_TOOLS_H
#define HEADER_SEEN_DATABASE_SONG_LIST_TOOLS_H




#include "utility/refcnt_ptr.h"
#include "database/song.h"




struct tLessBase 
{
  public:
    virtual ~tLessBase() {}
    virtual bool operator()(const tSong *song1, const tSong *song2) = 0;
};

class tLessContainer
{
    refcnt_ptr<tLessBase> Contents;
  public:
    tLessContainer(tLessBase *lb)
      : Contents(lb)
      { }
    bool operator()(const tSong *song1, const tSong *song2)
      {
        return (*Contents)(song1, song2);
      }
};

tLessContainer getLess(tSongField field, tSongField secondary = FIELD_INVALID, tSongField tertiary = FIELD_INVALID);
void sort(tSongList &list, tSongField field, tSongField secondary = FIELD_INVALID, tSongField tertiary = FIELD_INVALID);

class tPreferences;

void sort(tSongList &list, tSongField field, tPreferences &prefs);

struct tSongListSummary
{
  double	DurationTotal;
  unsigned	DurationDays;
  unsigned	DurationHours;
  unsigned	DurationMinutes;
  unsigned	DurationSeconds;

  /** Total size in megabytes. */
  double	SizeTotal;

  unsigned	Count;

  QString	FirstFilename;
};

tSongListSummary getSongListSummary(tSongList &list);
QString	stringifySongListSummary(const tSongListSummary &sum);
QString	stringifySongListSummary(tSongList &list);



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
