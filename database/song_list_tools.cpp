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




#include <qfile.h>
#include <qregexp.h>
#include <qapplication.h>
#include "database/song_list_tools.h"
#include "utility/prefs.h"




// comparison helpers ---------------------------------------------------------
class tMultilevelLess : public tLessBase
{
    refcnt_ptr<tLessBase> Primary, Secondary, Tertiary;

  public:
    tMultilevelLess(auto_ptr<tLessBase> primary, 
                    auto_ptr<tLessBase> secondary,
                    auto_ptr<tLessBase> tertiary)
    : Primary(primary), Secondary(secondary), Tertiary(tertiary)
    { }

    bool operator()(const tSong *song1, const tSong *song2)
    {
      if ((*Primary)(song1, song2))
        return true;
      else if ((*Primary)(song2, song1))
        return false;

      if (Secondary.get() == NULL)
        return false;

      if ((*Secondary)(song1, song2))
        return true;
      else if ((*Secondary)(song2, song1))
        return false;

      if (Tertiary.get() == NULL)
        return false;

      if ((*Tertiary)(song1, song2))
        return true;
      else if ((*Tertiary)(song2, song1))
        return false;

      return false;
    }
};




#define MAKE_STRING_SONG_LESS(CLASSNAME, FIELD) \
struct CLASSNAME : public tLessBase \
{ \
  bool operator()(const tSong *song1, const tSong *song2) \
  { \
    return song1->FIELD().lower().localeAwareCompare(song2->FIELD().lower()) < 0; \
  } \
};

#define MAKE_CXX_STRING_SONG_LESS(CLASSNAME, FIELD) \
struct CLASSNAME : public tLessBase \
{ \
  bool operator()(const tSong *song1, const tSong *song2) \
  { \
    return QString::fromUtf8(song1->FIELD().c_str()).lower().localeAwareCompare( \
        QString::fromUtf8(song2->FIELD().c_str()).lower()) < 0; \
  } \
};

#define MAKE_NUMERIC_SONG_LESS(CLASSNAME, FIELD) \
struct CLASSNAME : public tLessBase \
{ \
  bool operator()(const tSong *song1, const tSong *song2) \
  { \
    return song1->FIELD() < song2->FIELD(); \
  } \
};

namespace 
{
  MAKE_STRING_SONG_LESS(tArtistLess, artist);
  MAKE_STRING_SONG_LESS(tPerformerLess, performer);
  MAKE_STRING_SONG_LESS(tTitleLess, title);
  MAKE_STRING_SONG_LESS(tAlbumLess, album);
  MAKE_STRING_SONG_LESS(tGenreLess, genre);
  MAKE_STRING_SONG_LESS(tMoodLess, mood);
  MAKE_STRING_SONG_LESS(tTempoLess, tempo);
  MAKE_STRING_SONG_LESS(tCustom1Less, custom1);
  MAKE_STRING_SONG_LESS(tCustom2Less, custom2);
  MAKE_STRING_SONG_LESS(tCustom3Less, custom3);
  MAKE_STRING_SONG_LESS(tTitleOrFilenameLess, titleOrFilename);
  MAKE_CXX_STRING_SONG_LESS(tFilenameLess, filenameOnly);
  MAKE_CXX_STRING_SONG_LESS(tPathLess, pathname);
  MAKE_STRING_SONG_LESS(tMimeTypeLess, mimeType);
  MAKE_NUMERIC_SONG_LESS(tDurationLess, duration);
  MAKE_NUMERIC_SONG_LESS(tFullPlayCountLess, fullPlayCount);
  MAKE_NUMERIC_SONG_LESS(tPartialPlayCountLess, partialPlayCount);
  MAKE_NUMERIC_SONG_LESS(tPlayCountLess, playCount);
  MAKE_NUMERIC_SONG_LESS(tLastPlayedLess, lastPlayed);
  MAKE_NUMERIC_SONG_LESS(tRatingLess, rating);
  MAKE_NUMERIC_SONG_LESS(tYearLess, year().toInt); // HACKY-WHACKY :)
  MAKE_NUMERIC_SONG_LESS(tSizeLess, fileSize);
  MAKE_NUMERIC_SONG_LESS(tExistsSinceLess, existsSince);
  MAKE_NUMERIC_SONG_LESS(tLastModifiedLess, lastModified);
  MAKE_NUMERIC_SONG_LESS(tUniqueIdLess, uniqueId);

  struct tTrackNumberLess : public tLessBase
  {
    bool operator()(const tSong *song1, const tSong *song2)
    {
      QRegExp extract_numbers("^\\s*([0-9]+)");

      if (extract_numbers.search(song1->trackNumber()) != -1)
      {
	int my_track = extract_numbers.cap(1).toInt();
	if (extract_numbers.search(song2->trackNumber()) != -1)
	{
	  // both have track numbers, compare them
	  int other_track = extract_numbers.cap(1).toInt();
	  return my_track < other_track;
	}
	else
	{
	  // other is smaller since it has no track number
	  return false;
	}
      }
      else
      {
	if (extract_numbers.search(song2->trackNumber()) != -1)
	  return true; // "this" is smaller since it has no track number
	else
	  return false; // neither has a track number
      }
    }
  };
}




static void sortSingle(tSongList &list, tSongField field)
{
  switch (field)
  {
    // FIXME switch back to sort when bug in gcc is gone
    case FIELD_ARTIST : stable_sort(list.begin(), list.end(), tArtistLess()); break;
    case FIELD_PERFORMER : stable_sort(list.begin(), list.end(), tPerformerLess()); break;
    case FIELD_TITLE : stable_sort(list.begin(), list.end(), tTitleLess()); break;
    case FIELD_ALBUM : stable_sort(list.begin(), list.end(), tAlbumLess()); break;
    case FIELD_TRACKNUMBER : stable_sort(list.begin(), list.end(), tTrackNumberLess()); break;
    case FIELD_DURATION : stable_sort(list.begin(), list.end(), tDurationLess()); break;
    case FIELD_GENRE : stable_sort(list.begin(), list.end(), tGenreLess()); break;
    case FIELD_FULLPLAYCOUNT : stable_sort(list.begin(), list.end(), tFullPlayCountLess()); break;
    case FIELD_PARTIALPLAYCOUNT : stable_sort(list.begin(), list.end(), tPartialPlayCountLess()); break;
    case FIELD_PLAYCOUNT : stable_sort(list.begin(), list.end(), tPlayCountLess()); break;
    case FIELD_LASTPLAYED : stable_sort(list.begin(), list.end(), tLastPlayedLess()); break;
    case FIELD_RATING : stable_sort(list.begin(), list.end(), tRatingLess()); break;
    case FIELD_YEAR : stable_sort(list.begin(), list.end(), tYearLess()); break;
    case FIELD_FILE : stable_sort(list.begin(), list.end(), tFilenameLess()); break;
    case FIELD_PATH : stable_sort(list.begin(), list.end(), tPathLess()); break;
    case FIELD_SIZE : stable_sort(list.begin(), list.end(), tSizeLess()); break;
    case FIELD_EXISTSSINCE : stable_sort(list.begin(), list.end(), tExistsSinceLess()); break;
    case FIELD_LASTMODIFIED : stable_sort(list.begin(), list.end(), tLastModifiedLess()); break;
    case FIELD_MIMETYPE: stable_sort(list.begin(), list.end(), tMimeTypeLess()); break;
    case FIELD_UNIQUEID: stable_sort(list.begin(), list.end(), tUniqueIdLess()); break;
    case FIELD_MOOD : stable_sort(list.begin(), list.end(), tMoodLess()); break;
    case FIELD_TEMPO : stable_sort(list.begin(), list.end(), tTempoLess()); break;
    case FIELD_CUSTOM1 : stable_sort(list.begin(), list.end(), tCustom1Less()); break;
    case FIELD_CUSTOM2 : stable_sort(list.begin(), list.end(), tCustom2Less()); break;
    case FIELD_CUSTOM3 : stable_sort(list.begin(), list.end(), tCustom3Less()); break;
    case FIELD_TITLE_OR_FILENAME : stable_sort(list.begin(), list.end(), tTitleOrFilenameLess()); break;
    default:
      throw tRuntimeError(
        qApp->translate("ErrorMessages",
                        "Invalid field id %1 while sorting list of songs").arg((int) field));
  }
}




static tLessBase *getBasicLess(tSongField field)
{
  switch (field)
  {
    case FIELD_ARTIST : return new tArtistLess;
    case FIELD_PERFORMER : return new tPerformerLess; break;
    case FIELD_TITLE : return new tTitleLess;
    case FIELD_ALBUM : return new tAlbumLess;
    case FIELD_TRACKNUMBER : return new tTrackNumberLess;
    case FIELD_DURATION : return new tDurationLess;
    case FIELD_GENRE : return new tGenreLess;
    case FIELD_FULLPLAYCOUNT : return new tFullPlayCountLess;
    case FIELD_PARTIALPLAYCOUNT : return new tPartialPlayCountLess;
    case FIELD_PLAYCOUNT : return new tPlayCountLess;
    case FIELD_LASTPLAYED : return new tLastPlayedLess;
    case FIELD_RATING : return new tRatingLess;
    case FIELD_YEAR : return new tYearLess;
    case FIELD_FILE : return new tFilenameLess;
    case FIELD_PATH : return new tPathLess;
    case FIELD_SIZE : return new tSizeLess;
    case FIELD_EXISTSSINCE : return new tExistsSinceLess;
    case FIELD_LASTMODIFIED : return new tLastModifiedLess;
    case FIELD_MIMETYPE: return new tMimeTypeLess;
    case FIELD_UNIQUEID: return new tUniqueIdLess;
    case FIELD_MOOD : return new tMoodLess;
    case FIELD_TEMPO : return new tTempoLess;
    case FIELD_CUSTOM1 : return new tCustom1Less;
    case FIELD_CUSTOM2 : return new tCustom2Less;
    case FIELD_CUSTOM3 : return new tCustom3Less;
    case FIELD_TITLE_OR_FILENAME : return new tTitleOrFilenameLess;
    case FIELD_INVALID: return NULL;
    default:
      throw tRuntimeError(
        qApp->translate("ErrorMessages",
                        "Invalid field id %1 while sorting list of songs").arg((int) field));
  }
}




tLessContainer getLess(tSongField field, tSongField secondary, tSongField tertiary)
{
  if (secondary == FIELD_INVALID && tertiary == FIELD_INVALID)
    return tLessContainer(getBasicLess(field));
  else
  {
    auto_ptr<tLessBase> 
      primary_less(getBasicLess(field)),
      secondary_less(getBasicLess(secondary)),
      tertiary_less(getBasicLess(tertiary));

    return tLessContainer(new tMultilevelLess(primary_less, 
                                              secondary_less,
                                              tertiary_less));
  }
}




void sort(tSongList &list, tSongField field, tSongField secondary, tSongField tertiary)
{
  if (secondary == FIELD_INVALID && tertiary == FIELD_INVALID)
    sortSingle(list, field);
  else
  {
    auto_ptr<tLessBase> 
      primary_less(getBasicLess(field)), \
      secondary_less(getBasicLess(secondary)), \
      tertiary_less(getBasicLess(tertiary));

    tMultilevelLess my_less(
      primary_less, 
      secondary_less,
      tertiary_less);

    stable_sort(list.begin(), list.end(), my_less);
  }
}




void sort(tSongList &list, tSongField field, tPreferences &prefs)
{
  sort(list, field, 
      prefs.SortingPreferences.SecondarySortField[field],
      prefs.SortingPreferences.TertiarySortField[field]);
}




// song list summaries --------------------------------------------------------
tSongListSummary getSongListSummary(tSongList &list)
{
  tSongListSummary result;
  result.DurationTotal = 0;
  result.SizeTotal = 0;
  result.Count = list.size();

  FOREACH_CONST(first, list, tSongList)
  {
    result.DurationTotal += (*first)->duration();
    result.SizeTotal += (*first)->fileSize() / (1024. * 1024);
  }

  double duration = result.DurationTotal;
  result.DurationDays = int(duration / 3600. / 24.);
  duration -= result.DurationDays * 3600 * 24;
  result.DurationHours = int(duration / 3600);
  duration -= result.DurationHours * 3600;
  result.DurationMinutes = int(duration / 60);
  duration -= result.DurationMinutes * 60;
  result.DurationSeconds = int(duration);

  if (list.size())
    result.FirstFilename = QFile::decodeName(list[0]->filename().c_str());
  return result;
}




QString	stringifySongListSummary(const tSongListSummary &sum)
{
  QString duration_string;
  if (sum.DurationDays)
    duration_string.sprintf("%d days %d:%02d:%02d", 
	sum.DurationDays, sum.DurationHours, sum.DurationMinutes, 
	sum.DurationSeconds);
  else if (sum.DurationHours)
    duration_string.sprintf("%d:%02d:%02d", 
	sum.DurationHours, sum.DurationMinutes, sum.DurationSeconds);
  else
    duration_string.sprintf("%2d:%02d", 
	sum.DurationMinutes, sum.DurationSeconds);

  QString result;
  if (sum.Count == 0)
  {
    result = qApp->translate("StatusMessages", "0 songs");
  }
  else if (sum.Count == 1)
  {
   result = qApp->translate("StatusMessages", "%1 - Duration %2, Size %3")
      .arg(sum.FirstFilename)
      .arg(duration_string)
      .arg(qApp->translate("StatusMessages", "%1 MB").arg(sum.SizeTotal, 0, 'f', 2));
  }
  else
  {
    result = qApp->translate("StatusMessages", "%1 songs - Duration %2 - Size %3")
      .arg(sum.Count)
      .arg(duration_string)
      .arg(qApp->translate("StatusMessages", "%1 MB").arg(sum.SizeTotal, 0, 'f', 2));
  }
  return result;
}




QString	stringifySongListSummary(tSongList &list)
{
  return stringifySongListSummary(getSongListSummary(list));




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
}
