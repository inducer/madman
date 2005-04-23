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




#include <algorithm>
#include <memory>
#include <iomanip>
#include <stdexcept>
#include <libgen.h>
#include <qregexp.h>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <glib.h>
#include <sys/stat.h>
#include "database/database.h"
#include "database/program_base.h"
#include "utility/prefs.h"
#include "song.h"

#include <tag.h>
#include <mpegfile.h>
#include <flacfile.h>
#include <vorbisfile.h>

#include <id3v1tag.h>
#include <id3v2tag.h>
#include <xiphcomment.h>
#include <id3v2frame.h>

#include <textidentificationframe.h>

#include <qapplication.h>

#ifdef WITH_M4A 
#define USE_TAGGING
#include <mp4ff.h>
#include <faad.h>
#include <fcntl.h>
#endif


// private helpers ------------------------------------------------------------
namespace 
{
  string decodeFilename(const QString &str)
  {
    if (str.left(7) == "base64:")
      return decodeBase64(QString2string(str.mid(7)));
    else
      return QString2string(str);
  }

  template<class T, class T2>
  void assignAndCheckForModification(const tSong *song, tSongCollection *collection, T &value, const T2 &rvalue, tSongField field)
  { bool changed = value != rvalue;
    value = rvalue;
    if (changed && collection)
      collection->noticeSongModified(song, field);
  }




  QString convertTagLibString(const TagLib::String &ts)
  {
    if (ts.isNull())
      return QString::null;

    return 
      QString::fromUtf8(
	sanitizeUtf8(
	  ts.to8Bit(true)).c_str()).stripWhiteSpace();
  }



  void setFieldString(TagLib::Tag *tag, tSongField field, const QString &value)
  {
    using namespace TagLib;

    String conv_value(value.utf8(), String::UTF8);
    
    switch (field)
    {
    case FIELD_ARTIST:
      tag->setArtist(conv_value);
      break;
    case FIELD_TITLE:
      tag->setTitle(conv_value);
      break;
    case FIELD_ALBUM:
      tag->setAlbum(conv_value);
      break;
    case FIELD_TRACKNUMBER:
      tag->setTrack(value.toUInt());
      break;
    case FIELD_GENRE:
      tag->setGenre(conv_value);
      break;
    case FIELD_YEAR:
      tag->setYear(value.toUInt());
      break;
    default:
      throw runtime_error("unhandled field in setFieldString");
    }
  }




  QString getFieldString(tSongField field, TagLib::Tag *tag)
  {
    if (tag == NULL)
      return QString::null;

    switch (field)
    {
    case FIELD_ARTIST: return convertTagLibString(tag->artist());
    case FIELD_TITLE: return convertTagLibString(tag->title());
    case FIELD_ALBUM: return convertTagLibString(tag->album());
    case FIELD_TRACKNUMBER: 
      if (tag->track() == 0)
	return "";
      else
	return QString::number(tag->track());
    case FIELD_GENRE: return convertTagLibString(tag->genre());
    case FIELD_YEAR: 
      if (tag->year() == 0)
	return "";
      else
	return QString::number(tag->year());
    default:
      throw runtime_error("unhandled field in getFieldString");
    }
  }




  QString getFieldString(tSongField field, TagLib::Tag *tag1, TagLib::Tag *tag2)
  {
    QString t1_value, t2_value;
    if (tag1)
      t1_value = getFieldString(field, tag1);
    if (tag2)
      t2_value = getFieldString(field, tag2);
    
    if (t1_value.isNull())
      return t2_value;
    else
      return t1_value;
  }
}




// public helpers -------------------------------------------------------------
QString genreIdToString(int genre)
{
  switch (genre)
  {
    case 0:
      return "Blues";
    case 1:
      return "Classic Rock";
    case 2:
      return "Country";
    case 3:
      return "Dance";
    case 4:
      return "Disco";
    case 5:
      return "Funk";
    case 6:
      return "Grunge";
    case 7:
      return "Hip-Hop";
    case 8:
      return "Jazz";
    case 9:
      return "Metal";
    case 10:
      return "New Age";
    case 11:
      return "Oldies";
    case 12:
      return "Other";
    case 13:
      return "Pop";
    case 14:
      return "R&B";
    case 15:
      return "Rap";
    case 16:
      return "Reggae";
    case 17:
      return "Rock";
    case 18:
      return "Techno";
    case 19:
      return "Industrial";
    case 20:
      return "Alternative";
    case 21:
      return "Ska";
    case 22:
      return "Death Metal";
    case 23:
      return "Pranks";
    case 24:
      return "Soundtrack";
    case 25:
      return "Euro-Techno";
    case 26:
      return "Ambient";
    case 27:
      return "Trip-Hop";
    case 28:
      return "Vocal";
    case 29:
      return "Jazz+Funk";
    case 30:
      return "Fusion";
    case 31:
      return "Trance";
    case 32:
      return "Classical";
    case 33:
      return "Instrumental";
    case 34:
      return "Acid";
    case 35:
      return "House";
    case 36:
      return "Game";
    case 37:
      return "Sound Clip";
    case 38:
      return "Gospel";
    case 39:
      return "Noise";
    case 40:
      return "AlternRock";
    case 41:
      return "Bass";
    case 42:
      return "Soul";
    case 43:
      return "Punk";
    case 44:
      return "Space";
    case 45:
      return "Meditative";
    case 46:
      return "Instrumental Pop";
    case 47:
      return "Instrumental Rock";
    case 48:
      return "Ethnic";
    case 49:
      return "Gothic";
    case 50:
      return "Darkwave";
    case 51:
      return "Techno-Industrial";
    case 52:
      return "Electronic";
    case 53:
      return "Pop-Folk";
    case 54:
      return "Eurodance";
    case 55:
      return "Dream";
    case 56:
      return "Southern Rock";
    case 57:
      return "Comedy";
    case 58:
      return "Cult";
    case 59:
      return "Gangsta";
    case 60:
      return "Top 40";
    case 61:
      return "Christian Rap";
    case 62:
      return "Pop/Funk";
    case 63:
      return "Jungle";
    case 64:
      return "Native American";
    case 65:
      return "Cabaret";
    case 66:
      return "New Wave";
    case 67:
      return "Psychadelic";
    case 68:
      return "Rave";
    case 69:
      return "Showtunes";
    case 70:
      return "Trailer";
    case 71:
      return "Lo-Fi";
    case 72:
      return "Tribal";
    case 73:
      return "Acid Punk";
    case 74:
      return "Acid Jazz";
    case 75:
      return "Polka";
    case 76:
      return "Retro";
    case 77:
      return "Musical";
    case 78:
      return "Rock & Roll";
    case 79:
      return "Hard Rock";
    case 80:
      return "Folk";
    case 81:
      return "Folk-Rock";
    case 82:
      return "National Folk";
    case 83:
      return "Swing";
    case 84:
      return "Fast Fusion";
    case 85:
      return "Bebob";
    case 86:
      return "Latin";
    case 87:
      return "Revival";
    case 88:
      return "Celtic";
    case 89:
      return "Bluegrass";
    case 90:
      return "Avantgarde";
    case 91:
      return "Gothic Rock";
    case 92:
      return "Progressive Rock";
    case 93:
      return "Psychedelic Rock";
    case 94:
      return "Symphonic Rock";
    case 95:
      return "Slow Rock";
    case 96:
      return "Big Band";
    case 97:
      return "Chorus";
    case 98:
      return "Easy Listening";
    case 99:
      return "Acoustic";
    case 100:
      return "Humour";
    case 101:
      return "Speech";
    case 102:
      return "Chanson";
    case 103:
      return "Opera";
    case 104:
      return "Chamber Music";
    case 105:
      return "Sonata";
    case 106:
      return "Symphony";
    case 107:
      return "Booty Bass";
    case 108:
      return "Primus";
    case 109:
      return "Porn Groove";
    case 110:
      return "Satire";
    case 111:
      return "Slow Jam";
    case 112:
      return "Club";
    case 113:
      return "Tango";
    case 114:
      return "Samba";
    case 115:
      return "Folklore";
    case 116:
      return "Ballad";
    case 117:
      return "Power Ballad";
    case 118:
      return "Rhythmic Soul";
    case 119:
      return "Freestyle";
    case 120:
      return "Duet";
    case 121:
      return "Punk Rock";
    case 122:
      return "Drum Solo";
    case 123:
      return "A capella";
    case 124:
      return "Euro-House";
    case 125:
      return "Dance Hall";
    default:
      return "undefined";
  }
}




QString getFieldName(tSongField field)
{
  switch (field)
  {
    case FIELD_ARTIST: return qApp->translate("FieldDescriptions", "Artist");
    case FIELD_PERFORMER: return qApp->translate("FieldDescriptions", "Performer");
    case FIELD_TITLE: return qApp->translate("FieldDescriptions", "Title");
    case FIELD_ALBUM: return qApp->translate("FieldDescriptions", "Album");
    case FIELD_TRACKNUMBER: return qApp->translate("FieldDescriptions", "Track Number");
    case FIELD_DURATION: return qApp->translate("FieldDescriptions", "Duration");
    case FIELD_GENRE: return qApp->translate("FieldDescriptions", "Genre");
    case FIELD_FULLPLAYCOUNT: return qApp->translate("FieldDescriptions", "Play Count (full)");
    case FIELD_PARTIALPLAYCOUNT: return qApp->translate("FieldDescriptions", "Play Count (partial)");
    case FIELD_PLAYCOUNT: return qApp->translate("FieldDescriptions", "Play Count");
    case FIELD_LASTPLAYED: return qApp->translate("FieldDescriptions", "Last Played");
    case FIELD_RATING: return qApp->translate("FieldDescriptions", "Rating");
    case FIELD_YEAR: return qApp->translate("FieldDescriptions", "Year");
    case FIELD_FILE: return qApp->translate("FieldDescriptions", "File");
    case FIELD_PATH: return qApp->translate("FieldDescriptions", "Path");
    case FIELD_SIZE: return qApp->translate("FieldDescriptions", "File Size");
    case FIELD_EXISTSSINCE: return qApp->translate("FieldDescriptions", "Exists since");
    case FIELD_LASTMODIFIED: return qApp->translate("FieldDescriptions", "Last modified");
    case FIELD_MIMETYPE: return qApp->translate("FieldDescriptions", "MIME Type");
    case FIELD_UNIQUEID: return qApp->translate("FieldDescriptions", "Unique ID");
    case FIELD_MOOD: return qApp->translate("FieldDescriptions", "Mood");
    case FIELD_TEMPO: return qApp->translate("FieldDescriptions", "Tempo");
    case FIELD_CUSTOM1: return qApp->translate("FieldDescriptions", "Custom1");
    case FIELD_CUSTOM2: return qApp->translate("FieldDescriptions", "Custom2");
    case FIELD_CUSTOM3: return qApp->translate("FieldDescriptions", "Custom3");
    default: return qApp->translate("FieldDescriptions", "<unknown>");
  }
}





QString getFieldIdentifier(tSongField field)
{
  switch (field)
  {
    case FIELD_ARTIST: return "artist";
    case FIELD_PERFORMER: return "performer";
    case FIELD_TITLE: return "title";
    case FIELD_ALBUM: return "album";
    case FIELD_TRACKNUMBER: return "tracknumber";
    case FIELD_DURATION: return "duration";
    case FIELD_GENRE: return "genre";
    case FIELD_FULLPLAYCOUNT: return "fullplaycount";
    case FIELD_PARTIALPLAYCOUNT: return "partialplaycount";
    case FIELD_PLAYCOUNT: return "playcount";
    case FIELD_LASTPLAYED: return "lastplayed";
    case FIELD_RATING: return "rating";
    case FIELD_YEAR: return "year";
    case FIELD_FILE: return "file";
    case FIELD_PATH: return "path";
    case FIELD_SIZE: return "size";
    case FIELD_EXISTSSINCE: return "existssince";
    case FIELD_LASTMODIFIED: return "lastmodified";
    case FIELD_MIMETYPE: return "mimetype";
    case FIELD_UNIQUEID: return "uniqueid";
    case FIELD_MOOD: return "mood";
    case FIELD_TEMPO: return "tempo";
    case FIELD_CUSTOM1: return "custom1";
    case FIELD_CUSTOM2: return "custom2";
    case FIELD_CUSTOM3: return "custom3";
default: 
      throw tRuntimeError("Invalid field id in getFieldIdentifier");
  }

}




tSongField getFieldFromIdentifier(const QString &field)
{
  if (field == "artist") return FIELD_ARTIST;
  else if (field == "performer") return FIELD_PERFORMER;
  else if (field == "title") return FIELD_TITLE;
  else if (field == "album") return FIELD_ALBUM;
  else if (field == "tracknumber") return FIELD_TRACKNUMBER;
  else if (field == "duration") return FIELD_DURATION;
  else if (field == "genre") return FIELD_GENRE;
  else if (field == "fullplaycount") return FIELD_FULLPLAYCOUNT;
  else if (field == "partialplaycount") return FIELD_PARTIALPLAYCOUNT;
  else if (field == "playcount") return FIELD_PLAYCOUNT;
  else if (field == "lastplayed") return FIELD_LASTPLAYED;
  else if (field == "rating") return FIELD_RATING;
  else if (field == "year") return FIELD_YEAR;
  else if (field == "file") return FIELD_FILE;
  else if (field == "path") return FIELD_PATH;
  else if (field == "size") return FIELD_SIZE;
  else if (field == "existssince") return FIELD_EXISTSSINCE;
  else if (field == "lastmodified") return FIELD_LASTMODIFIED;
  else if (field == "mimetype") return FIELD_MIMETYPE;
  else if (field == "uniqueid") return FIELD_UNIQUEID;
  else if (field == "mood") return FIELD_MOOD;
  else if (field == "tempo") return FIELD_TEMPO;
  else if (field == "custom1") return FIELD_CUSTOM1;
  else if (field == "custom2") return FIELD_CUSTOM2;
  else if (field == "custom3") return FIELD_CUSTOM3;
  else 
  {
    throw tRuntimeError("Invalid field id in getFieldFromIdentifier");
  }
}




QString substituteSongFields(QString const &format, tSong *song, bool human_readable, bool shell_quote)
{
  QString result = format;

  for (unsigned i = 0; i < FIELD_COUNT; i++)
  {
    tSongField f = (tSongField) i;
    QString replacement;
    if (human_readable)
      replacement = song->humanReadableFieldText(f);
    else
      replacement = song->fieldText(f);

    if (shell_quote)
      replacement = quoteString(replacement);

    result.replace("%" + getFieldIdentifier(f) + "%", replacement);
  }
  result.replace("%newline%", "\n");

  return result;
}




string substituteSongFieldsUtf8(QString const &format, tSong *song, 
                                bool human_readable, bool shell_quote)
{
  string result = format;

  for (unsigned i = 0; i < FIELD_COUNT; i++)
  {
    tSongField f = (tSongField) i;
    string replacement;

    if (f == FIELD_FILE)
      replacement = song->filenameOnly();
    else if (f == FIELD_PATH)
      replacement = song->pathname();
    else if (human_readable)
      replacement = song->humanReadableFieldText(f).utf8();
    else
      replacement = song->fieldText(f).utf8();

    if (shell_quote)
      replacement = quoteString(replacement);

    replace(result, 
            "%" + QString2string(getFieldIdentifier(f)) + "%", 
            replacement);
  }
  replace(result, "%newline%", "\n");

  return result;
}




// tSong ----------------------------------------------------------------------
tSong::tDirectoryList tSong::DirectoryList;




tSong::tSong()
    : UniqueId(SONG_UID_INVALID),
    SongCollection(NULL),
    Duration(0),
    FileSize(0),
    ExistsSince(0),
    LastPlayed(-1),
    LastModified(time(NULL)),
    FullPlayCount(0),
    PartialPlayCount(0),
    Rating(-1),
    CacheValid(false)
{
  IndexIntoDirectoryList = 0;
  if (DirectoryList.size() == 0)
    DirectoryList.push_back("");
}




tSong::~tSong()
{
}




tFilename tSong::filename() const
{
  return pathname() + "/" + filenameOnly();
}




tFilename tSong::pathname() const
{
  return DirectoryList[ IndexIntoDirectoryList ];
}




tFilename tSong::filenameOnly() const
{
  return FilenameOnly;
}




tUniqueId tSong::uniqueId() const
{
  return UniqueId;
}




QString tSong::album() const
{
  ensureInfoIsThere();
  return Album;
}




QString tSong::artist() const
{
  ensureInfoIsThere();
  return Artist;
}




QString tSong::performer() const
{
  ensureInfoIsThere();
  return Performer;
}




QString tSong::title() const
{
  ensureInfoIsThere();
  return Title;
}




QString tSong::year() const
{
  ensureInfoIsThere();
  return Year;
}




QString tSong::genre() const
{
  ensureInfoIsThere();
  return Genre;
}




QString tSong::trackNumber() const
{
  ensureInfoIsThere();
  return TrackNumber;
}




float tSong::duration() const
{
  ensureInfoIsThere();
  return Duration;
}




time_t tSong::existsSince() const
{
  return ExistsSince;
}




time_t tSong::lastModified() const
{
  ensureInfoIsThere();
  return LastModified;
}




time_t tSong::lastPlayed() const
{
  return LastPlayed;
}




int tSong::playCount() const
{
  return FullPlayCount+PartialPlayCount;
}




int tSong::fullPlayCount() const
{
  return FullPlayCount;
}




int tSong::partialPlayCount() const
{
  return PartialPlayCount;
}




int tSong::rating() const
{
  return Rating;
}




int tSong::fileSize() const
{
  ensureInfoIsThere();
  return FileSize;
}



QString tSong::mood() const
{
  ensureInfoIsThere();
  return Mood;
}


QString tSong::tempo() const
{
  ensureInfoIsThere();
  return Tempo;
}


QString tSong::custom1() const
{
  ensureInfoIsThere();
  return Custom1;
}


QString tSong::custom2() const
{
  ensureInfoIsThere();
  return Custom2;
}


QString tSong::custom3() const
{
  ensureInfoIsThere();
  return Custom3;
}


QDomNode tSong::serialize(QDomDocument &doc) const
{
  ensureInfoIsThere();

  QDomElement result = doc.createElement("song");
  result.setAttribute("filename", QString("base64:")+encodeBase64(filename()).c_str());
  result.setAttribute("uniqueid", UniqueId);
  result.setAttribute("album", Album);
  result.setAttribute("artist", Artist);
  result.setAttribute("performer", Performer);
  result.setAttribute("title", Title);
  result.setAttribute("year", Year);
  result.setAttribute("genre", Genre);
  result.setAttribute("tracknumber", TrackNumber);
  result.setAttribute("duration", Duration);
  
  result.setAttribute("mood", Mood);
  result.setAttribute("tempo", Tempo);
  result.setAttribute("custom1", Custom1);
  result.setAttribute("custom2", Custom2);
  result.setAttribute("custom3", Custom3);

  result.setAttribute("existssince", (int) ExistsSince);
  result.setAttribute("lastplayed", (int) LastPlayed);
  result.setAttribute("lastmodified", (int) LastModified);
  result.setAttribute("fullplaycount", FullPlayCount);
  result.setAttribute("partialplaycount", PartialPlayCount);
  result.setAttribute("rating", Rating);
  result.setAttribute("filesize", (int) FileSize);
  return result;
}




void tSong::deserialize(const char **attributes)
{
  tFilename fn;
  fn = decodeFilename(lookupAttribute("filename", attributes));

  if (fn != filename())
  {
    throw tRuntimeError(qApp->translate("ErrorMessages", "deserializing to different filename"));
  }

  UniqueId = lookupAttribute("uniqueid", attributes).toUInt();
  Album = lookupAttribute("album", attributes);
  Artist = lookupAttribute("artist", attributes);
  try
  {
    Performer = lookupAttribute("performer", attributes);
  }
  catch (...)
  {
    Performer = qApp->translate("ErrorMessages", "<Unkown, reread tags>");
  }

  Title = lookupAttribute("title", attributes);
  Year = lookupAttribute("year", attributes);
  Genre = lookupAttribute("genre", attributes);
  TrackNumber = lookupAttribute("tracknumber", attributes);
  Duration = lookupAttribute("duration", attributes).toFloat();
  
  if (hasAttribute("mood", attributes))
    Mood = lookupAttribute("mood", attributes);
  else
    Mood = "";
  
  if (hasAttribute("tempo", attributes))
    Tempo = lookupAttribute("tempo", attributes);
  else
    Tempo = "";
  
  if (hasAttribute("custom1", attributes))
    Custom1 = lookupAttribute("custom1", attributes);
  else
    Custom1 = "";
  
  if (hasAttribute("custom2", attributes))
    Custom2 = lookupAttribute("custom2", attributes);
  else
    Custom2 = "";
  
  if (hasAttribute("custom3", attributes))
    Custom3 = lookupAttribute("custom3", attributes);
  else
    Custom3 = "";
  
  if (hasAttribute("existssince", attributes))
    ExistsSince = lookupAttribute("existssince", attributes).toUInt();

  if (hasAttribute("fullplaycount", attributes))
  {
    LastPlayed = lookupAttribute("lastplayed", attributes).toUInt();
    FullPlayCount = lookupAttribute("fullplaycount", attributes).toUInt();
    PartialPlayCount = lookupAttribute("partialplaycount", attributes).toUInt();

    if (FullPlayCount + PartialPlayCount == 0)
      LastPlayed = -1;
  }
  else
  {
    FullPlayCount = 0;
    PartialPlayCount = 0;
  }

  if (hasAttribute("lastmodified", attributes))
    LastModified = lookupAttribute("lastmodified", attributes).toUInt();
  else
  {
    // we'll believe that our mtime is way back, so we'll
    // update our tags.
    LastModified = 0;
  }

  if (hasAttribute("rating", attributes))
    Rating = lookupAttribute("rating", attributes).toInt();
  else
    Rating = -1;

  if (hasAttribute("filesize", attributes))
    FileSize = lookupAttribute("filesize", attributes).toInt();
  else
  {
    string fn = filename();
    struct stat statbuf;
    if (stat(fn.c_str(), &statbuf))
      throw runtime_error(("Unable to stat file "+filename()).c_str());
    FileSize = statbuf.st_size;
  }

  CacheValid = true;
}




void tSong::setUniqueId(tUniqueId id)
{
  UniqueId = id;
}




void tSong::setFilename(tFilename const &new_filename)
{
  if (SongCollection)
    SongCollection->noticeSongFilenameAboutToChange(this, filename(), new_filename);

  char *cstr_copy = strdup(new_filename.c_str());
  FilenameOnly = basename(cstr_copy);
  free(cstr_copy);
  cstr_copy = strdup(new_filename.c_str());
  tFilename pathname = dirname(cstr_copy);
  free(cstr_copy);

  bool found = false;
  IndexIntoDirectoryList = 0;
  FOREACH(first, DirectoryList, tDirectoryList)
  {
    if (pathname == *first)
    {
      found = true;
      break;
    }
    IndexIntoDirectoryList++;
  }
  if (!found)
  {
    IndexIntoDirectoryList = DirectoryList.size();
    DirectoryList.push_back(pathname);
  }

  CacheValid = false;

  if (SongCollection)
  {
    SongCollection->noticeSongModified(this, FIELD_FILE);
    SongCollection->noticeSongModified(this, FIELD_PATH);
  }
}




void tSong::invalidateCache() 
{
  CacheValid = false;

  // Cache invalidation does not entail song modification, so
  // don't call noticeSongModified here.
}




void tSong::setExistsSince(time_t value)
{
  ExistsSince = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_EXISTSSINCE);
}




void tSong::setLastModified(time_t value)
{
  LastModified = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_LASTMODIFIED);
}




void tSong::setLastPlayed(time_t value)
{
  LastPlayed = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_LASTPLAYED);
}




void tSong::setFullPlayCount(unsigned value)
{
  FullPlayCount = value;
  if (SongCollection)
  {
    SongCollection->noticeSongModified(this, FIELD_FULLPLAYCOUNT);
    SongCollection->noticeSongModified(this, FIELD_PLAYCOUNT);
  }
}




void tSong::setPartialPlayCount(unsigned value)
{
  PartialPlayCount = value;
  if (SongCollection)
  {
    SongCollection->noticeSongModified(this, FIELD_PARTIALPLAYCOUNT);
    SongCollection->noticeSongModified(this, FIELD_PLAYCOUNT);
  }
}




void tSong::setRating(int value)
{
  Rating = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_RATING);
}


void tSong::setMood(QString const &value)
{
  Mood = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_MOOD);
}

void tSong::setTempo(QString const &value)
{
  Tempo = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_TEMPO);
}

void tSong::setCustom1(QString const &value)
{
  Custom1 = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_CUSTOM1);
}

void tSong::setCustom2(QString const &value)
{
  Custom2 = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_CUSTOM2);
}

void tSong::setCustom3(QString const &value)
{
  Custom3 = value;
  if (SongCollection)
    SongCollection->noticeSongModified(this, FIELD_CUSTOM3);
}


QString tSong::fieldText(tSongField field) const
{
  try
  {
    switch (field)
    {
      case FIELD_ARTIST:
	return artist();
      case FIELD_PERFORMER:
	return performer();
      case FIELD_TITLE:
	return title();
      case FIELD_ALBUM:
	return album();
      case FIELD_TRACKNUMBER:
	return trackNumber();
      case FIELD_DURATION:
	return QString::number(duration());
      case FIELD_GENRE:
	return genre();
      case FIELD_FULLPLAYCOUNT:
	return QString::number(fullPlayCount());
      case FIELD_PARTIALPLAYCOUNT:
	return QString::number(partialPlayCount());
      case FIELD_PLAYCOUNT:
	return QString::number(playCount());

      case FIELD_LASTPLAYED:
	if (playCount())
	  return QString::number(lastPlayed());
	else
	  return "";

      case FIELD_RATING:
	return QString::number(rating());

      case FIELD_YEAR:
	return year();
      case FIELD_FILE:
	return QFile::decodeName(filenameOnly().c_str());
      case FIELD_PATH:
	return QFile::decodeName(pathname().c_str());
      case FIELD_SIZE:
	return QString::number(fileSize());

      case FIELD_EXISTSSINCE:
	return QString::number(existsSince());
      case FIELD_LASTMODIFIED:
	return QString::number(lastModified());
      case FIELD_MIMETYPE:
	return mimeType();
      case FIELD_UNIQUEID:
	return QString::number(uniqueId());
      
      case FIELD_MOOD:
	return mood();
      case FIELD_TEMPO:
	return tempo();
      case FIELD_CUSTOM1:
	return custom1();
      case FIELD_CUSTOM2:
	return custom2();
      case FIELD_CUSTOM3:
	return custom3();

      default:
	throw tRuntimeError("Invalid field in fieldText");
    }
  }
  catch (exception &ex)
  {
    return "<ERROR>";
  }
}




QString tSong::humanReadableFieldText(tSongField field) const
{
  switch (field)
  {
    case FIELD_DURATION:
      {
	int total_seconds = (int) duration();
	int seconds = total_seconds % 60;
	int minutes = total_seconds / 60;

	QString duration;
	duration.sprintf("%d:%02d", minutes, seconds);
	return duration;
      }
    case FIELD_LASTPLAYED:
      {
	if (playCount())
	{
	  QDateTime datetime;
	  datetime.setTime_t(lastPlayed());
	  return datetime.toString("MMM d h:mm");
	}
	else
	  return "";
      }
    case FIELD_EXISTSSINCE:
      {
	QDateTime datetime;
	datetime.setTime_t(existsSince());
	return datetime.toString("MMM d h:mm");
      }
    case FIELD_LASTMODIFIED:
      {
	QDateTime datetime;
	datetime.setTime_t(lastModified());
	return datetime.toString("MMM d h:mm");
      }
    case FIELD_RATING:
      {
	if (rating() == 0)
	{
	  return "-";
	}
	else if (rating() > 0)
	{
	  QString rating_str;
	  rating_str.fill('*', rating());
	  return rating_str;
	}
	else
	  return qApp->translate("Song fields", "not rated");
      }
      break;
    case FIELD_SIZE:
      return QString("%1 MB").arg(double(fileSize()) / (1024 * 1024), 0, 'f', 2);
    default:
      return fieldText(field);
  } 
}




void tSong::setFieldText(tSongField field, const QString &new_text)
{
  switch (field)
  {
    case FIELD_RATING:
      {
	bool ok;
	int rating = new_text.toInt(&ok);
	if (!ok)
	  throw tRuntimeError(QString("Invalid rating numeral '%1' in setFieldText").arg(new_text));

	setRating(rating);
	break;
      }
    case FIELD_MOOD:
      setMood(new_text);
      break;
    case FIELD_TEMPO:
      setTempo(new_text);
      break;
    case FIELD_CUSTOM1:
      setCustom1(new_text);
      break;
    case FIELD_CUSTOM2:
      setCustom2(new_text);
      break;
    case FIELD_CUSTOM3:
      setCustom3(new_text);
      break;
    default:
      throw tRuntimeError(QString("Cannot set field %1").arg(getFieldName(field)));
  }
}




void tSong::stripTag()
{
  stripTagInternal();
  readInfo();
}




void tSong::rewriteTag()
{
  QString my_album = album();
  QString my_artist = artist();
  QString my_performer = performer();
  QString my_title = title();
  QString my_year = year();
  QString my_genre = genre();
  QString my_track_number = trackNumber();

  stripTagInternal();

  setFieldText(FIELD_ALBUM, my_album);
  setFieldText(FIELD_ARTIST, my_artist);
  setFieldText(FIELD_PERFORMER, my_performer);
  setFieldText(FIELD_TITLE, my_title);
  setFieldText(FIELD_YEAR, my_year);
  setFieldText(FIELD_GENRE, my_genre);
  setFieldText(FIELD_TRACKNUMBER, my_track_number);
}




void tSong::ensureInfoIsThere() const
{
  if (!CacheValid)
    readInfo();
}





void tSong::readInfo() const
{
  struct stat statbuf;
  string fn = filename();
  if (stat(fn.c_str(), &statbuf))
    throw runtime_error(("Unable to stat file "+filename()).c_str());

  const_cast<tSong *>(this)->FileSize = statbuf.st_size;
  const_cast<tSong *>(this)->LastModified = statbuf.st_mtime;
  if (ExistsSince == 0)
    const_cast<tSong *>(this)->ExistsSince = statbuf.st_ctime;
}




void tSong::played(time_t when, bool full)
{
  if (full)
    setFullPlayCount(fullPlayCount() + 1);
  else
    setPartialPlayCount(fullPlayCount() + 1);

  setLastPlayed(when);
}




void tSong::resetStatistics()
{
  setFullPlayCount(0);
  setPartialPlayCount(0);
  setLastPlayed(-1);
}




void tSong::setCollection(tSongCollection *collection)
{
  SongCollection = collection;
}




// tMP3Song -------------------------------------------------------------------
class tMP3Song : public tSong
{
  public:
    QString mimeType() const
    { return "audio/mpeg"; }

    void readInfo() const;
    void setFieldText(tSongField field, QString const &value);

    void stripTagInternal();
};




void tMP3Song::readInfo() const
{
  using namespace TagLib;

  MPEG::File my_mp3(filename().c_str());
  if (!my_mp3.isOpen())
    throw runtime_error("Failed to open MP3 file");
  if (!my_mp3.isValid())
    throw runtime_error("File is probably not a valid MPEG stream");

  Tag *v1_tag = my_mp3.ID3v1Tag();
  ID3v2::Tag *v2_tag = my_mp3.ID3v2Tag();

  AudioProperties *props = my_mp3.audioProperties();
  if (props == NULL)
    throw runtime_error("Error obtaining MPEG properties");

  Tag *first, *second;
  if (tProgramBase::preferences().ID3ReadPreference == ID3_READ_PREFER_V2)
  {
    first = v2_tag;
    second = v1_tag;
  }
  else
  {
    first = v1_tag;
    second = v2_tag;
  }

  CacheValid = true;
  
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tMP3Song *>(this)->Artist, 
				getFieldString(FIELD_ARTIST, first, second), FIELD_ARTIST);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tMP3Song *>(this)->Title, 
				getFieldString(FIELD_TITLE, first, second), FIELD_TITLE);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tMP3Song *>(this)->Album, 
				getFieldString(FIELD_ALBUM, first, second), FIELD_ALBUM);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tMP3Song *>(this)->TrackNumber, 
				getFieldString(FIELD_TRACKNUMBER, first, second), FIELD_TRACKNUMBER);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tMP3Song *>(this)->Genre, 
				getFieldString(FIELD_GENRE, first, second), FIELD_GENRE);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tMP3Song *>(this)->Year,
				getFieldString(FIELD_YEAR, first, second), FIELD_YEAR);

  if (v2_tag && v2_tag->frameListMap().contains("TPE4"))
  {
    const List<ID3v2::Frame *> &frames = v2_tag->frameListMap()["TPE4"];
    if (frames.size())
      assignAndCheckForModification(this, SongCollection, 
				    const_cast<tMP3Song *>(this)->Performer,
				    convertTagLibString(frames[0]->toString()), 
				    FIELD_PERFORMER);
  }

  assignAndCheckForModification(this, SongCollection,
				const_cast<tMP3Song *>(this)->Duration, 
				props->length(), FIELD_DURATION);
  
  tSong::readInfo();
}




void tMP3Song::setFieldText(tSongField field, QString const &value)
{
  try
  {
    tSong::setFieldText(field, value);
    return;
  }
  catch (exception &ex)
  {
  }

  using namespace TagLib;

  {
    MPEG::File my_mp3(filename().c_str());
    if (!my_mp3.isOpen())
      throw runtime_error("Failed to open MP3 file");
    if (!my_mp3.isValid())
      throw runtime_error("File is probably not a valid MPEG stream");
    if (my_mp3.readOnly())
      throw runtime_error("Failed to open MP3 file for writing");

    Tag *v1_tag = my_mp3.ID3v1Tag(true);
    if (v1_tag == NULL)
      throw runtime_error("Error obtaining ID3v1 tag");

    ID3v2::Tag *v2_tag = my_mp3.ID3v2Tag(true);
    if (v2_tag == NULL)
      throw runtime_error("Error obtaining ID3v2 tag");

    if (field == FIELD_PERFORMER)
    {
      // v2 only
      String conv_value(value.utf8(), String::UTF8);
  
      auto_ptr<ID3v2::TextIdentificationFrame> my_frame(
	new ID3v2::TextIdentificationFrame("TPE4", String::UTF8));
      my_frame->setText(conv_value);
      v2_tag->removeFrames("TPE4");
      v2_tag->addFrame(my_frame.get());
      my_frame.release();
    }
    else
    {
      setFieldString(v1_tag, field, value);
      setFieldString(v2_tag, field, value);
    }

    my_mp3.save();
  }

  readInfo();
}




void tMP3Song::stripTagInternal()
{
  using namespace TagLib;

  MPEG::File my_mp3(filename().c_str());
  if (!my_mp3.isOpen())
    throw runtime_error("Failed to open MP3 file");
  if (!my_mp3.isValid())
    throw runtime_error("File is probably not a valid MPEG stream");
  if (my_mp3.readOnly())
    throw runtime_error("Failed to open MP3 file for writing");
  my_mp3.strip();
  my_mp3.save();
}




// tOggSong -------------------------------------------------------------------
class tOggSong : public tSong
{
  public:
    QString mimeType() const
    { return "audio/x-ogg"; }

    void readInfo() const;
    void setFieldText(tSongField field, QString const &value);

    void stripTagInternal();
};




void tOggSong::readInfo() const
{
  using namespace TagLib;

  Vorbis::File my_file(filename().c_str());
  if (!my_file.isOpen())
    throw runtime_error("Failed to open Ogg file");
  if (!my_file.isValid())
    throw runtime_error("File is probably not a valid Ogg Vorbis stream");

  Ogg::XiphComment *tag = my_file.tag();
  AudioProperties *props = my_file.audioProperties();
  if (props == NULL)
    throw runtime_error("Error obtaining MPEG properties");

  CacheValid = true;
  
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tOggSong *>(this)->Artist, 
				getFieldString(FIELD_ARTIST, tag), FIELD_ARTIST);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tOggSong *>(this)->Title, 
				getFieldString(FIELD_TITLE, tag), FIELD_TITLE);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tOggSong *>(this)->Album, 
				getFieldString(FIELD_ALBUM, tag), FIELD_ALBUM);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tOggSong *>(this)->TrackNumber, 
				getFieldString(FIELD_TRACKNUMBER, tag), FIELD_TRACKNUMBER);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tOggSong *>(this)->Genre, 
				getFieldString(FIELD_GENRE, tag), FIELD_GENRE);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tOggSong *>(this)->Year,
				getFieldString(FIELD_YEAR, tag), FIELD_YEAR);

  if (tag && tag->fieldListMap().contains("PERFORMER"))
  {
    const StringList &fields = tag->fieldListMap()["PERFORMER"];
    if (fields.size())
      assignAndCheckForModification(this, SongCollection, 
				    const_cast<tOggSong *>(this)->Performer,
				    convertTagLibString(fields[0]), FIELD_PERFORMER);
  }

  assignAndCheckForModification(this, SongCollection,
				const_cast<tOggSong *>(this)->Duration, 
				props->length(), FIELD_DURATION);
  
  tSong::readInfo();
}




void tOggSong::setFieldText(tSongField field, QString const &value)
{
  try
  {
    tSong::setFieldText(field, value);
    return;
  }
  catch (exception &ex)
  {
  }

  using namespace TagLib;

  Vorbis::File my_file(filename().c_str());
  if (!my_file.isOpen())
    throw runtime_error("Failed to open Vorbis file");
  if (!my_file.isValid())
    throw runtime_error("File is probably not a valid Vorbis stream");
  if (my_file.readOnly())
    throw runtime_error("Failed to open Vorbis file for writing");

  Ogg::XiphComment *tag = my_file.tag();
  if (tag == NULL)
    throw runtime_error("Error obtaining ID3v1 tag");

  if (field == FIELD_PERFORMER)
  {
    String conv_value(value.utf8(), String::UTF8);
    tag->addField("PERFORMER", conv_value);
  }
  else
    setFieldString(tag, field, value);

  my_file.save();
  readInfo();
}




void tOggSong::stripTagInternal()
{
  using namespace TagLib;

  Vorbis::File my_file(filename().c_str());
  if (!my_file.isOpen())
    throw runtime_error("Failed to open OGG file");
  if (!my_file.isValid())
    throw runtime_error("File is probably not a valid Vorbis stream");
  if (my_file.readOnly())
    throw runtime_error("Failed to open Vorbis file for writing");

  Ogg::XiphComment *tag = my_file.tag();
  while (tag->fieldListMap().size())
    tag->removeField(tag->fieldListMap().begin()->first);
  
  my_file.save();
}




// tFlacSong ------------------------------------------------------------------
class tFlacSong : public tSong
{
  public:
    QString mimeType() const
    { return "audio/x-flac"; }

    void readInfo() const;
    void setFieldText(tSongField field, QString const &value);

    void stripTagInternal();
};




void tFlacSong::readInfo() const
{
  using namespace TagLib;

  FLAC::File my_file(filename().c_str());
  if (!my_file.isOpen())
    throw runtime_error("Failed to open Flac file");
  if (!my_file.isValid())
    throw runtime_error("File is probably not a valid Flac stream");

  Tag *tag = my_file.tag();
  Ogg::XiphComment *xiphc = my_file.xiphComment();
  AudioProperties *props = my_file.audioProperties();
  if (props == NULL)
    throw runtime_error("Error obtaining MPEG properties");

  CacheValid = true;
  
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tFlacSong *>(this)->Artist, 
				getFieldString(FIELD_ARTIST, tag), FIELD_ARTIST);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tFlacSong *>(this)->Title, 
				getFieldString(FIELD_TITLE, tag), FIELD_TITLE);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tFlacSong *>(this)->Album, 
				getFieldString(FIELD_ALBUM, tag), FIELD_ALBUM);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tFlacSong *>(this)->TrackNumber, 
				getFieldString(FIELD_TRACKNUMBER, tag), FIELD_TRACKNUMBER);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tFlacSong *>(this)->Genre, 
				getFieldString(FIELD_GENRE, tag), FIELD_GENRE);
  assignAndCheckForModification(this, SongCollection, 
				const_cast<tFlacSong *>(this)->Year,
				getFieldString(FIELD_YEAR, tag), FIELD_YEAR);

  if (xiphc && xiphc->fieldListMap().contains("PERFORMER"))
  {
    const StringList &fields = xiphc->fieldListMap()["PERFORMER"];
    if (fields.size())
      assignAndCheckForModification(this, SongCollection, 
				    const_cast<tFlacSong *>(this)->Performer,
				    convertTagLibString(fields[0]), FIELD_PERFORMER);
  }

  assignAndCheckForModification(this, SongCollection,
				const_cast<tFlacSong *>(this)->Duration, 
				props->length(), FIELD_DURATION);
  
  tSong::readInfo();
}




void tFlacSong::setFieldText(tSongField field, QString const &value)
{
  try
  {
    tSong::setFieldText(field, value);
    return;
  }
  catch (exception &ex)
  {
  }

  using namespace TagLib;

  FLAC::File my_file(filename().c_str());
  if (!my_file.isOpen())
    throw runtime_error("Failed to open Flac file");
  if (!my_file.isValid())
    throw runtime_error("File is probably not a valid Flac stream");
  if (my_file.readOnly())
    throw runtime_error("Failed to open Flac file for writing");

  Ogg::XiphComment *tag = my_file.xiphComment();
  if (tag == NULL)
    throw runtime_error("Error obtaining flac tag");

  if (field == FIELD_PERFORMER)
  {
    String conv_value(value.utf8(), String::UTF8);
    tag->addField("PERFORMER", conv_value);
  }
  else
    setFieldString(tag, field, value);

  my_file.save();
  readInfo();
}




void tFlacSong::stripTagInternal()
{
  using namespace TagLib;

  FLAC::File my_file(filename().c_str());
  if (!my_file.isOpen())
    throw runtime_error("Failed to open Flac file");
  if (!my_file.isValid())
    throw runtime_error("File is probably not a valid Flac stream");
  if (my_file.readOnly())
    throw runtime_error("Failed to open Vorbis file for writing");

  Ogg::XiphComment *tag = my_file.xiphComment();
  while (tag->fieldListMap().size())
    tag->removeField(tag->fieldListMap().begin()->first);
  
  my_file.save();
}




#ifdef WITH_M4A
//tM4ASong -------------------------------------------------------------------
class tM4ASong : public tSong
{
  public:
    QString mimeType() const
    { return "audio/x-m4a"; }
    
    void readInfo() const;
    void setFieldText(tSongField field, QString const &value);

    void stripTagInternal(); 

};




//
//  C callbacks for metadata reading 
//
uint32_t md_read_callback(void *user_data, void *buffer, uint32_t length)
{
  mp4callback_data_t *file_data = (mp4callback_data_t*)user_data;
  return fread(buffer, 1, length, file_data->file);
}




uint32_t md_write_callback(void *user_data, void *buffer, uint32_t length)
{
  mp4callback_data_t *file_data = (mp4callback_data_t*)user_data;
  return fwrite(buffer, 1, length, file_data->file);
}




uint32_t md_truncate_callback(void *user_data) 
{
  mp4callback_data_t *file_data = (mp4callback_data_t*)user_data;
  ftruncate(file_data->fd, ftello(file_data->file));
  return 0;
}




uint32_t md_seek_callback(void *user_data, uint64_t position)
{
  mp4callback_data_t *file_data = (mp4callback_data_t*)user_data;
  return fseek(file_data->file, position, SEEK_SET);
}


int GetAACTrack(mp4ff_t *infile)
{
  /* find AAC track */
  unsigned int i, rc;
  int numTracks = mp4ff_total_tracks(infile);
  for (i = 0; i < numTracks; i++)
  {
    unsigned char *buff = NULL;
    unsigned int buff_size = 0;
    mp4AudioSpecificConfig mp4ASC;
    mp4ff_get_decoder_config(infile, i, &buff, &buff_size);
    if (buff)
    {
      rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
      free(buff);
      if (rc < 0)
        return -1;
      return i;
    }      
  }
  /* can"t decode this */
  return -1;
}




void tM4ASong::readInfo() const
{
  mp4ff_t *infile;
  char *tag_string=NULL;
  char *tag_item=NULL;
  mp4ff_callback_t *mp4cb;
  unsigned char *buffer;
  unsigned int buffer_size;
  mp4AudioSpecificConfig mp4ASC;
  int track = 0;
  
  mp4callback_data_t callback_data;
  callback_data.fd = open(filename().c_str(), O_RDWR);
  if (callback_data.fd < 0) {
    return ;
  }
     
  callback_data.file = fdopen(callback_data.fd, "r+");
  if (!callback_data.file)
  {
    close(callback_data.fd);
    return ;
  }
  
  mp4cb = (mp4ff_callback_t *) malloc(sizeof(mp4ff_callback_t)); 
  mp4cb->read = md_read_callback;
  mp4cb->seek = md_seek_callback;
  mp4cb->user_data = &callback_data;
      
  infile = mp4ff_open_read(mp4cb);
  mp4ff_meta_get_title(infile,&tag_string);
  assignAndCheckForModification(this, SongCollection,
        const_cast<tM4ASong *>(this)->Title,
        QString::fromUtf8(tag_string), FIELD_TITLE);
 
  mp4ff_meta_get_artist(infile,&tag_string);
  assignAndCheckForModification(this, SongCollection,
        const_cast<tM4ASong *>(this)->Artist,
        QString::fromUtf8(tag_string), FIELD_ARTIST);
      
  mp4ff_meta_get_album(infile,&tag_string);
  assignAndCheckForModification(this, SongCollection, 
        const_cast<tM4ASong *>(this)->Album,
       QString::fromUtf8(tag_string), FIELD_ALBUM);                                                                              
           
  mp4ff_meta_get_track(infile,&tag_string);
  assignAndCheckForModification(this, SongCollection,
        const_cast<tM4ASong *>(this)->TrackNumber,
        QString::fromUtf8(tag_string), FIELD_TRACKNUMBER);
       
  mp4ff_meta_get_genre(infile,&tag_string);
  assignAndCheckForModification(this, SongCollection,
        const_cast<tM4ASong *>(this)->Genre,
        QString::fromUtf8(tag_string), FIELD_GENRE);
       
  mp4ff_meta_get_date(infile,&tag_string);
  assignAndCheckForModification(this, SongCollection,
        const_cast<tM4ASong *>(this)->Year,
        QString::fromUtf8(tag_string), FIELD_YEAR);
                               
  if ((track = GetAACTrack(infile)) < 0)
  {

    mp4ff_close(infile);
    free(mp4cb);
    close(callback_data.fd);
    fclose(callback_data.file);

    return;
  }

  buffer = NULL;
  buffer_size = 0;
  mp4ff_get_decoder_config(infile, track, &buffer, &buffer_size);
  if (buffer)
  {
    AudioSpecificConfig(buffer, buffer_size, &mp4ASC);
    free(buffer);
  }
  long samples = mp4ff_num_samples(infile,track);
  float f = 1024.0;
  float seconds;
  if ((mp4ASC.sbr_present_flag == 1) || mp4ASC.forceUpSampling)
  {
    f = f * 2.0;
  }
  seconds = (float)samples*(float)(f-1.0)/(float)mp4ASC.samplingFrequency;
  assignAndCheckForModification(this, SongCollection,
       const_cast<tM4ASong *>(this)->Duration,
       seconds, FIELD_DURATION);
 
  free(tag_string);
  mp4ff_close(infile);
  free(mp4cb);
  close(callback_data.fd);
  fclose(callback_data.file);

  tSong::readInfo();

}




void  tM4ASong::setFieldText(tSongField field, QString const &value)
{
  try
  {
    tSong::setFieldText(field,value);
  }
  catch(exception ex)
  {
  }


   mp4callback_data_t callback_data;

    callback_data.fd = open(filename().c_str(), O_RDWR);
    if (callback_data.fd < 0) {
        return ;
    }
     
    callback_data.file = fdopen(callback_data.fd, "r+");
    if (!callback_data.file)
    {
        close(callback_data.fd);
        return ;
    }

    //
    //  Create the callback structure
    //

    mp4ff_callback_t *mp4_cb = (mp4ff_callback_t*) malloc(sizeof(mp4ff_callback_t));
    if (!mp4_cb) {
      close(callback_data.fd);
      fclose(callback_data.file);
      return;
    }
    mp4_cb->read = md_read_callback;
    mp4_cb->seek = md_seek_callback;
    mp4_cb->write = md_write_callback;
    mp4_cb->truncate = md_truncate_callback;
    mp4_cb->user_data = &callback_data;

    mp4ff_metadata_t * mp4ff_mdata = (mp4ff_metadata_t *)malloc(sizeof(mp4ff_metadata_t));
    if (!mp4ff_mdata) {
      free(mp4_cb);
      close(callback_data.fd);
      fclose(callback_data.file);
      return;
    }
    mp4ff_mdata->tags = (mp4ff_tag_t*)malloc(7 * sizeof(mp4ff_tag_t));
    if (!mp4ff_mdata) {
      free(mp4_cb);
      free(mp4ff_mdata);
      close(callback_data.fd);
      fclose(callback_data.file);
      return ;
    }

    //
    //  Open the mp4 input file  
    //                   

    mp4ff_t *mp4_ifile = mp4ff_open_read(mp4_cb);
    if (!mp4_ifile)
    {
        free(mp4_cb);
        free(mp4ff_mdata);
        close(callback_data.fd);
        fclose(callback_data.file);
        return ;
    } 
    
    mp4ff_mdata->tags[0].item = "artist";
    mp4ff_mdata->tags[0].value = strdup((char*)(FIELD_ARTIST == field ? value.ascii() : artist().ascii()));

    mp4ff_mdata->tags[1].item = "album";
    mp4ff_mdata->tags[1].value = strdup((char*)(FIELD_ALBUM == field ? value.ascii() : album().ascii()));

    mp4ff_mdata->tags[2].item = "title";
    mp4ff_mdata->tags[2].value = strdup((char*)(FIELD_TITLE == field ? value.ascii() : title().ascii()));

    mp4ff_mdata->tags[3].item = "genre";
    mp4ff_mdata->tags[3].value = strdup((char*) (FIELD_GENRE == field ? value.ascii() : genre().ascii()));

    mp4ff_mdata->tags[4].item = "date";
    mp4ff_mdata->tags[4].value =  (char*)malloc(128);
    snprintf(mp4ff_mdata->tags[4].value, 128, "%d",(FIELD_YEAR == field ? value.toUInt() : year().toUInt()));

    mp4ff_mdata->tags[5].item = "track";
    mp4ff_mdata->tags[5].value = (char*)malloc(128);
    snprintf(mp4ff_mdata->tags[5].value, 128, "%d",  (FIELD_TRACKNUMBER == field ? value.toUInt() : trackNumber().toUInt()));

    mp4ff_mdata->count =6;

    for(int i=0;i<mp4ff_mdata->count;i++) if(strcmp(mp4ff_mdata->tags[i].value, "")==0) mp4ff_mdata->tags[i].value = "Unknown";
    
    mp4ff_meta_update(mp4_cb, mp4ff_mdata);
    
    mp4ff_close(mp4_ifile);
    free(mp4_cb);
    close(callback_data.fd);
    fclose(callback_data.file);
   

    free(mp4ff_mdata->tags);
    free(mp4ff_mdata);
    readInfo();

}




void  tM4ASong::stripTagInternal()
{

}
#endif




//tTaglessSong ---------------------------------------------------------------
class tTaglessSong : public tSong
{
  public:
    QString mimeType() const 
      { return "audio/unknown";}
    void stripTagInternal() { }
    void readInfo() const;
};




void tTaglessSong::readInfo() const
{
  tFilename::size_type slash_pos = filename().rfind("/");
  QString name = string2QString(filename().substr(slash_pos+1));
  assignAndCheckForModification(this,SongCollection,
                                const_cast<tTaglessSong *>(this)->Title,
                                name, FIELD_TITLE);
  tSong::readInfo();
}




// public ---------------------------------------------------------------------
tSong *makeSong(tFilename const &filename)
{
  tFilename::size_type dot_pos = filename.rfind(".");
  if (dot_pos == tFilename::npos || filename.find("/", dot_pos) != tFilename::npos) 
    throw tRuntimeError(
      qApp->translate("ErrorMessages", "Unknown file type: %1")
      .arg(string2QString(filename)));
  tFilename ext = filename.substr(dot_pos+1);

  QString extension = string2QString(ext).lower();

  if (extension == "mp3")
  {
    tSong *song = new tMP3Song;
    song->setFilename(filename);
    return song;
  }
  else if (extension == "ogg")
  {
    tSong *song = new tOggSong;
    song->setFilename(filename);
    return song;
  }
  else if (extension == "flac")
  {
    tSong *song = new tFlacSong;
    song->setFilename(filename);
    return song;
  }
#ifdef WITH_M4A
  else  if (extension == "m4a")
  {
    tSong *song = new tM4ASong;
    song->setFilename(filename);
    return song;
  }
#endif
  else 
  {
    QStringList::iterator it = tProgramBase::preferences().TaglessExtensions.begin();
    while (it != tProgramBase::preferences().TaglessExtensions.end())
    {
      if(extension == *it)
      {
        tSong *song = new tTaglessSong;
        song->setFilename(filename);
        return song;
      }
      ++it;
    }
  }
  throw tRuntimeError(
      qApp->translate("ErrorMessages", "Unknown file type: %1")
      .arg(string2QString(filename)));
}





tSong *deserializeSong(const char **attributes)
{
  tFilename filename;
  filename = decodeFilename(lookupAttribute("filename", attributes));

  tSong *song = makeSong(filename);
  song->deserialize(attributes);
  return song;
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
