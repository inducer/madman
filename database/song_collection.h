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




#ifndef SONG_COLLECTION
#define SONG_COLLECTION




#include <vector>
#include <qobject.h>

#include "song.h"







class tProgress;




class tSongCollection : public QObject
{
    Q_OBJECT

    typedef hash_map<tUniqueId, tSong *> tUniqueIdMap;
    typedef hash_map<tFilename, tSong *, hash_string> tFilenameMap;

    tSongList SongList;
    tSongList New;
    tSongList Garbage; // to be deleted at destruction time

    tUniqueIdMap UniqueIdMap;
    tFilenameMap FilenameMap;

    tUniqueId NextUniqueId;

  public:

    typedef tSongList::iterator iterator;
    typedef tSongList::const_iterator const_iterator;

    tSongCollection();
    ~tSongCollection();

    const_iterator begin() const
    {
      return SongList.begin();
    }
    const_iterator end() const
    {
      return SongList.end();
    }

  public slots:
    void readTags(tProgress *progress,
	const QString &progresstext = "Reading tags...", 
	tSongList *list = NULL);
    void rereadTags(tProgress *progress);
    void scan(tDirectoryList const &dl, tProgress *progress);
    void insert(tSong *song, bool loaded);
    void substitute(tSong *song, tSong *old);
    void remove(tSong *song);
    void clear();

    void noticeSongModified(const tSong *song, tSongField field);
    void noticeSongFilenameAboutToChange(tSong *song, const tFilename &old, const tFilename &current);

    void beginBulkChange();
    void endBulkChange();

  public:
    QDomNode serialize(QDomDocument &doc) const;

    tSong *getByFilename(const tFilename &filename) const;
    tSong *getByUniqueId(tUniqueId uid) const;
    bool isValid(const tSong *song) const;

  signals:
    void notifyBeginBulkChange();
    void notifyEndBulkChange();
    void notifyAddition(const tSong *song);
    void notifySubstitution(const tSong *song, const tSong *old);
    void notifyDeletion(const tSong *song);
    void notifyLoad(const tSong *song);
    void notifySongModified(const tSong *song, tSongField field);

  private:
    tUniqueId nextUniqueId();
    void scanSingleDirectory(tFilename const &dir, tProgress *progress);

    // make this type noncopyable
    tSongCollection(const tSongCollection &src);
    tSongCollection &operator=(const tSongCollection &src);
};




class tSongCollectionScopedBulkChange
{
  tSongCollection &Collection;
  public:
    tSongCollectionScopedBulkChange(tSongCollection &collection)
      : Collection(collection)
    {
      Collection.beginBulkChange();
    }
    ~tSongCollectionScopedBulkChange()
    {
      Collection.endBulkChange();
    }

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
