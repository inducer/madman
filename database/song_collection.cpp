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




#include "utility/progress.h"

#include <dirent.h>
#include <sys/stat.h>
#include <qapplication.h>
#include <qprogressdialog.h>
#include <qdir.h>

#include "song_collection.h"




tSongCollection::tSongCollection()
    : NextUniqueId(0)
{}




tSongCollection::~tSongCollection()
{
  FOREACH(first, Garbage, tSongList)
    delete *first;
  FOREACH(first, SongList, tSongList)
    delete *first;
}




void tSongCollection::readTags(tProgress *progress,
    const QString &progresstext, 
    tSongList *list)
{
  if (list == NULL) list = &SongList;
  progress->setWhat(progresstext);
  progress->setTotalSteps(list->size());
  int index = 0;

  FOREACH(first, *list, tSongList)
  {
    cout << "read tag: " << (*first)->filename() << endl;
    try
    {
      (*first)->ensureInfoIsThere();
    }
    catch (runtime_error &ex)
    {
      cerr 
        << "*** error while reading tags from " << (*first)->filename() << ":"<< endl
        << "    " << ex.what() << endl;
    }

    ++index;
    if (index % 10 == 0)
    {
      progress->setProgress(index);
      if (progress->wasCancelled())
	break;
    }
  }
}




void tSongCollection::rereadTags(tProgress *progress)
{
  progress->setWhat(tr("Reading tags..."));
  progress->setTotalSteps(SongList.size());
  int index = 0;

  FOREACH(first, SongList, tSongList)
  {
    cout << "read tag: " << (*first)->filename() << endl;
    (*first)->invalidateCache();
    (*first)->ensureInfoIsThere();

    ++index;
    if (index % 10 == 0)
    {
      progress->setProgress(index);
      if (progress->wasCancelled())
	break;
    }
  }
}




void tSongCollection::scan(tDirectoryList const &dl, tProgress *progress)
{
  emit notifyBeginBulkChange();

  unsigned moves_detected = 0, tags_reread = 0;

  // scan for new songs
  progress->setWhat(tr("Scanning for files..."));

  New.clear();
  progress->setTotalSteps(0);
  FOREACH_CONST(first, dl, tDirectoryList)
    scanSingleDirectory(*first, progress);

  // look for removed songs
  progress->setWhat(tr("Looking for dropouts and orphans..."));
  progress->setTotalSteps(SongList.size());
  int index = 0;

  tSongList orphans;
  FOREACH(first, SongList, tSongList)
  {
    // check if song is still in desired directory
    bool is_orphan = true;
    FOREACH_CONST(first_d, dl, tDirectoryList)
      if ((*first)->filename().substr(0, first_d->length()) == *first_d)
	is_orphan = false;

    if (is_orphan)
      orphans.push_back(*first);
    else
    {
      // check if song file is still present
      struct stat statbuf;
      if (stat((*first)->filename().c_str(), &statbuf))
	orphans.push_back(*first);
    }

    ++index;
    if (index % 10 == 0)
    {
      progress->setProgress(index);
    }
  }

  // read tags now so move detection can use them.
  readTags(progress, tr("Updating existing tags..."));
  readTags(progress, tr("Reading tags of new songs..."), &New);

  // compare them to new songs to detect moves
  progress->setWhat(tr("Detecting moves..."));
  progress->setTotalSteps(orphans.size());
  index = 0;

  tSongList::iterator first_o = orphans.begin(), last_o = orphans.end();
  while (first_o != last_o)
  {
    bool deorphanized = false;
    tSongList::iterator first_n = New.begin(), last_n = New.end();
    while (first_n != last_n)
    {
      if ((*first_n)->album() == (*first_o)->album()
           && (*first_n)->artist() == (*first_o)->artist()
           && (*first_n)->title() == (*first_o)->title())
      {
        cerr 
	  << "move: " << (*first_o)->filename() << " -> " << (*first_n)->filename() << endl;
	moves_detected++;

	(*first_n)->setUniqueId((*first_o)->uniqueId());
	(*first_n)->setExistsSince((*first_o)->existsSince());
	// lastModified kept from new song.
	(*first_n)->setLastPlayed((*first_o)->lastPlayed());
	(*first_n)->setFullPlayCount((*first_o)->fullPlayCount());
	(*first_n)->setPartialPlayCount((*first_o)->partialPlayCount());
	(*first_n)->setRating((*first_o)->rating());

	substitute(*first_n, *first_o);
        New.erase(first_n);

	Garbage.push_back(*first_o);

	first_o = orphans.erase(first_o);
	last_o = orphans.end();

	deorphanized = true;
        break;
      }
      first_n++;
    }
    if (!deorphanized)
      first_o++;

    progress->setProgress(++index);
  }

  // remove orphaned songs
  progress->setWhat(tr("Removing orphaned songs..."));
  progress->setTotalSteps(orphans.size());
  index = 0;
  FOREACH(first, orphans, tSongList)
  {
    cerr << "delete: " << (*first) ->filename() << endl;
    remove(*first);

    ++index;
    if (index % 10 == 0)
    {
      progress->setProgress(index);
    }
  }

  // add new songs
  progress->setWhat(tr("Adding songs..."));
  progress->setTotalSteps(New.size());
  index = 0;

  FOREACH(first, New, tSongList)
  {
    cerr << "add: " << (*first)->filename() << endl;
    try
    {
      // this is needed for the sake of proper exception handling,
      // fix courtesy of Klaus S. Madsen
      (*first)->ensureInfoIsThere();

      insert(*first, false);
    }
    catch (exception &ex)
    {
      cerr 
        << "*** exception while adding" << endl
        << "*** " <<(*first)->filename() << ":" << endl
        << "*** " << ex.what() << endl;
      delete *first;
    }

    ++index;
    if (index % 10 == 0)
    {
      progress->setProgress(index);
    }
  }

  // reread tags of songs that were modified since last scan
  progress->setWhat(tr("Updating tags of changed songs..."));
  progress->setTotalSteps(SongList.size());

  index = 0;
  FOREACH(first, SongList, tSongList)
  {
    struct stat statbuf;
    if (stat((*first)->filename().c_str(), &statbuf))
      cerr 
        << "*** File not present after orphan detection: " 
        << (*first)->filename().c_str() << endl;
    else
    {
      if (statbuf.st_mtime > (*first)->lastModified())
      {
        cerr 
	  << "changed: " << (*first) ->filename() << endl;

        (*first)->invalidateCache();
        (*first)->ensureInfoIsThere();
	tags_reread++;
      }
    }

    ++index;
    if (index % 10 == 0)
    {
      progress->setProgress(index);
    }
  }

  emit notifyEndBulkChange();

  cout 
    << "scan statistics:" << endl
    << "  # of songs not previously in database:" << New.size() << endl
    << "  # of songs removed:" << orphans.size() << endl
    << "  # of songs moved:" << moves_detected << endl
    << "  # of songs changed:" << tags_reread << endl;
}




void tSongCollection::insert(tSong *song, bool loaded)
{
  if (song->uniqueId() == SONG_UID_INVALID)
    song->setUniqueId(nextUniqueId());
  SongList.push_back(song);
  FilenameMap.insert(make_pair(song->filename(), song));
  UniqueIdMap.insert(make_pair(song->uniqueId(), song));
  if (loaded)
    emit notifyLoad(song);
  else
    emit notifyAddition(song);

  song->setCollection(this);
}




void tSongCollection::substitute(tSong *song, tSong *old)
{
  FilenameMap.erase(old->filename());
  UniqueIdMap.erase(old->uniqueId());
  FilenameMap.insert(make_pair(song->filename(), song));
  UniqueIdMap.insert(make_pair(song->uniqueId(), song));
  SongList.push_back(song);
  emit notifySubstitution(song, old);

  FOREACH(first, SongList, tSongList)
    if (*first == old)
    {
      SongList.erase(first);
      break;
    }
}




void tSongCollection::remove(tSong *song)
{
  emit notifyDeletion(song);
  FilenameMap.erase(song->filename());
  UniqueIdMap.erase(song->uniqueId());

  FOREACH(first, SongList, tSongList)
    if (*first == song)
    {
      SongList.erase(first);
      break;
    }
  Garbage.push_back(song);
}




void tSongCollection::clear()
{
  emit notifyBeginBulkChange();
  while (SongList.size() > 0)
    remove(SongList[ SongList.size() - 1 ]);
  emit notifyEndBulkChange();
}




void tSongCollection::noticeSongModified(const tSong *song, tSongField field)
{
  emit notifySongModified(song, field);
}




void tSongCollection::noticeSongFilenameAboutToChange(tSong *song, const tFilename &old, const tFilename &current)
{
  FilenameMap.erase(string(old));
  FilenameMap.insert(make_pair(current, song));
}




void tSongCollection::beginBulkChange()
{
  emit notifyBeginBulkChange();
}




void tSongCollection::endBulkChange()
{
  emit notifyEndBulkChange();
}




QDomNode tSongCollection::serialize(QDomDocument &doc) const
{
  QDomElement result = doc.createElement("songs");
  FOREACH_CONST(first, SongList, tSongList)
    result.appendChild((*first)->serialize(doc));
  return result;
}




tSong *tSongCollection::getByFilename(tFilename const &filename) const
{
  tFilenameMap::const_iterator it = FilenameMap.find(filename);
  if (it == FilenameMap.end())
    return NULL;
  else
    return it->second;
}




tSong *tSongCollection::getByUniqueId(tUniqueId uid) const
{
  tUniqueIdMap::const_iterator it = UniqueIdMap.find(uid);
  if (it == UniqueIdMap.end())
    return NULL;
  else
    return it->second;
}




bool tSongCollection::isValid(const tSong *song) const
{
  FOREACH_CONST(first, SongList, tSongList)
    if ((*first) == song)
      return true;
  return false;
}




tUniqueId tSongCollection::nextUniqueId()
{
  while (getByUniqueId(NextUniqueId))
    NextUniqueId++;
  return NextUniqueId++;
}




void tSongCollection::scanSingleDirectory(tFilename const &dir, tProgress *progress)
{
  vector<tFilename> files;
  enumerateFiles(dir, files, progress);

  FOREACH(first, files, vector<tFilename>)
    try 
    {
      if (FilenameMap.find(*first) == FilenameMap.end())
      {
        auto_ptr<tSong> song(makeSong(*first));
        New.push_back(song.get());
        song.release();
      }
    }
    catch (...)
    {
    }
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
