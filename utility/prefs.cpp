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




#include <qsettings.h>

#include <stdexcept>

#include "utility/player.h"
#include "utility/plugin.h"
#include "utility/prefs.h"
#include "database/song.h"




namespace
{
  void addAutoDJScoringRule(const QString &comment, const QString &criterion, int score, tAutoDJPreferences &prefs)
  {
    tAutoDJScoringRule adj;
    adj.Comment = comment;
    adj.Criterion = criterion;
    adj.ScoreAdjustment = score;
    prefs.ScoringRuleList.push_back(adj);
  }
}




// tAutoDJPreferences ---------------------------------------------------------
void tAutoDJPreferences::resetToDefault()
{
  ScoringRuleList.clear();
  appendDefault();
}




void tAutoDJPreferences::appendDefault()
{
  addAutoDJScoringRule("Baseline score", "~all", 5, *this);
  addAutoDJScoringRule("Unrated", "~unrated", 5, *this);
  addAutoDJScoringRule("Rated '*****'", "~rating(5)", 15, *this);
  addAutoDJScoringRule("Rated '****'", "~rating(4)", 12, *this);
  addAutoDJScoringRule("Rated '***'", "~rating(3)", 8, *this);
  addAutoDJScoringRule("Rated '**'", "~rating(2)", 5, *this);
  addAutoDJScoringRule("Rated '*'", "~rating(1)", 3, *this);
  addAutoDJScoringRule("Rated '-'", "~rating(0)", -10, *this);
  addAutoDJScoringRule("Unplayed", "~play_count(0)", 4, *this);
  addAutoDJScoringRule("Played within 24 hours", "~last_played_n_days_ago(<=1)", -10, *this);
  addAutoDJScoringRule("Played within 3 days", "~last_played_n_days_ago(<=3)&~last_played_n_days_ago(>1)", -5, *this);
  addAutoDJScoringRule("Played within a week", "~last_played_n_days_ago(<=7)&~last_played_n_days_ago(>3)", -3, *this);
  addAutoDJScoringRule("Not played for a month", "~last_played_n_days_ago(>=30)&~last_played_n_days_ago(<60)", 3, *this);
  addAutoDJScoringRule("Not played for two months", "~last_played_n_days_ago(>60)", 6, *this);
  addAutoDJScoringRule("Often played to the end", "~play_count(>=4)&~full_play_ratio(>0.5)", 5, *this);
  addAutoDJScoringRule("Rarely played to the end", "~play_count(>=4)&~full_play_ratio(<0.3)", -5, *this);
  addAutoDJScoringRule("Played more than three times", "~play_count(>=3)", 3, *this);
  addAutoDJScoringRule("Less than a week old", "~existed_for_days(<= 7)", 10, *this);
  addAutoDJScoringRule("Less than two weeks old", "~existed_for_days(<= 14)", 5, *this);
  addAutoDJScoringRule("Less than three weeks old", "~existed_for_days(<= 21)", 3, *this);

  BasedOnRulesetVersion = CurrentRulesetVersion;
}




// tSortingPreferences --------------------------------------------------------
void tSortingPreferences::resetToDefault()
{
  for (int i = 0; i < FIELD_COUNT;i++)
  {
    SecondarySortField[i] = FIELD_INVALID;
    TertiarySortField[i] = FIELD_INVALID;
  }

  SecondarySortField[FIELD_ARTIST] = FIELD_ALBUM;
  TertiarySortField[FIELD_ARTIST] = FIELD_TRACKNUMBER;

  SecondarySortField[FIELD_TITLE] = FIELD_ARTIST;

  SecondarySortField[FIELD_ALBUM] = FIELD_TRACKNUMBER;

  SecondarySortField[FIELD_GENRE] = FIELD_ARTIST;

  SecondarySortField[FIELD_PATH] = FIELD_FILE;
}




// tPreferences ---------------------------------------------------------------
void tPreferences::save(QSettings &settings)
{
  settings.writeEntry("/madman/player", Player.name());

#define WRITE_BOOL(SETTING, VARIABLE) \
  settings.writeEntry("/madman/" SETTING, VARIABLE ? 1 : 0);

  QStringList plugin_dirs = stringvector2QStringList(PluginDirectories);
  settings.writeEntry("/madman/plugin_directories", plugin_dirs);
  WRITE_BOOL("remember_geometry", RememberGeometry);
  WRITE_BOOL("scan_at_startup", ScanAtStartup);
  WRITE_BOOL("collect_history", CollectHistory);
  WRITE_BOOL("enable_tray_icon", EnableSystemTrayIcon);
  WRITE_BOOL("enable_system_tray_icon", EnableSystemTrayIcon);
  WRITE_BOOL("minimize_to_system_tray", MinimizeToSystemTray);
  WRITE_BOOL("show_previous_song_info", ShowPreviousSongInfo);
  WRITE_BOOL("show_next_song_info", ShowNextSongInfo);

  settings.writeEntry("/madman/passive_popup_mode", (int) PassivePopupMode);
  settings.writeEntry("/madman/passive_popup_timeout", (int) PassivePopupTimeout);
  settings.writeEntry("/madman/number_of_backups_kept", BackupCount);

  WRITE_BOOL("http_daemon_enabled", HttpDaemonEnabled);
  settings.writeEntry("/madman/http_server_port", HttpDaemonPort);
  WRITE_BOOL("http_downloads_enabled", HttpDownloadsEnabled);
  WRITE_BOOL("http_scripting_enabled", HttpScriptingEnabled);
  WRITE_BOOL("http_write_scripting_enabled", HttpWriteScriptingEnabled);
  WRITE_BOOL("http_local_play_enabled", HttpLocalPlayEnabled);
  WRITE_BOOL("http_browsing_enabled", HttpBrowsingEnabled);
  WRITE_BOOL("http_restrict_to_localhost", HttpRestrictToLocalhost);

  settings.writeEntry("/madman/tray_tooltip_format_v2", TrayTooltipFormat );
  settings.writeEntry("/madman/passive_popup_format_v3", PassivePopupFormat);
  settings.writeEntry("/madman/caption_format_v2", CaptionFormat );
  settings.writeEntry("/madman/playlist_menu_format_v2", PlaylistMenuFormat );

  unsigned i = 0;
  settings.writeEntry("/madman/auto_dj/score_adjustment_count", (int) AutoDJPreferences.ScoringRuleList.size());
  FOREACH_CONST(first, AutoDJPreferences.ScoringRuleList, tAutoDJScoringRuleList)
  {
    const tAutoDJScoringRule &adj(*first);
    settings.writeEntry(QString("/madman/auto_dj/score_adjustment.%1.comment").arg(i), 
	adj.Comment);
    settings.writeEntry(QString("/madman/auto_dj/score_adjustment.%1.criterion").arg(i), 
	adj.Criterion);
    settings.writeEntry(QString("/madman/auto_dj/score_adjustment.%1.score").arg(i), 
	adj.ScoreAdjustment);
    i++;
  }
  settings.writeEntry("/madman/auto_dj/based_on_ruleset_version", 
                      (int) AutoDJPreferences.BasedOnRulesetVersion);


  settings.writeEntry("/madman/ui/sorting/field_count", FIELD_COUNT);
  for (int i = 0; i < FIELD_COUNT;i++)
  {
    settings.writeEntry(QString("/madman/ui/sorting/secondary_field.%1").arg(i), 
        SortingPreferences.SecondarySortField[i]);
    settings.writeEntry(QString("/madman/ui/sorting/tertiary_field.%1").arg(i), 
        SortingPreferences.TertiarySortField[i]);
  }

  settings.writeEntry("/madman/id3/read_preference" , (int) ID3ReadPreference);
  settings.writeEntry("/madman/tagless_extensions", TaglessExtensions);
}




void tPreferences::load(QSettings &settings)
{
  QString player_name = settings.readEntry("/madman/player");
  
  tPlayer * new_player = createPlayer(player_name);
  if (!new_player)
  {
    vector<QString> players;
    listPlayers(players);
    if (players.size())
      new_player = createPlayer(players[0]);
    if (!new_player)
      throw runtime_error("can't instantiate any players, giving up.");
  }
  setPlayer(new_player);

  bool ok;
  PluginDirectories = QStringList2stringvector(settings.readListEntry("/madman/plugin_directories", &ok));
  if (!ok)
  {
    PluginDirectories.clear();
    PluginDirectories.push_back(STRINGIFY(MADMAN_LIBDIR) "/madman/plugins");
  }

#define READ_BOOL(SETTING, VARIABLE, DEFAULT) \
  VARIABLE = settings.readNumEntry("/madman/" SETTING, DEFAULT);

  READ_BOOL("remember_geometry", RememberGeometry, 1);
  READ_BOOL("scan_at_startup", ScanAtStartup, 0);
  READ_BOOL("collect_history", CollectHistory, 1);
  READ_BOOL("enable_system_tray_icon", EnableSystemTrayIcon, 1);
  READ_BOOL("minimize_to_system_tray", MinimizeToSystemTray, 0);
  READ_BOOL("show_previous_song_info", ShowPreviousSongInfo, 0);
  READ_BOOL("show_next_song_info", ShowNextSongInfo, 0);

  PassivePopupMode = (tPassivePopupMode)
    settings.readNumEntry("/madman/passive_popup_mode", (int) POPUP_TOP_CENTER);
  PassivePopupTimeout = 
    settings.readNumEntry("/madman/passive_popup_timeout", 7500);
   
  BackupCount = settings.readNumEntry("/madman/number_of_backups_kept", 4);

  READ_BOOL("http_daemon_enabled", HttpDaemonEnabled, 0);
  HttpDaemonPort = settings.readNumEntry("/madman/http_server_port", 51533);
  READ_BOOL("http_downloads_enabled", HttpDownloadsEnabled, 1);
  READ_BOOL("http_scripting_enabled", HttpScriptingEnabled, 1);
  READ_BOOL("http_write_scripting_enabled", HttpWriteScriptingEnabled, 1);
  READ_BOOL("http_local_play_enabled", HttpLocalPlayEnabled, 1);
  READ_BOOL("http_browsing_enabled", HttpBrowsingEnabled, 1);
  READ_BOOL("http_restrict_to_localhost", HttpRestrictToLocalhost, 1);

  TrayTooltipFormat = settings.readEntry("/madman/tray_tooltip_format_v2", "%title_or_filename% - %artist%");
  PassivePopupFormat = settings.readEntry(
    "/madman/passive_popup_format_v3", 
    "%title_or_filename% (%duration%)%newline%%artist%");
  CaptionFormat = settings.readEntry("/madman/caption_format_v2", "%title_or_filename% - %artist% [%databasename%]");
  PlaylistMenuFormat = settings.readEntry("/madman/playlist_menu_format_v2", "%title_or_filename% - %artist% (%rating%)");

  // auto dj ------------------------------------------------------------------
  unsigned criteria_count = 
    settings.readNumEntry("/madman/auto_dj/score_adjustment_count", 0);
  if (criteria_count == 0)
    AutoDJPreferences.resetToDefault();
  else
  {
    for (unsigned i = 0; i < criteria_count; i++)
    {
      tAutoDJScoringRule adj;
      adj.Comment = settings.readEntry(
	  QString("/madman/auto_dj/score_adjustment.%1.comment").arg(i), "");
      adj.Criterion = settings.readEntry(
	  QString("/madman/auto_dj/score_adjustment.%1.criterion").arg(i), "");
      adj.ScoreAdjustment = settings.readNumEntry(
	  QString("/madman/auto_dj/score_adjustment.%1.score").arg(i), 0);

      if (adj.ScoreAdjustment < adj.ScorePlayNever)
	adj.ScoreAdjustment = adj.ScorePlayNever;
      if (adj.ScoreAdjustment > adj.ScoreMaximum)
	adj.ScoreAdjustment = adj.ScoreMaximum;
      AutoDJPreferences.ScoringRuleList.push_back(adj);
    }
  }
  AutoDJPreferences.BasedOnRulesetVersion = settings.readNumEntry(
    "/madman/auto_dj/based_on_ruleset_version", 
    tAutoDJPreferences::CurrentRulesetVersion - 1);

  // sorting ------------------------------------------------------------------
  int supposed_field_count = settings.readNumEntry("/madman/ui/sorting/field_count", -1);
  if (supposed_field_count != FIELD_COUNT)
    SortingPreferences.resetToDefault();
  else
  {
    for (int i = 0; i < FIELD_COUNT;i++)
    {
      SortingPreferences.SecondarySortField[i] = 
        (tSongField)
        settings.readNumEntry(QString("/madman/ui/sorting/secondary_field.%1").arg(i), FIELD_INVALID);
      SortingPreferences.TertiarySortField[i] = 
        (tSongField)
        settings.readNumEntry(QString("/madman/ui/sorting/tertiary_field.%1").arg(i), FIELD_INVALID);
    }
  }

  ID3ReadPreference = (tID3ReadPreference) settings.readNumEntry("/madman/id3/read_preference" , ID3_READ_PREFER_V2);
  TaglessExtensions = settings.readListEntry("/madman/tagless_extensions",&ok);
  if (!ok)
  {
    TaglessExtensions.clear();
    TaglessExtensions.push_back("wma");
    TaglessExtensions.push_back("fmr");
  }
}




void tPreferences::setPlayer(tPlayer *newplayer)
{
  Player.setBackend(newplayer);
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
