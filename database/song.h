/*
madman - a music manager
Copyright (C) 2003  Andreas Kloeckner <ak@ixion.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/




#ifndef SONG
#define SONG




#include <qstring.h>
#include <qdom.h>
#include <qobject.h>
#include "utility/base.h"




#define SONG_UID_INVALID (-1)




enum tSongField {
  FIELD_ARTIST = 0,
  FIELD_PERFORMER = 1,
  FIELD_TITLE = 2,
  FIELD_ALBUM = 3,
  FIELD_TRACKNUMBER = 4,
  FIELD_DURATION = 5,
  FIELD_GENRE = 6,

  FIELD_FULLPLAYCOUNT = 7,
  FIELD_PARTIALPLAYCOUNT = 8,
  FIELD_PLAYCOUNT = 9,
  FIELD_LASTPLAYED = 10,
  FIELD_RATING = 11,

  FIELD_YEAR = 12,
  FIELD_FILE = 13,
  FIELD_PATH = 14,
  FIELD_SIZE = 15,

  FIELD_EXISTSSINCE = 16,
  FIELD_LASTMODIFIED = 17,
  FIELD_MIMETYPE = 18,
  FIELD_UNIQUEID = 19,
  
  FIELD_MOOD = 20,
  FIELD_TEMPO = 21,
  FIELD_CUSTOM1 = 22,
  FIELD_CUSTOM2 = 23,
  FIELD_CUSTOM3 = 24,
  
  FIELD_REAL_COUNT = 25,
   
  
  FIELD_EXTRA_FIRST = (FIELD_REAL_COUNT),
  FIELD_COUNT = (FIELD_REAL_COUNT + 0),
  FIELD_INVALID = 0x100000
};




//#ifdef WITH_M4A 
typedef struct
{
  FILE* file;
  int fd; //only used for truncating/writing.
} mp4callback_data_t;
//#endif



const int GENRE_COUNT = 126;

QString genreIdToString(int genre);
QString getFieldName(tSongField field);
QString getFieldIdentifier(tSongField field);
tSongField getFieldFromIdentifier(const QString &field);
QString substituteSongFields(QString const &format, tSong *song, 
                             bool human_readable, bool shell_quote = false);
string substituteSongFieldsUtf8(QString const &format, tSong *song, 
                                bool human_readable, bool shell_quote = false);





class tSongCollection;


class tSong 
{
  protected:
    typedef vector<tFilename> tDirectoryList;
    static tDirectoryList DirectoryList;
    
    tFilename FilenameOnly;
    unsigned IndexIntoDirectoryList;

    tUniqueId UniqueId;

    tSongCollection *SongCollection;

    QString Album, Artist, Performer, Title, Year, Genre, TrackNumber, 
            Mood, Tempo, Custom1, Custom2, Custom3;

    float Duration;
    long int FileSize;

    time_t ExistsSince;
    time_t LastPlayed, LastModified;
    int FullPlayCount,PartialPlayCount;
    // -1 if unspecified
    int Rating;
    
    mutable bool CacheValid;

  public:
    tSong();
    virtual ~tSong();

    tFilename filename() const;
    tFilename pathname() const;
    tFilename filenameOnly() const;
    tUniqueId uniqueId() const;

    QString album() const;
    QString artist() const;
    QString performer() const;
    QString title() const;
    QString year() const;
    QString genre() const;
    QString trackNumber() const;
    
    QString mood() const;
    QString tempo() const;
    QString custom1() const;
    QString custom2() const;
    QString custom3() const;
    
    float duration() const;

    time_t existsSince() const;
    time_t lastModified() const;
    time_t lastPlayed() const;
    int playCount() const;
    int fullPlayCount() const;
    int partialPlayCount() const;
    int rating() const;
    
    int fileSize() const;
    virtual QString mimeType() const = 0 ;

    virtual QDomNode serialize(QDomDocument &doc) const;
    virtual void deserialize(const char **attributes);

    void setUniqueId(tUniqueId id);
    void setFilename(tFilename const &filename);
    void invalidateCache();

    void setExistsSince(time_t value);
    void setLastModified(time_t value);
    void setLastPlayed(time_t value);
    void setFullPlayCount(unsigned value);
    void setPartialPlayCount(unsigned value);
    void setRating(int value);
    void setMood(QString const &value);
    void setTempo(QString const &value);
    void setCustom1(QString const &value);
    void setCustom2(QString const &value);
    void setCustom3(QString const &value);

    QString fieldText(tSongField field) const;
    QString humanReadableFieldText(tSongField field) const;
    virtual void setFieldText(tSongField field, const QString &new_text);

    void stripTag();
    void rewriteTag();

    void ensureInfoIsThere() const;
    /* tSong::readInfo needs to be called *after* any information
     * has been read by the subclass.
     */
    virtual void readInfo() const;

    void played(time_t when, bool full);
    void resetStatistics();

    void setCollection(tSongCollection *collection);

  protected:
    virtual void stripTagInternal() = 0;
};




// song factories -------------------------------------------------------------
tSong *makeSong(const tFilename &filename);
tSong *deserializeSong(const char **attributes);




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
