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




#include "database/database.h"
#include "utility/progress.h"

#include "expat/expat.h"

#include <qregexp.h>
#include <qfile.h>
#include <qdom.h>
#include <qapplication.h>

#include <stdexcept>
#include <cstring>




// tSAXDatabaseHandler --------------------------------------------------------
class tSAXDatabaseHandler
{
    enum tMode {
      NONE,
      DIRECTORIES,
      HISTORY,
      SONGS,
      SONG_SET,
      SONG_SET_NODE,
      SONG_SET_RENDERING,
      SONG_SET_POSITIVE_SET,
      SONG_SET_NEGATIVE_SET
    } CurrentMode;

    tDatabase &Database;
    tProgress	*Progress;
    XML_Parser		Parser;

    typedef vector<tPlaylistNode *> tPlaylistNodeStack;
    tPlaylistNodeStack	PlaylistNodeStack;

    tUniqueIdList	UniqueIdList;

  public:
    tSAXDatabaseHandler(tDatabase &database, tProgress *progress,
	XML_Parser parser);

    void startElement(const XML_Char *name, const XML_Char **attributes);
    void endElement(const XML_Char *name);
};




tSAXDatabaseHandler::tSAXDatabaseHandler(tDatabase &database, tProgress *progress,
    XML_Parser parser)
  : CurrentMode(NONE), Database(database), Progress(progress), Parser(parser)
{
}





void tSAXDatabaseHandler::startElement(const XML_Char *name, const XML_Char **attributes)
{
  if (Progress) 
  {
    int current_progress = XML_GetCurrentByteIndex(Parser);
    if (current_progress - Progress->progress() > 100000)
      Progress->setProgress(current_progress);
  }

  // items --------------------------------------------------------------------
  if (strcmp(name, "song") == 0)
  {
    switch (CurrentMode)
    {
      case SONGS:
        try
        {
          Database.SongCollection.insert(deserializeSong(attributes), true);
        }
        catch (exception &ex)
        {
          cerr 
            << "*** WHOOPS" << endl
            << "The sax start element handler for a song threw an exception:" << endl
            << ex.what() << endl
            << "Skipping this song." << endl;
        }
	return;
      case SONG_SET_RENDERING:
      case SONG_SET_NEGATIVE_SET:
      case SONG_SET_POSITIVE_SET:
	UniqueIdList.push_back(lookupAttribute("unique_id", attributes).toUInt());
	return;
      default:
	;
    }
  }
  if (strcmp(name, "directory") == 0 && CurrentMode == DIRECTORIES)
  {
    string dir = lookupAttributeUtf8("name", attributes);
    if (dir.substr(0, 7) == "base64:")
      Database.DirectoryList.push_back(decodeBase64(dir.substr(7)));
    else
      Database.DirectoryList.push_back(dir.c_str());

    return;
  }
  if (strcmp(name, "playedsong") == 0 && CurrentMode == HISTORY)
  {
    Database.History.played(
	lookupAttribute("uniqueid", attributes).toUInt(),
	lookupAttribute("time", attributes).toUInt(),
	lookupAttribute("playduration", attributes).toDouble()
	);
    return;
  }

  // mode changes -------------------------------------------------------------
  if (strcmp(name, "madmandb") == 0 && CurrentMode == NONE)
    return;
  if (strcmp(name, "songs") == 0 && CurrentMode == NONE)
  {
    CurrentMode = SONGS;
    return;
  }
  if (strcmp(name, "directories") == 0 && CurrentMode == NONE)
  {
    CurrentMode = DIRECTORIES;
    return;
  }
  if (strcmp(name, "history") == 0 && CurrentMode == NONE)
  {
    CurrentMode = HISTORY;
    return;
  }
  if (strcmp(name, "song_set_node") == 0 && (CurrentMode == NONE || CurrentMode == SONG_SET_NODE))
  {
    CurrentMode = SONG_SET_NODE;
    tPlaylistNode *my_node = new tPlaylistNode(&Database, NULL);
    my_node->setName(lookupAttribute("name", attributes));

    if (PlaylistNodeStack.size() == 0)
      Database.setPlaylistTree(my_node);
    else
    {
      tPlaylistNode *parent = PlaylistNodeStack.back();
      parent->makeChildNodeNameUnique(my_node);
      parent->insertChild(my_node, parent->end());
    }

    PlaylistNodeStack.push_back(my_node);
    return;
  }
  if (strcmp(name, "song_set") == 0 && (CurrentMode == SONG_SET_NODE))
  {
    CurrentMode = SONG_SET;
    tPlaylist *my_set = new tPlaylist;
    my_set->setSongCollection(&Database.SongCollection);
    PlaylistNodeStack.back()->setData(my_set);

    QString criterion_string = lookupAttribute("criterion", attributes);
    try
    {
      my_set->setCriterion(criterion_string);
    }
    catch (...)
    {
      // If we can't compile the criterion, just comment it out

      cerr << "*** WARNING *** The criterion " << criterion_string 
	<< " failed to compile." << endl
	<< "Commenting out." << endl;

      // First, remove all previous comments to prevent breakage.
      QRegExp comment_re("\\{[^}]*\\}");
      while (comment_re.search(criterion_string) != -1)
	criterion_string = criterion_string.replace(comment_re, "");

      // Then, set the new commented-out criterion.
      my_set->setCriterion(QString("{ COMPILE FAILURE ON LOAD, COMMENTED OUT: %1 }").arg(criterion_string));
    }
    return;
  }
  if (strcmp(name, "rendering") == 0 && (CurrentMode == SONG_SET))
  {
    CurrentMode = SONG_SET_RENDERING;
    UniqueIdList.clear();
    return;
  }
  if (strcmp(name, "positiveset") == 0 && (CurrentMode == SONG_SET))
  {
    CurrentMode = SONG_SET_POSITIVE_SET;
    UniqueIdList.clear();
    return;
  }
  if (strcmp(name, "negativeset") == 0 && (CurrentMode == SONG_SET))
  {
    CurrentMode = SONG_SET_NEGATIVE_SET;
    UniqueIdList.clear();
    return;
  }

  // error condition ----------------------------------------------------------
  cerr << "error: " << QString::fromUtf8(name, strlen(name)) << endl;
  throw tRuntimeError(qApp->translate("ErrorMessages", "Invalid database structure" ));
}




void tSAXDatabaseHandler::endElement(const XML_Char *name)
{
  if (strcmp(name, "madmandb") == 0 && CurrentMode == NONE)
    return;
  if (strcmp(name, "songs") == 0 && CurrentMode == SONGS)
  {
    CurrentMode = NONE;
    return;
  }
  if (strcmp(name, "directories") == 0 && CurrentMode == DIRECTORIES)
  {
    CurrentMode = NONE;
    return;
  }
  if (strcmp(name, "history") == 0 && CurrentMode == HISTORY)
  {
    CurrentMode = NONE;
    return;
  }
  if (strcmp(name, "song_set_node") == 0 && CurrentMode == SONG_SET_NODE)
  {
    PlaylistNodeStack.pop_back();
    if (PlaylistNodeStack.size() == 0)
      CurrentMode = NONE;
    else
      CurrentMode = SONG_SET_NODE;

    return;
  }
  if (strcmp(name, "song_set") == 0 && (CurrentMode == SONG_SET))
  {
    CurrentMode = SONG_SET_NODE;
    return;
  }
  if (strcmp(name, "rendering") == 0 && (CurrentMode == SONG_SET_RENDERING))
  {
    CurrentMode = SONG_SET;
    PlaylistNodeStack.back()->data()->setRendering(UniqueIdList);
    return;
  }
  if (strcmp(name, "positiveset") == 0 && (CurrentMode == SONG_SET_POSITIVE_SET))
  {
    CurrentMode = SONG_SET;
    PlaylistNodeStack.back()->data()->setPositiveSet(UniqueIdList);
    return;
  }
  if (strcmp(name, "negativeset") == 0 && (CurrentMode == SONG_SET_NEGATIVE_SET))
  {
    CurrentMode = SONG_SET;
    PlaylistNodeStack.back()->data()->setNegativeSet(UniqueIdList);
    return;
  }
}




// expat parsing code ---------------------------------------------------------
void startExpatElement(void *data, const XML_Char *el, const XML_Char **attr)
{
  try
  {
    reinterpret_cast<tSAXDatabaseHandler *>(data)->startElement(el, attr);
  }
  catch (exception &ex)
  {
    cerr 
      << "*** WHOOPS" << endl
      << "The sax start element handler for " << el << " threw an exception:" << endl
      << ex.what() << endl
      << "Cannot continue. This is probably a bug." << endl;
    abort();
  }
}




void endExpatElement(void *data, const XML_Char *el) 
{
  try
  {
    reinterpret_cast<tSAXDatabaseHandler *>(data)->endElement(el);
  }
  catch (exception &ex)
  {
    cerr 
      << "*** WHOOPS" << endl
      << "The sax end element handler for " << el << " threw an exception:" << endl
      << ex.what() << endl
      << "Cannot continue, sorry. This is probably a bug." << endl;
    abort();
  }
} 




// tDatabase ------------------------------------------------------------------
tDatabase::tDatabase()
{
}




void tDatabase::setPlaylistTree(tPlaylistNode *node)
{
  auto_ptr<tPlaylistNode> my_node(node);
  PlaylistTree = my_node;
  noticePlaylistTreeChanged();
}




void tDatabase::clear()
{
  FileLock = auto_ptr<tFileLock>();
  setPlaylistTree(NULL);

  SongCollection.clear();
  DirectoryList.clear();
  History.clear();
}




void tDatabase::startNew()
{
  clear();
  setPlaylistTree(new tPlaylistNode(this, new tPlaylist));
  PlaylistTree->data()->setSongCollection(&SongCollection);
  PlaylistTree->setName(qApp->translate("PlaylistName", "Root Playlist"));

  tPlaylistNode *node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~rating(>=4)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Absolute favorites"));
  PlaylistTree->addChild(node);
  
  node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~album(love)|~title(love)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Love songs"));
  PlaylistTree->addChild(node);
  
  node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~genre(rock)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Rock songs"));
  PlaylistTree->addChild(node);

  node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~play_count(=0)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Never been listened"));
  PlaylistTree->addChild(node);

  node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~existed_for_days(<30)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Recent additions"));
  PlaylistTree->addChild(node);

  node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~full_play_ratio(>=0.5)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Often listened to the end"));
  PlaylistTree->addChild(node);

  node = new tPlaylistNode(this, new tPlaylist);
  node->data()->setSongCollection(&SongCollection);
  node->data()->setCriterion("~full_play_ratio(<0.5)");
  node->data()->reevaluateCriterion();
  node->setName(qApp->translate("PlaylistName", "Rarely listened to the end"));
  PlaylistTree->addChild(node);
}




void tDatabase::load(const QString &filename, bool break_lock, tProgress *progress)
{
  clear();
  QFile dbfile(filename);

  auto_ptr<tFileLock> my_new_lock(
    new tFileLock(QFile::encodeName(filename).data(), break_lock));

  if (dbfile.open(IO_ReadOnly))
  {
    if (progress)
    {
      progress->setTotalSteps(dbfile.size());
      progress->setProgress(0);
    }

    tSongCollectionScopedBulkChange bulkchange(SongCollection);

    XML_Parser parser = XML_ParserCreate(NULL);
    if (parser == NULL)
      throw runtime_error("expat: unable to create parser");

    tSAXDatabaseHandler handler(*this, progress, parser);

    XML_SetUserData(parser, &handler);
    XML_SetElementHandler(parser, startExpatElement, endExpatElement);

    char buffer[1<<8];
    while (!dbfile.atEnd())
    {
      Q_LONG read_bytes = dbfile.readBlock(buffer, sizeof(buffer));
      if (XML_Parse(parser, buffer, read_bytes, dbfile.atEnd()) == 0)
      {
	throw tRuntimeError(tr("Error in XML Parser: %1 - line %2")
                            .arg(QString::fromUtf8(
                                   XML_ErrorString(
                                     XML_GetErrorCode(parser))))
                            .arg(XML_GetCurrentLineNumber(parser)));
      }
    }

    XML_ParserFree(parser);
    dbfile.close();
    FileLock = my_new_lock;
  }
  else
    throw tRuntimeError(qApp->translate("ErrorMessages", "Database file not accessible: %1").arg(filename));
}




void tDatabase::save(const QString &filename, tProgress *progress)
{
  if (progress)
  {
    progress->setTotalSteps(5);
    progress->setProgress(0);
  }

  QDomDocument configxml;
  QDomElement doc_element = configxml.createElement("madmandb");
  configxml.appendChild(doc_element);

  // save song collection
  doc_element.appendChild(SongCollection.serialize(configxml));
  if (progress) progress->setProgress(1);

  // save directories
  QDomElement dirs_element = configxml.createElement("directories");
  FOREACH(first_d, DirectoryList, tDirectoryList)
  {
    QDomElement dir_element = configxml.createElement("directory");
    dir_element.setAttribute("name", string2QString("base64:" + encodeBase64(*first_d)));
    dirs_element.appendChild(dir_element);
  }
  doc_element.appendChild(dirs_element);
  if (progress) progress->setProgress(2);

  // order matters, song set tree needs to be serialized after songs
  if (playlistTree())
    doc_element.appendChild(playlistTree()->serialize(configxml));
  if (progress) progress->setProgress(3);

  doc_element.appendChild(History.serialize(configxml));
  if (progress) progress->setProgress(4);

  QFile dbfile(filename);
  if (dbfile.open(IO_WriteOnly))
  {
    QTextStream stream(&dbfile);
    stream.setEncoding(QTextStream::UnicodeUTF8);
    stream << configxml;
  }
  else
    throw tRuntimeError(qApp->translate("ErrorMessages", "Database file not writable: %1").arg(filename));
  if (progress) progress->setProgress(5);
}



void tDatabase::noticePlaylistTreeChanged()
{
  emit notifyPlaylistTreeChanged();
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
