import httplib
import urllib
import codecs
import os
from string import *




class tRequestError(Exception):
  def __init__(self, response):
    self.Status = response.status
    self.Reason = response.reason
    self.Explanation = response.read()

  def __str__(self):
    return "Request failed: %d %s, %s" % (self.Status, 
	self.Reason, self.Explanation)

  def status(self):
    return self.Status

  def reason(self):
    return self.Reason

  def explanation(self):
    return self.Explanation




class tSelection:
  def getURLFields(self):
    return self.Fields

class tCriterionSelection(tSelection):
  def __init__(self, criterion, sort_by = None):
      fields = [ 
	("selection_type", "criterion"),
	("criterion", unicode(criterion).encode("utf8")) 
	]
      if sort_by is not None:
	fields.append(("sort_by", sort_by))

      self.Fields = urllib.urlencode(fields, 1)

class tUniqueIdSelection(tCriterionSelection):
  def __init__(self, uid):
    tCriterionSelection.__init__(self, "~uniqueid(%d)" % uid)

class tCurrentPlaylistSelection(tSelection):
  def __init__(self):
    fields = [ 
      ("selection_type", "current_playlist") 
      ]
    self.Fields = urllib.urlencode(fields, 1)

class tPlaylistSelection(tSelection):
  def __init__(self, name):
    fields = [ 
      ("selection_type", "playlist"),
      ("playlist_name", unicode(name).encode("utf8")) 
      ]
    self.Fields = urllib.urlencode(fields, 1)

class tCurrentSongSelection(tSelection):
  def __init__(self):
    fields = [ 
      ("selection_type", "current") 
      ]
    self.Fields = urllib.urlencode(fields, 1)

class tAutoDJSelection(tSelection):
  def __init__(self, number = 20):
    fields = [ 
      ("song_count", str(number)), 
      ("selection_type", "autodj") 
      ]
    self.Fields = urllib.urlencode(fields, 1)




class tRemote:
  def __init__(self, host = None, port = 51533):
    if host is None:
      self.Host = os.getenv("MADMANHOST", "localhost")
    else:
      self.Host = host
    self.Port = port
    self.decodeUtf8 = codecs.getdecoder("utf_8")
    self.getVersion()

  def request(self, url):
    conn = httplib.HTTPConnection(self.Host, self.Port)
    conn.request("GET", url)
    response = conn.getresponse()
    
    if response.status != 200:
      raise tRequestError,response
    (result, length) = self.decodeUtf8(response.read())
    return result

  def getVersion(self):
    return self.request("/madman/scripting/get_version")

  def quit(self):
    return self.request("/madman/scripting/quit")

  def play(self):
    return self.request("/madman/scripting/play")

  def pause(self):
    return self.request("/madman/scripting/pause")

  def stop(self):
    return self.request("/madman/scripting/stop")

  def next(self):
    return self.request("/madman/scripting/next")

  def previous(self):
    return self.request("/madman/scripting/previous")

  def skipTo(self, seconds):
    self.request("/madman/scripting/skip_to?seconds=%f" % seconds)

  def setField(self, uid, field, value):
    return self.request("/madman/scripting/set_field" \
                        "?uniqueid=%d&field=%s&value=%s" % \
                        (uid, field, urllib.quote_plus(unicode(value).encode("utf8"))))

  def getInfo(self, selection):
    data = self.request("/madman/scripting/get_complete_record?%s" % selection.getURLFields())
    lines = split(data, "\n")
    current_song = {}
    songs = []
    for line in lines:
      if len(line) == 0:
	continue
      if line == "END_SONG":
	songs.append(current_song)
	current_song = {}
      else:
	equals_index = find(line, "=")
	if equals_index == -1:
	  raise Exception, "Invalid line in song data from madman"
	key = line[ 0:equals_index ]
	value = line[ equals_index+1: ]
	current_song[ key ] = value
    return songs

  def playNow(self, selection):
    self.request("/madman/scripting/play_now?%s" % selection.getURLFields())

  def playNext(self, selection):
    self.request("/madman/scripting/play_next?%s" % selection.getURLFields())

  def playEventually(self, selection):
    self.request("/madman/scripting/play_eventually?%s" % selection.getURLFields())
  
  def totalTime(self):
    return float(self.request("/madman/scripting/total_time"))

  def currentTime(self):
    return float(self.request("/madman/scripting/current_time"))

  def isPlaying(self):
    return int(self.request("/madman/scripting/is_playing"))

  def isPaused(self):
    return int(self.request("/madman/scripting/is_paused"))

  def clearPlaylist(self):
    return self.request("/madman/scripting/clear_playlist")

  def getPlaylistTreeXML(self, root = None, depth = None):
    fields = [ ]

    if root is not None:
      fields.append(("root", unicode(root).encode("utf8")))
    if depth is not None:
      fields.append(("depth", str(depth)))

    return self.request("/madman/scripting/playlist_tree/get_playlist_tree?%s" % urllib.urlencode(fields, 1))

  def setPlaylistName(self, qual_name, new_name):
    fields = [
      ("qualifiedname", unicode(qual_name).encode("utf8")),
      ("newname", unicode(new_name).encode("utf8")) ]

    return self.request("/madman/scripting/playlist_tree/set_playlist_name?%s" % urllib.urlencode(fields, 1))

  def setPlaylistCriterion(self, qual_name, criterion):
    fields = [
      ("qualifiedname", unicode(qual_name).encode("utf8")),
      ("criterion", unicode(criterion).encode("utf8")) ]

    return self.request("/madman/scripting/playlist_tree/set_playlist_criterion?%s" % urllib.urlencode(fields, 1))

  def createPlaylist(self, qual_parent, new_name):
    fields = [
      ("qualifiedparent", unicode(qual_parent).encode("utf8")),
      ("newname", unicode(new_name).encode("utf8")) ]

    return self.request("/madman/scripting/playlist_tree/create_playlist?%s" % urllib.urlencode(fields, 1))

  def deletePlaylist(self, qual_name):
    fields = [
      ("qualifiedname", unicode(qual_name).encode("utf8")) ]

    return self.request("/madman/scripting/playlist_tree/delete_playlist?%s" % urllib.urlencode(fields, 1))

  def addSongToPlaylist(self, qual_name, uniqueid):
    fields = [
      ("qualifiedname", unicode(qual_name).encode("utf8")),
      ("uniqueid", uniqueid) ]

    return self.request("/madman/scripting/playlist_tree/add_song_to_playlist?%s" % urllib.urlencode(fields, 1))

  def removeSongFromPlaylist(self, qual_name, uniqueid):
    fields = [
      ("qualifiedname", unicode(qual_name).encode("utf8")),
      ("uniqueid", uniqueid) ]

    return self.request("/madman/scripting/playlist_tree/remove_song_from_playlist?%s" % urllib.urlencode(fields, 1))

