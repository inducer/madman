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




#ifndef HEADER_SEEN_PREFS_H
#define HEADER_SEEN_PREFS_H




#include <vector>
#include "utility/base.h"
#include "utility/player.h"
#include "database/song.h"




class tMainWindow;
class QSettings;




enum tID3ReadPreference
{
  ID3_READ_PREFER_V2 = 0,
  ID3_READ_PREFER_V1 = 1, 
};




enum tPassivePopupMode
{
  POPUP_FIRST = 0,

  POPUP_DISABLED = 0,

  POPUP_TOP_LEFT = 1,
  POPUP_TOP_CENTER = 2,
  POPUP_TOP_RIGHT = 3,

  POPUP_FIRST_BOTTOM = 4,

  POPUP_BOTTOM_LEFT = 4,
  POPUP_BOTTOM_CENTER = 5,
  POPUP_BOTTOM_RIGHT = 6,

  POPUP_COUNT = 6,
};

struct tAutoDJScoringRule
{
  QString Comment;
  QString Criterion;

  static const int ScorePlayNever = -10;
  static const int ScoreMinimum = -9;
  static const int ScoreMaximum = 30;
  int ScoreAdjustment;
};

typedef vector<tAutoDJScoringRule> tAutoDJScoringRuleList;

struct tAutoDJPreferences 
{
  tAutoDJScoringRuleList ScoringRuleList;

  void resetToDefault();
};

struct tSortingPreferences
{
  typedef tSongField tSortFieldList[FIELD_COUNT];
  tSortFieldList SecondarySortField;
  tSortFieldList TertiarySortField;

  void resetToDefault();
};

struct tPreferences
{
    bool RememberGeometry;
    bool ScanAtStartup;
    bool CollectHistory;
    bool EnableSystemTrayIcon;
    bool MinimizeToSystemTray;
    bool ShowPreviousSongInfo, ShowNextSongInfo;
    
    tPassivePopupMode PassivePopupMode;
    unsigned PassivePopupTimeout;

    int BackupCount;
    
    tPlayerFacade Player;
    tDirectoryList PluginDirectories;
    
    bool HttpDaemonEnabled;
    int HttpDaemonPort;
    bool HttpDownloadsEnabled;
    bool HttpScriptingEnabled;
    bool HttpWriteScriptingEnabled;
    bool HttpLocalPlayEnabled;
    bool HttpBrowsingEnabled;
    bool HttpRestrictToLocalhost;

    QString TrayTooltipFormat, PassivePopupFormat, 
      CaptionFormat, PlaylistMenuFormat;
    
    tAutoDJPreferences AutoDJPreferences;
    
    tSortingPreferences SortingPreferences;
    
    tID3ReadPreference ID3ReadPreference;
    
    QStringList TaglessExtensions;
    
    void save(QSettings &settings);
    void load(QSettings &settings);

    void setPlayer(tPlayer *newplayer);
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
