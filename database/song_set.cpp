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
#include <qmessagebox.h>
#include <stdexcept>
#include <qinputdialog.h>
#include <qregexp.h>
#include <qapplication.h>
#include <qdir.h>

#include "database/song_set.h"
#include "database/criterion.h"
#include "database/song_list_tools.h"
#include "utility/progress.h"




namespace
{
  template<class T>
  QDomNode serializeUniqueIdContainer(QDomDocument &doc, QString const &name, T const &uid_list)
  {
    QDomElement result = doc.createElement(name);
    FOREACH_CONST(first, uid_list, typename T)
    {
      QDomElement song = doc.createElement("song");
      result.appendChild(song);
      song.setAttribute("unique_id", *first);
    }
    return result;
  }
}





// tSongSet -------------------------------------------------------------------
tSongSet::tSongSet()
{
  initialize();
}




tSongSet::tSongSet(const tSongSet &src)
{
  initialize();
  assign(src);
}




tSongSet::~tSongSet()
{ 
}




void tSongSet::initialize()
{
  SongCollection = NULL;
  BulkChangeLevel = 0;
  setCriterion("~none");
}




void tSongSet::assign(const tSongSet &src)
{
  SongCollection = src.SongCollection;

  setCriterion(src.Criterion);
  hasChanged(true);
}




tSongSet &tSongSet::operator=(const tSongSet &src)
{
  assign(src);
  return *this;
}




void tSongSet::setSongCollection(tSongCollection *collection)
{
  if (SongCollection)
    disconnect(SongCollection, NULL, this, NULL);

  clear(false);
  
  SongCollection = collection;
  if (SongCollection)
  {
    connect(SongCollection, SIGNAL(notifyBeginBulkChange()),
	this, SLOT(noticeCollectionBeginBulkChange()));
    connect(SongCollection, SIGNAL(notifyEndBulkChange()),
	this, SLOT(noticeCollectionEndBulkChange()));
    connect(SongCollection, SIGNAL(notifyAddition(const tSong *)),
	this, SLOT(noticeCollectionAddition(const tSong *)));
    connect(SongCollection, SIGNAL(notifySubstitution(const tSong *, const tSong *)),
	this, SLOT(noticeCollectionSubstitution(const tSong *, const tSong *)));
    connect(SongCollection, SIGNAL(notifyDeletion(const tSong *)),
	this, SLOT(noticeCollectionDeletion(const tSong *)));
    connect(SongCollection, SIGNAL(notifySongModified(const tSong *, tSongField)),
	this, SLOT(noticeSongModified(const tSong *, tSongField)));

    reevaluateCriterion();
  }
  noticeCollectionSwitched();
}




void tSongSet::render(tConstSongList &rendering, tProgress *progress) const
{
  tSongList pre_rendering;
  render(pre_rendering, progress);

  rendering.clear();
  FOREACH(first, pre_rendering, tSongList)
    rendering.push_back(*first);
}




void tSongSet::renderToUniqueIdList(tUniqueIdList &uidlist, tProgress *progress)
{
  tSongList rendering;
  render(rendering, progress);

  uidlist.clear();
  FOREACH(first, rendering, tSongList)
    uidlist.push_back((*first)->uniqueId());
}




void tSongSet::clear(bool criterion)
{
  if (criterion)
    setCriterion("~none");
}




void tSongSet::setCriterion(tCriterion *crit)
{
  auto_ptr<tCriterion> parsed_crit(crit);
  ParsedCriterion = parsed_crit;
}




void tSongSet::setCriterion(const QString &new_crit)
{
  if (new_crit == Criterion)
    return;

  auto_ptr<tCriterion> parsed_crit(parseCriterion(new_crit));
  ParsedCriterion = parsed_crit;

  Criterion = new_crit;
}




void tSongSet::beginBulkChange()
{
  if (BulkChangeLevel == 0)
    SubstantialBulkChange = false;
  BulkChangeLevel++;
}




void tSongSet::endBulkChange()
{
  /* The if is necessary since this happens during loading:
   * Newly loaded song sets catch the endBulkChange() after
   * loading finishes, but weren't loaded for the beginBulkChange()
   * when it started.
   */

  if (BulkChangeLevel > 0)
    BulkChangeLevel--;

  hasChanged(SubstantialBulkChange);
  SubstantialBulkChange = false;
}




void tSongSet::hasChanged(bool substantial_change, tProgress *progress)
{
  if (BulkChangeLevel == 0)
    emit notifyChange(substantial_change, progress);
  else
  {
    if (substantial_change)
      SubstantialBulkChange = true;
  }
}



 
// tSearchSongSet -------------------------------------------------------------
tSearchSongSet::tSearchSongSet()
{
  initialize();
}




tSearchSongSet::tSearchSongSet(const tSearchSongSet &src)
  : tSongSet(src)
{
  initialize();
  assign(src);
}




void tSearchSongSet::initialize()
{
  DoSort = false;
  SAUDirty = true;
  CollectionBulkChangeLevel = 0;
  ResortOnModification = true;
}




void tSearchSongSet::assign(const tSearchSongSet &src)
{
  setSort(src.DoSort,
          src.Ascending,
          src.SortField,
          src.SecondarySortField,
          src.TertiarySortField);
}




tSearchSongSet &tSearchSongSet::operator=(const tSearchSongSet &src)
{
  tSongSet::operator=(src);
  assign(src);
  return *this;
}




void tSearchSongSet::reevaluateCriterion(tProgress *progress)
{
  if (SAUDirty)
    rebuildSortedAndUnrestricted();

  Rendering.clear();

  unsigned progress_index = 0;
  FOREACH_CONST(first, SortedAndUnrestricted, tSongList)
  {
    float match = ParsedCriterion->matchDegree(*first);
    if (match >= 0.5)
      Rendering.push_back(*first);

    if (progress_index % 100 == 0 && progress)
    {
      progress->processEvents();
      if (progress->wasCancelled())
	return;
    }
    progress_index++;
  }
  hasChanged(true, progress);
}




void tSearchSongSet::render(tSongList &rendering, tProgress * /*progress*/) const
{
  rendering = Rendering;
}




void tSearchSongSet::setSort(bool do_sort,  bool ascending,
    tSongField field, tSongField secondary, tSongField tertiary)
{
  DoSort = do_sort;
  SortField = field;
  SecondarySortField = secondary;
  TertiarySortField = tertiary;
  Ascending = ascending;

  SAUDirty = true;
  reevaluateCriterion();
  hasChanged(true);
}




void tSearchSongSet::setSortOnModification(bool value)
{
  if (value && ! ResortOnModification)
    reevaluateCriterion();
  ResortOnModification = value;
}




void tSearchSongSet::rebuildSortedAndUnrestricted()
{
  if (!SongCollection)
    throw tRuntimeError("rebuildSortedAndUnrestricted: No song collection available");

  SortedAndUnrestricted.clear();
  FOREACH_CONST(first, *SongCollection, tSongCollection)
    SortedAndUnrestricted.push_back(*first);

  if (DoSort)
  {
    sort(SortedAndUnrestricted, SortField, SecondarySortField, 
        TertiarySortField);
    if (!Ascending)
      reverse(SortedAndUnrestricted.begin(), SortedAndUnrestricted.end());
  }
  SAUDirty = false;
}




bool tSearchSongSet::containsSong(tUniqueId uid) const
{
  tSong *song = SongCollection->getByUniqueId(uid);
  if (song)
     return ParsedCriterion->matchDegree(song) > 0.5;
  else
  {
    throw tRuntimeError("tSearchSongSet::containsSong: Song uniqueId nonexistant");
  }
}




void tSearchSongSet::noticeCollectionSwitched()
{
  SAUDirty = true;
  reevaluateCriterion();
  hasChanged(false);
}




void tSearchSongSet::noticeCollectionBeginBulkChange()
{
  beginBulkChange();
  CollectionBulkChangeLevel++;
}




void tSearchSongSet::noticeCollectionEndBulkChange()
{
  CollectionBulkChangeLevel--;

  if (CollectionBulkChangeLevel == 0)
  {
    SAUDirty = true;
    reevaluateCriterion();
    endBulkChange();
  }
}




void tSearchSongSet::noticeCollectionAddition(const tSong *song)
{
  if (CollectionBulkChangeLevel == 0)
  {
    bool is_in = ParsedCriterion->matchDegree(song) > 0.5;

    addSongToSortedSongList(SortedAndUnrestricted, song);
    if (is_in)
    {
      // only rely on the sortedness of the rendering if it is being maintained
      if (ResortOnModification)
      {
        addSongToSortedSongList(Rendering, song);
        hasChanged(false);
      }
      else
        reevaluateCriterion();
    }
  }
}




void tSearchSongSet::noticeCollectionSubstitution(const tSong *song, const tSong *old)
{
  tSongList::iterator it = find(SortedAndUnrestricted.begin(),
				SortedAndUnrestricted.end(), old);
  if (it == SortedAndUnrestricted.end())
    throw tRuntimeError("Song to be substituted was not in SAU.");
  *it = const_cast<tSong *>(song);
  
  it = find(Rendering.begin(), Rendering.end(), old);
  if (it != Rendering.end())
    *it = const_cast<tSong *>(song);
  
  hasChanged(false);
}




void tSearchSongSet::noticeCollectionDeletion(const tSong *song)
{
  tSongList::iterator it = find(SortedAndUnrestricted.begin(),
				SortedAndUnrestricted.end(), song);
  if (it == SortedAndUnrestricted.end())
    throw tRuntimeError("Song to be deleted was not in SAU.");
  SortedAndUnrestricted.erase(it);
  
  it = find(Rendering.begin(), Rendering.end(), song);
  if (it != Rendering.end())
    Rendering.erase(it);
  
  hasChanged(false);
}




void tSearchSongSet::noticeSongModified(const tSong *song, tSongField field)
{
  if (CollectionBulkChangeLevel == 0)
  {
    tSongList::iterator it = find(Rendering.begin(), Rendering.end(), song);
    bool was_in = it != Rendering.end();
    bool is_in = ParsedCriterion->matchDegree(song) > 0.5;

    bool take_out = (was_in && !is_in);
    bool put_in = (is_in && !was_in);

    if (ResortOnModification &&
        is_in && (field == SortField || 
                  field == SecondarySortField || 
                  field == TertiarySortField))
    {
      take_out = was_in;
      put_in = true;
    }

    removeSongFromSongList(SortedAndUnrestricted, song);
    addSongToSortedSongList(SortedAndUnrestricted, song);

    if (take_out)
      Rendering.erase(it);
    if (put_in)
      addSongToSortedSongList(Rendering, song);
    
    if (take_out || put_in)
      hasChanged(false);
  }

  emit notifySongModified(song, field);
}




void tSearchSongSet::addSongToSortedSongList(tSongList &sl, const tSong *song)
{
  tLessContainer less(getLess(SortField, 
                              SecondarySortField,
                              TertiarySortField));
  
  tSongList::iterator at = lower_bound(sl.begin(), sl.end(), 
                                       const_cast<tSong *>(song), less);
  sl.insert(at, const_cast<tSong *>(song));
}




void tSearchSongSet::removeSongFromSongList(tSongList &sl, const tSong *song)
{
  tSongList::iterator it = find(sl.begin(),
                                sl.end(),
                                song);
  if (it == sl.end())
    throw tRuntimeError(qApp->translate("ErrorMessages",
                                        "Song assumed to be in songlist, but not found."));
  sl.erase(it);
}




// tPlaylist ------------------------------------------------------------------
tPlaylist::tPlaylist()
{
}




tPlaylist::tPlaylist(const tPlaylist &src)
  : tSongSet(src)
{
  assign(src);
}




void tPlaylist::assign(const tPlaylist &src)
{
  Rendering = src.Rendering;
  RenderingSet = src.RenderingSet;
  PositiveSet = src.PositiveSet;
  NegativeSet = src.NegativeSet;

  hasChanged(true);
}



tPlaylist &tPlaylist::operator=(const tPlaylist &src)
{
  assign(src);
  tSongSet::operator=(src);
  return *this;
}




void tPlaylist::sortBy(tSongField field, tSongField secondary, tSongField tertiary, bool ascending)
{
  sort(Rendering, field, secondary, tertiary);
  if (!ascending)
    ::reverse(Rendering.begin(), Rendering.end());

  hasChanged(false);
}




void tPlaylist::jumble()
{
  srand(time(NULL));

  for (unsigned i = 0; i < Rendering.size(); i++)
  {
    int new_index = rand() % Rendering.size();
    tSong *buffer_old = Rendering[ new_index ];
    Rendering[ new_index ] = Rendering[ i ];
    Rendering[ i ] = buffer_old;
  }
  hasChanged(false);
}




void tPlaylist::reverse()
{
  std::reverse(Rendering.begin(), Rendering.end());
  hasChanged(false);
}




void tPlaylist::add(tSong *song, int preferred_index)
{
  float match = ParsedCriterion->matchDegree(song);
  addToRendering(song, preferred_index);
  if (match < 0.5)
    PositiveSet.insert(song->uniqueId());
  NegativeSet.erase(song->uniqueId());

  hasChanged(false);
}




void tPlaylist::add(const tFilename &filename, int preferred_index)
{
  tSong *song = SongCollection->getByFilename(filename);
  if (song)
    add(song, preferred_index);
  else
    throw tRuntimeError(tr("Unable to add unknown file %1" )
        .arg(QString::fromUtf8(filename.c_str())));
}




void tPlaylist::remove(int index)
{
  if (index >= (int) Rendering.size())
  {
    qWarning("tried to remove song out of bounds");
    return;
  }

  tSong *song = Rendering[ index ];
  removeFromRendering(index);

  float match = ParsedCriterion->matchDegree(song);

  PositiveSet.erase(song->uniqueId());
  if (match >= 0.5)
    NegativeSet.insert(song->uniqueId());

  hasChanged(false);
}




void tPlaylist::remove(tSong *song)
{
  removeFromRendering(song);

  PositiveSet.erase(song->uniqueId());
  float match = ParsedCriterion->matchDegree(song);
  if (match >= 0.5)
    NegativeSet.insert(song->uniqueId());

  hasChanged(false);
}




void tPlaylist::clear(bool criterion)
{
  Rendering.clear();
  RenderingSet.clear();
  PositiveSet.clear();
  NegativeSet.clear();

  tSongSet::clear(criterion);
}




void tPlaylist::reevaluateCriterion(tProgress *progress)
{
  if (progress)
    progress->setWhat(tr("Searching..."));

  unsigned progress_index = 0;
  // reevaluate current rendering
  unsigned index = 0;
  while (index < Rendering.size())
  {
    tSong *song = Rendering[ index ];

    float match = ParsedCriterion->matchDegree(song);

    if (match < 0.5 && PositiveSet.count(song->uniqueId()) == 0)
      removeFromRendering(index);
    else
      index++;

    if (progress_index % 100 == 0 && progress)
    {
      progress->processEvents();
      if (progress->wasCancelled())
	return;
    }
    progress_index++;
  }

  // reevaluate the whole collection
  FOREACH_CONST(first, *SongCollection, tSongCollection)
  {
    tUniqueId uid = (*first)->uniqueId();

    float match = ParsedCriterion->matchDegree((*first));
    if (match >= 0.5 && (NegativeSet.count(uid) == 0)
	&& !containsSong(uid))
      addToRendering(*first);

    if (progress_index % 100 == 0 && progress)
    {
      progress->processEvents();
      if (progress->wasCancelled())
	return;
    }
    progress_index++;
  }

  hasChanged(true, progress);
}




void tPlaylist::render(tSongList &rendering, tProgress * /*progress*/) const
{
  rendering = Rendering;
}




void tPlaylist::noticeCollectionSwitched()
{
}




void tPlaylist::noticeCollectionBeginBulkChange()
{
  beginBulkChange();
}




void tPlaylist::noticeCollectionEndBulkChange()
{
  endBulkChange();
}




void tPlaylist::noticeCollectionAddition(const tSong *song)
{
  float match = ParsedCriterion->matchDegree(song);
  if (match >= 0.5 && NegativeSet.count(song->uniqueId()) == 0)
    addToRendering(song);

  hasChanged(false);
}




void tPlaylist::noticeCollectionSubstitution(const tSong *song, const tSong *old)
{
  tSongList::iterator it = find(Rendering.begin(), Rendering.end(), old);
  if (it != Rendering.end())
    *it = const_cast<tSong *>(song);

  hasChanged(false);
}




void tPlaylist::noticeCollectionDeletion(const tSong *song)
{
  bool affected = false;
  int index = 0;
  while (index < (int) Rendering.size())
  {
    if (Rendering[index] == song)
    {
      removeFromRendering(index);
      affected = true;
    }
    else
      index++;
  }

  NegativeSet.erase(song->uniqueId());
  PositiveSet.erase(song->uniqueId());

  if (affected)
    hasChanged(false);
}




void tPlaylist::noticeSongModified(const tSong *song, tSongField field)
{
  // matchesCriterion needs to be called first since it may actually recursively
  // end up calling this function!
  float match = ParsedCriterion->matchDegree(song);
  bool matches_criterion = match >= 0.5;
  bool in_rendering = containsSong(song->uniqueId());

  if (in_rendering)
  {
    if (!matches_criterion && PositiveSet.count(song->uniqueId()) == 0)
    {
      removeFromRendering(song);
      hasChanged(false);
    }
  }
  else
  {
    if (matches_criterion && NegativeSet.count(song->uniqueId()) == 0)
    {
      addToRendering(song);
      hasChanged(false);
    }
  }

  emit notifySongModified(song, field);
}




QDomNode tPlaylist::serialize(QDomDocument &doc)
{
  QDomElement result = doc.createElement("song_set");
  result.setAttribute("criterion", Criterion);

  tUniqueIdList rendering;
  renderToUniqueIdList(rendering);

  result.appendChild(serializeUniqueIdContainer(doc, "rendering", rendering));
  result.appendChild(serializeUniqueIdContainer(doc, "positiveset", PositiveSet));
  result.appendChild(serializeUniqueIdContainer(doc, "negativeset", NegativeSet));

  return result;
}




void tPlaylist::setRendering(const tUniqueIdList &list)
{
  FOREACH_CONST(first, list, tUniqueIdList)
  {
    tSong *song = SongCollection->getByUniqueId(*first);
    if (song)
    {
      RenderingSet.insert(*first);
      Rendering.push_back(song);
    }
  }
}




void tPlaylist::setPositiveSet(const tUniqueIdList &list)
{
  FOREACH_CONST(first, list, tUniqueIdList)
  {
    tSong *song = SongCollection->getByUniqueId(*first);
    if (song)
      PositiveSet.insert(*first);
  }
}




void tPlaylist::setNegativeSet(const tUniqueIdList &list)
{
  FOREACH_CONST(first, list, tUniqueIdList)
  {
    tSong *song = SongCollection->getByUniqueId(*first);
    if (song)
      NegativeSet.insert(*first);
  }
}




void tPlaylist::addToRendering(const tSong *song, int preferred_index)
{
  tUniqueId uid = song->uniqueId();

  if (preferred_index == -1 || preferred_index > (int) Rendering.size())
    Rendering.push_back(const_cast<tSong *>(song));
  else
    Rendering.insert(Rendering.begin() + preferred_index, 
	const_cast<tSong *>(song));

  // multiple insertions don't hurt
  RenderingSet.insert(uid);
}




void tPlaylist::removeFromRendering(const tSong *song)
{
  int index = 0;
  while (index < (int) Rendering.size())
  {
    if (Rendering[index] == song)
      removeFromRendering(index);
    else
      index++;
  }
}




void tPlaylist::removeFromRendering(int index)
{
  tSong *song = Rendering[ index ];
  tUniqueId uid = song->uniqueId();

  Rendering.erase(Rendering.begin() + index);

  // is there no other instance? 
  FOREACH(first, Rendering, tSongList)
  {
    if (*first == song) 
      return;
  }
  // then erase song from the rendering set
  RenderingSet.erase(uid);
}




void importM3UIntoPlaylist(tPlaylist *song_set, const QString &filename)
{
  if (song_set == NULL)
    return;

  // FIXME encodings unclear
  QFile m3ufile(filename);
  QDir m3udir = QFileInfo(filename).dir(true);
  
  if (m3ufile.open(IO_ReadOnly))
  {
    QTextStream textstr(&m3ufile);
    
    while (!textstr.atEnd())
    {
      QString song_fn = textstr.readLine().stripWhiteSpace();
      if (song_fn[0] == '#')
	continue;

      QFileInfo fi(song_fn);
      if (fi.isRelative())
        song_fn = m3udir.absFilePath(song_fn, true);

      try
      {
	song_set->add(string(song_fn.utf8()));
      }
      catch (...)
      {
      }
    }
  }
  else
    throw tRuntimeError(QString("Could not open M3U file '%1'").arg(filename));
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
