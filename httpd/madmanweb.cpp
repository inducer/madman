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
#include <qregexp.h>
#include <qsocket.h>
#include <qurl.h>
#include <qdom.h>

#include "utility/player.h"
#include "utility/prefs.h"

#include "database/program_base.h"
#include "database/database.h"
#include "database/auto_dj.h"

#include "madmanweb.h"
#include "webdata.h"




namespace 
{
  QString decodeUrlString(QString const &url_string)
  {
    QString copy = url_string;
    copy.replace("+", " ");
    QUrl::decode(copy);
    return copy;
  }
}




// tool functions -------------------------------------------------------------
QString replaceQueryValue(const QUrl &url, const QString &sought_key, const QString &new_value)
{
  QStringList fields = QStringList::split("&", url.query());

  QRegExp key_value_re("^([^=]+)\\=(.*)$");
  bool found = false;
  FOREACH(first, fields, QStringList)
  {
    if (!key_value_re.exactMatch(*first))
      throw tRuntimeError("Trouble extracting query field");
    QString key = key_value_re.cap(1);
    QString value = key_value_re.cap(2);

    QString decoded_key = decodeUrlString(key);

    if (decoded_key == sought_key)
    {
      QString encoded_value = new_value;
      QUrl::encode(encoded_value);
      *first = QString("%1=%2").arg(key).arg(encoded_value);
      found = true;
    }
  }
  if (!found)
  {
    QString encoded_key = sought_key;
    QUrl::encode(encoded_key);
    QString encoded_value = new_value;
    QUrl::encode(encoded_value);
    fields.push_back(QString("%1=%2").arg(encoded_key).arg(encoded_value));
  }

  QUrl url_copy = url;
  url_copy.setQuery(fields.join("&"));
  return url_copy.encodedPathAndQuery();
}




QString getQueryValue(const QUrl &url, const QString &sought_key)
{
  QStringList fields = QStringList::split("&", url.query());

  QRegExp key_value_re("^([^=]+)\\=(.*)$");
  FOREACH(first, fields, QStringList)
  {
    if (!key_value_re.exactMatch(*first))
      throw tRuntimeError("Trouble extracting query field");
    QString key = key_value_re.cap(1);
    QString value = key_value_re.cap(2);

    key = decodeUrlString(key);
    value = decodeUrlString(value);

    if (key == sought_key)
      return value;
  }
  throw tHttpException(400, QString("Required key %1 not in query"). arg(sought_key));
}




int getQueryInteger(const QUrl &url, const QString &sought_key)
{
  bool my_ok;
  int result = getQueryValue(url, sought_key).toInt(&my_ok);
  if (!my_ok)
    throw tHttpException(400, QString("Invalid numeral '%1'"). arg(getQueryValue(url, sought_key)));
  return result;
}




QString getOptionalQueryValue(const QUrl &url, const QString &sought_key)
{
  try
  {
    return getQueryValue(url, sought_key);
  }
  catch (...)
  {
    return QString::null;
  }
}




int getOptionalQueryInteger(const QUrl &url, const QString &sought_key, int default_value, bool *ok = NULL)
{
  try
  {
    bool my_ok;
    int result = getQueryValue(url, sought_key).toInt(&my_ok);
    if (!my_ok)
    {
      if (ok) 
	*ok = false;
      return default_value;
    }
    if (ok) 
      *ok = true;
    return result;
  }
  catch (...)
  {
    if (ok) 
      *ok = false;
    return default_value;
  }
}




bool getAscending(const QUrl &url)
{
  QString sort_ascending_str = getOptionalQueryValue(url, "sort_ascending");

  return sort_ascending_str == "1" || sort_ascending_str.isNull();
}




void plaintextResponse(QSocket *&socket, const QString &str)
{
  tHttpResponseHeader header(200, "OK", 1, 1);
  header.setContentType("text/plain; charset=UTF-8");
  header.setValue("Cache-Control", "no-cache");

  tHttpResponseSender *sender = new tHttpStringResponseSender(str.utf8());
  sender->setHeader(header);
  sender->kickOff(socket);
}




void htmlResponse(QSocket *&socket, const QString &str)
{
  tHttpResponseHeader header(200, "OK", 1, 1);
  header.setContentType("text/html; charset=UTF-8");
  header.setValue("Cache-Control", "no-cache");

  tHttpResponseSender *sender = new tHttpStringResponseSender(str.utf8());
  sender->setHeader(header);
  sender->kickOff(socket);
}




void getSongSelection(const QUrl &url, tSongList &list)
{
  list.clear();

  QString type = getQueryValue(url, "selection_type");
  if (type == "criterion")
  {
    tPlaylist set;
    set.setSongCollection(&tProgramBase::database().SongCollection);
    set.setCriterion(getQueryValue(url, "criterion"));
    set.reevaluateCriterion();

    QString sort_by = getOptionalQueryValue(url, "sort_by");
    bool ascending = getAscending(url);

    try
    {
      tSongField sort_field = getFieldFromIdentifier(sort_by);
      set.sortBy(sort_field, 
          tProgramBase::preferences().SortingPreferences.SecondarySortField[sort_field],
          tProgramBase::preferences().SortingPreferences.TertiarySortField[sort_field],
          ascending);
    }
    catch (...) { }

    set.render(list);
  }
  else if (type == "current_playlist")
  {
    vector<tFilename> songs;
    if (tProgramBase::preferences().HttpLocalPlayEnabled)
    {
      tPlayer &player = tProgramBase::preferences().Player;
      player.getPlayList(songs);
      FOREACH_CONST(first, songs, vector<tFilename>)
      {
	tSong *song = tProgramBase::database().SongCollection.getByFilename(*first);
	if (song)
	  list.push_back(song);
      }
    }
  }
  else if (type == "playlist")
  {
    if (tProgramBase::database().playlistTree())
    {
      tPlaylistNode *node = tProgramBase::database().playlistTree()->resolve(
	  getQueryValue(url, "playlist_name"));
      if (node)
	node->data()->render(list);
      else
	throw tHttpException(404, QString("Playlist '%1' not found"). arg(type));
    }
  }
  else if (type == "current")
  {
    tPlayer &player = tProgramBase::preferences().Player;

    tSong *song = tProgramBase::database().SongCollection.getByFilename(
	player.currentFilename());
    if (song)
      list.push_back(song);
    else
      throw tHttpException(406, QString("No current song at this point"). arg(type));
  }
  else if (type == "autodj")
  {
    tSearchSongSet set;
    set.setSongCollection(&tProgramBase::database().SongCollection);
    set.setCriterion("~all");
    set.reevaluateCriterion();

    tAutoDJ dj(tProgramBase::preferences().AutoDJPreferences, &set);
    dj.selectSongs(list, 
	getOptionalQueryInteger(url, "song_count", 20));
  }
  else
    throw tHttpException(400, QString("Invalid selection type '%1'"). arg(type));
}




// tHttpFileResponseSender ----------------------------------------------------
tHttpFileResponseSender::tHttpFileResponseSender(ifstream *file)
: File(file), SizeOfSentDocument(0)
{
  File->seekg(0, ios_base::end);
  TotalSize = File->tellg();
  File->seekg(0, ios_base::beg);
}




void tHttpFileResponseSender::setHeader(tHttpResponseHeader &header)
{
  header.setValue("Content-Length", QString::number(TotalSize));
  super::setHeader(header);
}




void tHttpFileResponseSender::readyToSend()
{
  while (SizeOfSentDocument < TotalSize)
  {
    char buffer[ 1024 * 256 ];
    File->seekg(SizeOfSentDocument, ios_base::beg);
    int read_bytes = File->readsome(buffer, sizeof(buffer));
    int written_bytes = Socket->writeBlock(buffer, read_bytes);
    SizeOfSentDocument += written_bytes;
    if (Socket->bytesToWrite() != 0)
    {
      // We've saturated the buffer for now, QSocket will call back.
      // We do this to keep from shoving the whole multi-megabyte
      // song onto the heap (through QSocket's buffer)
      return;
    }
    if (written_bytes == 0)
    {
      // We've saturated the buffer for now, QSocket can't take any more.
      return;
    }
  }
  Socket->close();
}




// tHttpBufferResponseSender --------------------------------------------------
tHttpBufferResponseSender::tHttpBufferResponseSender(char const *buffer, unsigned length)
  : Buffer(buffer), BufferLength(length), SizeOfSentDocument(0)
{
}




void tHttpBufferResponseSender::setHeader(tHttpResponseHeader &header)
{
  header.setValue("Content-Length", QString::number(BufferLength));
  super::setHeader(header);
}




void tHttpBufferResponseSender::readyToSend()
{
  while (SizeOfSentDocument < BufferLength)
  {
    int bytes_to_be_written = min(BufferLength - SizeOfSentDocument, 16u*1024u);
    int written_bytes = Socket->writeBlock(
	Buffer + SizeOfSentDocument, bytes_to_be_written);
    SizeOfSentDocument += written_bytes;
    if (Socket->bytesToWrite() != 0)
    {
      // we've saturated the buffer for now, QSocket will call back
      // we do this to keep from shoving the whole multi-megabyte
      // song onto the heap (through QSocket's buffer)
      return;
    }
    if (written_bytes == 0)
    {
      // we've saturated the buffer for now, QSocket can't take any more
      return;
    }
  }
  Socket->close();
}




// tStreamingHttpResponder ----------------------------------------------------
bool tStreamingHttpResponder::canHandle(const QUrl &url, QHttpRequestHeader &header)
{
  if (url.path() == "/madman/download")
    return true;
  if (url.path() == "/madman/streaming")
    return true;
  return false;
}




void tStreamingHttpResponder::handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket)
{
  bool download = url.path() == "/madman/download";

  int unique_id = getQueryInteger(url, "uniqueid");

  tSong *song = tProgramBase::database().SongCollection.getByUniqueId(unique_id);
  if (!song)
    throw tHttpException(404, QString("Unique ID %1 not found in database"). arg(unique_id));

  tHttpResponseHeader response_header(200, "OK", 1, 1);
  
  if (download)
  {
    response_header.setContentType("application/octet-stream");
    response_header.setValue("Content-Disposition", 
	QString("attachment; filename=\"%1\"").
	arg(QString::fromUtf8(song->filenameOnly().c_str())));
  }
  else
    response_header.setContentType(song->mimeType());

  ifstream *file = new ifstream(song->filename().c_str(), ios_base::in);
  if (file->bad())
  {
    delete file;
    throw tHttpException(404, QString("File '%1' not found"). arg(
          QString::fromUtf8(song->filename().c_str())));
  }

  tHttpResponseSender *sender = new tHttpFileResponseSender(file);
  sender->setHeader(response_header);
  sender->kickOff(socket);
}




// tStaticDataHttpResponder ---------------------------------------------------
bool tStaticDataHttpResponder::canHandle(const QUrl &url, QHttpRequestHeader &header)
{
  const QString prefix = "/madman/static/";
  return url.path().left(prefix.length()) == prefix;
}




void tStaticDataHttpResponder::handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket)
{
  const QString prefix = "/madman/static/";
  QString filename = url.path().mid(prefix.length());

  if (filename.isNull())
    throw tHttpException(403, QString("The contents of this virtual directory cannot be listed"));

  const char *content = getFile(filename.latin1()) ;
  if (content == NULL)
    throw tHttpException(404, QString("File '%1' not found"). arg(filename));

  unsigned size = getFileSize(filename.latin1());
  const char *mime_type = getFileType(filename.latin1());

  tHttpResponseHeader response_header(200, "OK", 1, 1);
  response_header.setContentType(mime_type);
  response_header.setValue("Content-Length", QString::number(size));
  
  tHttpResponseSender *sender = new tHttpBufferResponseSender(content, size);
  sender->setHeader(response_header);
  sender->kickOff(socket);
}




// tBrowseHttpResponder -------------------------------------------------------
bool tBrowseHttpResponder::canHandle(const QUrl &url, QHttpRequestHeader &header)
{
  if (url.path() == "/" || url.path() == "")
    return true;
  const QString prefix = "/madman/browse/";
  return url.path().left(prefix.length()) == prefix;
}




namespace 
{
  QString generateNavigation(const QUrl &url, int start, int count, int total_count)
  {
    QString result = "";

    if (start == 0)
      result += QString("<img src=\"/madman/static/back_gray.png\">");
    else
    {
      result += QString("<a href=\"%1\"><img src=\"/madman/static/back.png\"></a>")
	.arg(replaceQueryValue(url, "start", QString::number(max(0, start - count))));
    }
    if(start + count >= total_count)
      result += QString("<img src=\"/madman/static/forward_gray.png\">");
    else
    {
      result += QString("<a href=\"%1\"><img src=\"/madman/static/forward.png\"></a>")
	.arg(replaceQueryValue(url, "start", QString::number(start + count)));
    }

    int pages = 0; 
    int start_page_index = max(0, start - 10 * count); 
    int page_number = start_page_index / count + 1;

    if (start_page_index != 0)
    {
      result += QString("<a href=\"%2\">%1</a> \n")
	.arg(1)
	.arg(replaceQueryValue(url, "start", QString::number(0)));
      result += " ... ";
    }

    while (pages < 20 && start_page_index < total_count)
    {
      if (start_page_index == start)
	result += QString("%1 ").arg(page_number);
      else
	result += QString("<a href=\"%2\">%1</a> \n")
	  .arg(page_number)
	  .arg(replaceQueryValue(url, "start", QString::number(start_page_index)));

      start_page_index += count;
      pages++;
      page_number++;
    }

    if (start_page_index < total_count)
    {
      result +=" ... ";
      start_page_index = total_count - total_count % count;
      page_number = start_page_index / count + 1;
      if (start_page_index == total_count && total_count > count)
	start_page_index -= count;
      result += QString("<a href=\"%2\">%1</a> \n")
	.arg(page_number)
	.arg(replaceQueryValue(url, "start", QString::number(start_page_index)));
    }
    
    return result;
  }
}




QString tBrowseHttpResponder::getTitleCell(const QUrl &url, const QString &human_readable, const QString &machine_readable)
{
  QString arrow = "";
  QString begin_link = "";
  QString end_link = "";
  if (getOptionalQueryValue(url, "sort_by") == machine_readable)
  {
    bool ascending = getAscending(url);

    begin_link = QString("<a href=\"%1\">")
      .arg(replaceQueryValue(url, "sort_ascending", !ascending ? "1" : "0"));
    end_link = "</a>";
    arrow = QString("<img src=\"/madman/static/%1.png\"> ")
      .arg(ascending ? "down" : "up");
  }
  else
  {
    begin_link = QString("<a href=\"%1\">")
      .arg(replaceQueryValue(url, "sort_by", machine_readable));
    end_link = "</a>";
  }

  return QString("<th>%1%4%2%3</td>")
    .arg(arrow)
    .arg(human_readable)
    .arg(end_link)
    // begin_link contains a URL (which may contain %x sequences)
    // and hence must be inserted last
    .arg(begin_link);
}




QString tBrowseHttpResponder::getTitleRow(const QUrl &url)
{
  QString result;
  result += QString("<th></th>");
  result += getTitleCell(url, "Artist", "artist");
  result += getTitleCell(url, "Title", "title");
  result += getTitleCell(url, "Album", "album");
  result += getTitleCell(url, "Genre", "genre");

  return QString("<tr>%1</tr>").arg(result);
}




QString tBrowseHttpResponder::getSongRow(tSong *song, bool bright)
{
  QString columns;
  QString special_functions;

  if (tProgramBase::preferences().HttpDownloadsEnabled)
  {
    special_functions += QString("<a href=\"/madman/streaming?uniqueid=%1\">"
	"<img src=\"/madman/static/stream.png\" title=\"Stream\">"
	"</a>").arg(song->uniqueId());
    special_functions += QString("<a href=\"/madman/download?uniqueid=%1\">"
	"<img src=\"/madman/static/save.png\" title=\"Download\">"
	"</a>").arg(song->uniqueId());
  }

  if (tProgramBase::preferences().HttpLocalPlayEnabled)
  {
    special_functions += QString("<a href=\"/madman/scripting/play_now?selection_type=criterion&criterion=~uniqueid(%1)\">"
	"<img src=\"/madman/static/play.png\" title=\"Play now\">"
	"</a>").arg(song->uniqueId());
    special_functions += QString("<a href=\"/madman/scripting/play_next?selection_type=criterion&criterion=~uniqueid(%1)\">"
	"<img src=\"/madman/static/play_next.png\" title=\"Play next\"\">"
	"</a>").arg(song->uniqueId());
    special_functions += QString("<a href=\"/madman/scripting/play_eventually?selection_type=criterion&criterion=~uniqueid(%1)\">"
	"<img src=\"/madman/static/play_eventually.png\" title=\"Play eventually\">"
	"</a>").arg(song->uniqueId());
  }

  special_functions += QString("<a href=\"/madman/browse/info?uniqueid=%1\">"
      "<img src=\"/madman/static/info.png\" title=\"More information\">"
      "</a>").arg(song->uniqueId());

  columns += QString("<td>%1</td>").arg(special_functions);

  columns += QString("<td>%1</td>").arg(song->artist());
  columns += QString("<td>%1</td>").arg(song->title());
  columns += QString("<td>%1</td>").arg(song->album());
  columns += QString("<td>%1</td>").arg(song->genre());

  return QString("<tr class=\"%2\">%1</tr>\n")
    .arg(columns)
    .arg(bright ? QString("brightline") : QString("darkline"));
}




QString tBrowseHttpResponder::generateMenu(const QUrl &url)
{
  QString result;

  result +=
    "<a href=\"/\">Search</a> "
    "<a href=\"/madman/browse/list?selection_type=current_playlist\">Playlist</a> ";
  
  if (tProgramBase::preferences().HttpLocalPlayEnabled)
  {
    result +=
      "Server: "
      "<a href=\"/madman/scripting/play\">Play</a> "
      "<a href=\"/madman/scripting/pause\">Pause</a> "
      "<a href=\"/madman/scripting/stop\">Stop</a> "
      "<a href=\"/madman/scripting/next\">Next</a> "
      "<a href=\"/madman/scripting/previous\">Previous</a> &middot;"
      "<a href=\"/madman/scripting/play_eventually?selection_type=autodj\">AutoDJ</a> "
      "<br>";
  }

  if (tProgramBase::preferences().HttpDownloadsEnabled)
  {
    result +=
      "Streaming: "
      "<a href=\"/madman/scripting/get_as_playlist?selection_type=autodj\">AutoDJ</a>";
  }

  return result;
}




void tBrowseHttpResponder::handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket)
{
  if (url.path() == "/" || url.path() == "")
  {
    // redirect to browser
    tHttpResponseHeader response_header(307, "Temporary Redirect", 1, 1);

    response_header.setContentType("text/html; charset=UTF-8");
    response_header.setValue("Location", "/madman/browse/list?selection_type=criterion&criterion=%7eall&start=0&count=50&sort_by=artist");

    tHttpResponseSender *sender = new tHttpStringResponseSender("Redirecting...");
    sender->setHeader(response_header);
    sender->kickOff(socket);
    return;
  }

  if (!tProgramBase::preferences().HttpBrowsingEnabled)
    throw tHttpException(403, QString("Browsing is forbidden in this madman instance."));

  QString body;

  if (url.path() == "/madman/browse/list")
  {
    tSongList list;
    getSongSelection(url, list);

    // pick up and verify start and count
    tSongList::iterator first = list.begin(), last = list.end();

    unsigned start_int = 0, count_int = list.size();

    QString start = getOptionalQueryValue(url, "start");
    if (!start.isNull())
    {
      bool ok;
      start_int = start.toUInt(&ok);
      if (!ok)
	throw tHttpException(400, QString("Invalid numeral %1 in query string"). arg(start));

      if (start_int > list.size())
	start_int = list.size();

      first = list.begin() + start_int;

      QString count = getOptionalQueryValue(url, "count");
      if (!count.isNull())
      {
	bool ok;
	count_int = count.toUInt(&ok);
	if (!ok)
	  throw tHttpException(400, QString("Invalid numeral %1 in query string"). arg(count));

	if (start_int + count_int > list.size())
	  last = list.end();
	else
	  last = first + count_int;
      }
      else
	count_int = list.size() - start_int;
    }

    // generate table
    QString table_contents;
    bool bright = false;
    while (first != last)
    {
      table_contents += getSongRow(*first, bright);
      first++;
      bright = !bright;
    }

    // make body
    if (getQueryValue(url, "selection_type") == "criterion")
    {
      body += QString(
	  "<form method=\"GET\">\n"
	  "  <input type=\"hidden\" name=\"selection_type\" value=\"criterion\">\n"
	  "  <input type=\"hidden\" name=\"sort_by\" value=\"%2\">\n"
	  "  <input type=\"hidden\" name=\"sort_ascending\" value=\"%4\">\n"
	  "  <input type=\"hidden\" name=\"start\" value=\"0\">\n"
	  "  Search for:"
	  "  <input type=\"text\" name=\"criterion\" value=\"%1\">\n"
	  "  <input type=\"text\" name=\"count\" value=\"%3\"> per page\n"
	  "  <input type=\"submit\" value=\"Search\"><p>\n"
	  "</form>\n")
	.arg(getQueryValue(url, "criterion").replace("\"", "&quot;"))
	.arg(getQueryValue(url, "sort_by"))
	.arg(getQueryValue(url, "count"))
	.arg(getAscending(url) ? 1 : 0);
    }

    body += generateNavigation(url, start_int, count_int, list.size()) + "<p>";
    body += "<table class=\"songtable\">"
      + getTitleRow(url)
      + table_contents
      + "</table>";
  }
  else if (url.path() == "/madman/browse/info")
  {
    bool darkline = false;
    QString info;

    tUniqueId unique_id = getQueryInteger(url, "uniqueid");

    tSong *song = tProgramBase::database().SongCollection.getByUniqueId(unique_id);
    if (!song)
      throw tHttpException(404, QString("Invalid unique id %1 in query string"). arg(unique_id));

#define GENERATE_INFO_ROW(NAME, VALUE) \
    info += QString("<tr class=\"%2\"><td><b>" NAME "</b>&nbsp;&nbsp;&nbsp;</td><td>%1</td></tr>") \
      .arg((VALUE)) \
      .arg(darkline ? "darkline" : "brightline"); \
    darkline = !darkline;

    GENERATE_INFO_ROW("Artist", song->artist());
    GENERATE_INFO_ROW("Performer", song->performer());
    GENERATE_INFO_ROW("Title", song->title());
    GENERATE_INFO_ROW("Year", song->year());
    GENERATE_INFO_ROW("Genre", song->genre());
    GENERATE_INFO_ROW("Track number", song->trackNumber());

    int total_seconds = (int) song->duration();
    int seconds = total_seconds % 60;
    int minutes = total_seconds / 60;

    QString duration;
    duration.sprintf("%d:%02d", minutes, seconds);

    GENERATE_INFO_ROW("Duration", duration);

    QString rating;
    if (song->rating() == 0)
    {
      rating = "-";
    }
    else if (song->rating() > 0)
    {
      rating.fill('*', song->rating());
    }
    GENERATE_INFO_ROW("Rating", rating);
    GENERATE_INFO_ROW("Play Count", song->playCount());
    GENERATE_INFO_ROW("File size", QString("%1 MB").arg(float(song->fileSize()) / (1024 * 1024), 0, 'f', 2));

    body += "<table class=\"infotable\">" + info + "</table>";
  }
  else
    throw tHttpException(404, QString("This browsing subsection is undefined."));

  QString outer = getFile("outer.html");
  QString document = outer.arg("madman").arg(generateMenu(url)).arg(body);

  tHttpResponseHeader response_header(200, "OK", 1, 1);
  response_header.setContentType("text/html; charset=UTF-8");
  response_header.setValue("Cache-Control", "no-cache");

  tHttpResponseSender *sender = new tHttpStringResponseSender(document.utf8());
  sender->setHeader(response_header);
  sender->kickOff(socket);
}




// tScriptingHttpResponder ----------------------------------------------------
bool tScriptingHttpResponder::canHandle(const QUrl &url, QHttpRequestHeader &header)
{
  const QString prefix = "/madman/scripting/";
  return url.path().left(prefix.length()) == prefix;
}




namespace
{
  void enumeratePlaylistTree(tPlaylistNode *root, QDomDocument &doc, QDomElement &parent, int recurse_depth = -1)
  {
    QDomElement node_el = doc.createElement("node");
    node_el.setAttribute("name", root->name());
    node_el.setAttribute("criterion", root->data()->criterion());
    parent.appendChild(node_el);

    if (recurse_depth == 0)
      return;

    if (recurse_depth > 0)
      recurse_depth--;

    FOREACH(first, *root, tPlaylistNode)
      enumeratePlaylistTree(*first, doc, node_el, recurse_depth);
  }
}




void tScriptingHttpResponder::handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket)
{
  if (!tProgramBase::preferences().HttpScriptingEnabled)
    throw tHttpException(403, QString("Scripting access has been forbidden"));

#define CHECK_FOR_LOCAL_PLAYER_PERMISSION \
  if (!tProgramBase::preferences().HttpLocalPlayEnabled) \
    throw tHttpException(403, QString("Local player control has been forbidden"));
#define CHECK_FOR_WRITE_PERMISSION \
  if (!tProgramBase::preferences().HttpWriteScriptingEnabled) \
    throw tHttpException(403, QString("Write scripting access has been forbidden"));

  if (url.path() == "/madman/scripting/get_version")
  {
    plaintextResponse(socket, STRINGIFY(MADMAN_VERSION));
  }
  else if (url.path() == "/madman/scripting/get_complete_record")
  {
    if (!tProgramBase::preferences().HttpScriptingEnabled)
      throw tHttpException(403, QString("Scripting access has been forbidden"));

    tSongList list;
    getSongSelection(url, list);

    QString result;
    FOREACH_CONST(first, list, tSongList)
    {
      for (int i = 0; i < FIELD_COUNT; i++)
      {
	tSongField field = (tSongField) i;
	result += getFieldIdentifier(field) + "=" + (*first)->fieldText(field) + "\n";
      }
      result += "END_SONG\n";
    }
    plaintextResponse(socket, result);
  }
  else if (url.path() == "/madman/scripting/set_field")
  {
    CHECK_FOR_WRITE_PERMISSION;

    int unique_id = getQueryInteger(url, "uniqueid");

    tSong *song = tProgramBase::database().SongCollection.getByUniqueId(unique_id);
    if (!song)
      throw tHttpException(404, QString("Unique ID %1 not found in database"). arg(unique_id));

    song->setFieldText(
	getFieldFromIdentifier(getQueryValue(url, "field")),
	getQueryValue(url, "value"));

    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/quit")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    if (!tProgramBase::preferences().HttpWriteScriptingEnabled)
      throw tHttpException(403, QString("Write scripting access has been forbidden"));

    QSocket *copy_of_socket_pointer = socket;
    plaintextResponse(socket, "OK");
    copy_of_socket_pointer->flush();

    tProgramBase::quitApplication();
  }
  else if (url.path() == "/madman/scripting/get_as_playlist")
  {
    tSongList list;
    getSongSelection(url, list);

    QString result = "#EXTM3U\n";
    FOREACH_CONST(first, list, tSongList)
    {
      result += QString("#EXTINF:%1,%2 - %3\n")
	.arg(int((*first)->duration()))
	.arg((*first)->artist())
	.arg((*first)->title());
      result += QString("http://%1/madman/streaming?uniqueid=%2\n")
	.arg(header.value("Host"))
	.arg((*first)->uniqueId());
    }

    tHttpResponseHeader response_header(200, "OK", 1, 1);
    response_header.setContentType("application/x-m3u; charset=UTF-8");
    response_header.setValue("Cache-Control", "no-cache");
    response_header.setValue("Content-Disposition", "attachment; filename=playlist.m3u");

    tHttpResponseSender *sender = new tHttpStringResponseSender(result.utf8());
    sender->setHeader(response_header);
    sender->kickOff(socket);
  }
  else if (url.path() == "/madman/scripting/play_now")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tSongList list;
    getSongSelection(url, list);
    tProgramBase::preferences().Player.playNow(list);
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/play_next")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tSongList list;
    getSongSelection(url, list);
    tProgramBase::preferences().Player.playNext(list);
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/play_eventually")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tSongList list;
    getSongSelection(url, list);
    tProgramBase::preferences().Player.playEventually(list);
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/play")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tProgramBase::preferences().Player.play();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/stop")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tProgramBase::preferences().Player.stop();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/pause")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tProgramBase::preferences().Player.pause();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/next")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    tProgramBase::preferences().Player.skipForward();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/previous")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    tProgramBase::preferences().Player.skipBack();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/skip_to")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;

    bool ok;
    double seconds = getQueryValue(url, "seconds").toDouble(&ok);
    if (!ok)
      throw tHttpException(400, QString("Invalid seconds numeral"));

    tProgramBase::preferences().Player.skipToSeconds(seconds);
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/total_time")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    plaintextResponse(socket, QString::number(tProgramBase::preferences().Player.totalTime()));
  }
  else if (url.path() == "/madman/scripting/current_time")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    plaintextResponse(socket, QString::number(tProgramBase::preferences().Player.currentTime()));
  }
  else if (url.path() == "/madman/scripting/is_playing")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    plaintextResponse(socket, QString::number(tProgramBase::preferences().Player.isPlaying() ? 1 : 0));
  }
  else if (url.path() == "/madman/scripting/is_paused")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    plaintextResponse(socket, QString::number(tProgramBase::preferences().Player.isPaused() ? 1 : 0));
  }
  else if (url.path() == "/madman/scripting/clear_playlist")
  {
    CHECK_FOR_LOCAL_PLAYER_PERMISSION;
    tProgramBase::preferences().Player.clearPlaylist();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/playlist_tree/get_playlist_tree")
  {
    QDomDocument pltreexml;
    QDomElement doc_element = pltreexml.createElement("playlisttree");
    pltreexml.appendChild(doc_element);

    int recursion_depth = getOptionalQueryInteger(url, "depth", -1);

    tPlaylistNode *root_node = NULL;
    QString root_path = getOptionalQueryValue(url, "root");
    if (root_path.isNull())
      root_node = tProgramBase::database().playlistTree();
    else
      root_node = tProgramBase::database().playlistTree()->resolve(root_path);
    if (root_node == NULL)
      throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(root_path));

    enumeratePlaylistTree(root_node, pltreexml, doc_element, recursion_depth);

    plaintextResponse(socket, pltreexml.toString());
  }
  else if (url.path() == "/madman/scripting/playlist_tree/set_playlist_name")
  {
    CHECK_FOR_WRITE_PERMISSION;

    QString qualified_name = getQueryValue(url, "qualifiedname");
    QString new_name = getQueryValue(url, "newname");

    tPlaylistNode *node = tProgramBase::database().playlistTree();
    if (node)
      node = node->resolve(qualified_name);

    if (node == NULL)
      throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(qualified_name));

    node->setName(new_name);
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/playlist_tree/set_playlist_criterion")
  {
    CHECK_FOR_WRITE_PERMISSION;

    QString qualified_name = getQueryValue(url, "qualifiedname");
    QString criterion_text = getQueryValue(url, "criterion");

    tPlaylistNode *node = tProgramBase::database().playlistTree();
    if (node)
      node = node->resolve(qualified_name);

    if (node == NULL)
      throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(qualified_name));

    node->data()->setCriterion(criterion_text);
    node->data()->reevaluateCriterion();
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/playlist_tree/create_playlist")
  {
    CHECK_FOR_WRITE_PERMISSION;

    QString qualified_name = getQueryValue(url, "qualifiedparent");
    QString new_name = getQueryValue(url, "newname");

    tPlaylistNode *node = tProgramBase::database().playlistTree();
    if (node)
      node = node->resolve(qualified_name);

    auto_ptr<tPlaylistNode> new_node(new tPlaylistNode(&tProgramBase::database(), new tPlaylist()));
    new_node->data()->setSongCollection(&tProgramBase::database().SongCollection);
    new_node->setName(new_name);
    if (node == NULL)
    {
      if (tProgramBase::database().playlistTree() == NULL)
      {
	tProgramBase::database().setPlaylistTree(new_node.get());
	new_node.release();
      }
      else
	throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(qualified_name));
    }
    else
    {
      node->addChild(new_node.get());
      new_node.release();
    }
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/playlist_tree/delete_playlist")
  {
    CHECK_FOR_WRITE_PERMISSION;

    QString qualified_name = getQueryValue(url, "qualifiedname");

    tPlaylistNode *node = tProgramBase::database().playlistTree();
    if (node)
      node = node->resolve(qualified_name);

    if (node == NULL)
      throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(qualified_name));

    tPlaylistNode *parent = node->parent();
    if (parent)
    {
      parent->removeChild(node);
      delete node;
    }
    else
    {
      tProgramBase::database().setPlaylistTree(NULL);
      delete node;
    }
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/playlist_tree/add_song_to_playlist")
  {
    CHECK_FOR_WRITE_PERMISSION;

    QString qualified_name = getQueryValue(url, "qualifiedname");

    tPlaylistNode *node = tProgramBase::database().playlistTree();
    if (node)
      node = node->resolve(qualified_name);

    if (node == NULL)
      throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(qualified_name));

    tUniqueId unique_id = getQueryInteger(url, "uniqueid");

    tSong *song = tProgramBase::database().SongCollection.getByUniqueId(unique_id);
    if (!song)
      throw tHttpException(400, QString("Invalid unique id %1 in query string"). arg(unique_id));

    node->data()->add(song);
    plaintextResponse(socket, "OK");
  }
  else if (url.path() == "/madman/scripting/playlist_tree/remove_song_from_playlist")
  {
    CHECK_FOR_WRITE_PERMISSION;

    QString qualified_name = getQueryValue(url, "qualifiedname");

    tPlaylistNode *node = tProgramBase::database().playlistTree();
    if (node)
      node = node->resolve(qualified_name);

    if (node == NULL)
      throw tHttpException(404, QString("A playlist by the name of '%1' was not found.").arg(qualified_name));

    tUniqueId unique_id = getQueryInteger(url, "uniqueid");

    tSong *song = tProgramBase::database().SongCollection.getByUniqueId(unique_id);
    if (!song)
      throw tHttpException(400, QString("Invalid unique id %1 in query string"). arg(unique_id));

    node->data()->remove(song);
    plaintextResponse(socket, "OK");
  }
  else
    throw tHttpException(404, QString("This scripting command is undefined"));
}




// add functions --------------------------------------------------------------
void addResponders(tHttpDaemon *httpd)
{
  // downloads
  httpd->addResponder(new tStreamingHttpResponder);

  // browsing 
  httpd->addResponder(new tStaticDataHttpResponder);
  httpd->addResponder(new tBrowseHttpResponder);

  // scripting
  httpd->addResponder(new tScriptingHttpResponder);
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
