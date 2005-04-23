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




#ifndef _HEADER_SEEN_SONGSET_H
#define _HEADER_SEEN_SONGSET_H




#include <algorithm>
#include <memory>
#include "song_collection.h"




// tSongSet -------------------------------------------------------------------
class tProgress;
class tCriterion;

class tSongSet : public QObject
{
    Q_OBJECT

  protected:
    tSongCollection *SongCollection;

    QString Criterion;
    auto_ptr<tCriterion> ParsedCriterion;

    int BulkChangeLevel;
    bool SubstantialBulkChange;

  public:
    tSongSet();
    tSongSet(const tSongSet &src);
    virtual ~tSongSet();

  private:
    void initialize();
    void assign(const tSongSet &src);

  public:
    virtual tSongSet &operator=(const tSongSet &src);
    virtual tSongSet *duplicate() = 0;

    /** This subroutine first clears the content of this song
     * set, then switches over to the new collection, then
     * reevaluates the criterion.
     */
    void setSongCollection(tSongCollection *collection);

    virtual void render(tSongList &rendering, tProgress *progess = NULL ) const = 0;
    void render(tConstSongList &rendering, tProgress *progess = NULL ) const;

    void renderToUniqueIdList(tUniqueIdList &rendering, tProgress *progess = NULL );

    virtual bool containsSong(tUniqueId uid) const = 0;

    virtual void clear(bool criterion);

    // criterion management ---------------------------------------------------
    QString const &criterion() const
    {
      return Criterion;
    }

    /// This object assumes ownership of \c crit.
    void setCriterion(tCriterion *crit);
    void setCriterion(const QString &crit);
    virtual void reevaluateCriterion(tProgress *progess = NULL) = 0;

  signals:
    void notifyChange(bool substanial_change, tProgress *progress);
    void notifySongModified(const tSong *song, tSongField field);

  protected slots:
    virtual void noticeCollectionSwitched() = 0;
    virtual void noticeCollectionBeginBulkChange() = 0;
    virtual void noticeCollectionEndBulkChange() = 0;
    virtual void noticeCollectionAddition(const tSong *song) = 0;
    virtual void noticeCollectionSubstitution(const tSong *song, const tSong *old) = 0;
    virtual void noticeCollectionDeletion(const tSong *song) = 0;
    virtual void noticeSongModified(const tSong *tSong, tSongField field) = 0;

  // change management --------------------------------------------------------
  public:
    void beginBulkChange();
    void endBulkChange();

  protected:
    void hasChanged(bool substantial_change, tProgress *progress = NULL);
};




class tSearchSongSet : public tSongSet
{
    bool		DoSort;
    tSongField 		SortField, SecondarySortField, TertiarySortField;
    bool		Ascending;

    tSongList		Rendering;
    tSongList		SortedAndUnrestricted;
    bool		SAUDirty;
    int 		CollectionBulkChangeLevel;

  public:
    tSearchSongSet();
    tSearchSongSet(const tSearchSongSet &src);

  private:
    void initialize();
    void assign(const tSearchSongSet &src);

  public:
    virtual tSearchSongSet &operator=(const tSearchSongSet &src);
    tSearchSongSet *duplicate()
      { return new tSearchSongSet(*this); }

    void reevaluateCriterion(tProgress *progess = NULL);
    void render(tSongList &rendering, tProgress *progess = NULL ) const;

    void setSort(bool do_sort, bool ascending = true, 
        tSongField field = FIELD_ARTIST,
        tSongField secondary = FIELD_INVALID,
        tSongField tertiary = FIELD_INVALID
        );
    void rebuildSortedAndUnrestricted();

    bool containsSong(tUniqueId uid) const;

  protected:
    void noticeCollectionSwitched();
    void noticeCollectionBeginBulkChange();
    void noticeCollectionEndBulkChange();
    void noticeCollectionAddition(const tSong *song);
    void noticeCollectionSubstitution(const tSong *song, const tSong *old);
    void noticeCollectionDeletion(const tSong *song);
    void noticeSongModified(const tSong *tSong, tSongField field);
};




class tPlaylist : public tSongSet
{
    Q_OBJECT;

    tSongList Rendering;
    tUniqueIdSet RenderingSet,PositiveSet,NegativeSet;

  public:
    tPlaylist();
    tPlaylist(const tPlaylist &src);

  private:
    void assign(const tPlaylist &src);

  public:
    tPlaylist &operator=(const tPlaylist &src);
    tPlaylist *duplicate()
      { return new tPlaylist(*this); }
      

    void sortBy(tSongField field, tSongField secondary, tSongField tertiary, bool ascending = true);
    void jumble();
    void reverse();
    void clear(bool criterion);

    void reevaluateCriterion(tProgress *progess = NULL);
    void render(tSongList &rendering, tProgress *progess = NULL ) const;

    void add(tSong *song, int preferred_index = -1);
    void add(const tFilename &filename, int preferred_index = -1);
    void remove(int index);
    void remove(tSong *song);

    bool containsSong(tUniqueId uid) const
    {
      return RenderingSet.count(uid) != 0;
    }

  protected slots:
    void noticeCollectionSwitched();
    void noticeCollectionBeginBulkChange();
    void noticeCollectionEndBulkChange();
    void noticeCollectionAddition(const tSong *song);
    void noticeCollectionSubstitution(const tSong *song, const tSong *old);
    void noticeCollectionDeletion(const tSong *song);
    void noticeSongModified(const tSong *tSong, tSongField field);

  public:
    QDomNode serialize(QDomDocument &doc);
    void setRendering(const tUniqueIdList &list);
    void setPositiveSet(const tUniqueIdList &list);
    void setNegativeSet(const tUniqueIdList &list);

  protected:
    void addToRendering(const tSong *song, int preferred_index = -1);
    void removeFromRendering(const tSong *song);
    void removeFromRendering(int index);
};




void importM3UIntoPlaylist(tPlaylist *song_set, const QString &filename);




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
