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




#ifndef HEADER_SEEN_MAINWIN_H
#define HEADER_SEEN_MAINWIN_H




#include <memory>
#include <qtimer.h>
#include <qlistview.h>
#include <qapplication.h>

#include "utility/prefs.h"

#include "database/program_base.h"
#include "database/database.h"

#include "designer/mainwin.h"

#include "httpd/httpd.h"
#include "httpd/madmanweb_external.h"

#include "ui/prefs_dialog.h"
#include "ui/song_set_view.h"
#include "ui/overview.h"




class tPlaylistNodeListViewItem;
class tProgress;
class TrayIcon;
class tMainWindow;




class tUIProgramBase : public tProgramBase
{
    tMainWindow *MainWindow;

  public:
    tUIProgramBase(tMainWindow *mw);

  protected:
    void internal_setStatus(const QString &status);
    void internal_quitApplication();
};




class tMainWindow : public tMainWindowBase
{
    Q_OBJECT;

    tUIProgramBase ProgramBase;

    tSearchSongSet SearchSongSet;
    tSearchViewManager SearchViewManager;
    tPlaylistViewManager PlaylistEditor;

    auto_ptr<tCriterion> SearchCriterion;

    QString CurrentFilename;
    bool FilenameValid;

    QTimer UpdateTimer;

    tProgress *CurrentSearchProgress;
    tHttpDaemon *HttpDaemon;
    TrayIcon *SystemTrayIcon;

    tOverviewManager OverviewManager;

    bool PreviouslyPaused, IgnoreRatingChanges, EnableAutoDJ;

    QPopupMenu *PlaylistButtonPopup;

  public:
    tMainWindow();
    virtual ~tMainWindow();

    void initialize(const QString &filename_to_open);

    void setDatabase(tDatabase *db);

    void closeEvent(QCloseEvent *event);

    void updateAll();

    void realizeSystemTrayIconSettings();
    void realizeHttpdSettings();

  public slots:
    void hideLists(bool hide);

    void rescan();
    void rescan(tDatabase *db);
    void rereadTags();
  
    void startNewDatabase();
    void fileNew();
    void fileOpen();
    void saveDatabase(const QString &name);
    void fileSave();
    bool fileSaveWithResult();
    void fileSaveAs();
    bool fileSaveAsWithResult();
    void showPreferences(tDatabase *db, int tab);
    void filePreferences();

    void enableAutoDJ(bool on);
    void doAutoDJ();
    void clearPlaylist();
    
    void helpAbout();
    void help();

    void playSearchResultNow();
    void playSearchResultNext();
    void playSearchResultEventually();

    void rateCurrentSong(int rating);
    void ratingBoxChanged(int index) { if (!IgnoreRatingChanges) rateCurrentSong(index-1); }
    void rateNone() { rateCurrentSong(-1); }
    void rate0() { rateCurrentSong(0); }
    void rate1() { rateCurrentSong(1); }
    void rate2() { rateCurrentSong(2); }
    void rate3() { rateCurrentSong(3); }
    void rate4() { rateCurrentSong(4); }
    void rate5() { rateCurrentSong(5); }

    void addPlaylist(tPlaylistNode *node, tPlaylistNode *parent);
    void addPlaylist();
    void bookmarkCurrentSearch();
    void importPlaylist();
    void duplicatePlaylist();
    void removePlaylist();
    void playPlaylist();

    void editMultilinePlaylistCriterion();
    void slotDropPlaylistNode(const QString &node, tPlaylistNode *onto);
    void renamePlaylist(QListViewItem *item, int col, const QString &text);
    void songSetSelectionChanged();
    void songSetCriterionChanged();
    void noticeSongSetChanged();

    void add(tSongList const &songlist);

    void followCurrentSongLink(const QString &href);

    void searchChanged();
    void searchChanged(const QString &text)
    {
      searchChanged();
    }

    void buildOverviewTree();
    void overviewSelectionChanged(QListViewItem *item);

    void highlightCurrentSong();
    void highlightSong(tSong *song);

    void playPause();
    void stop();
    void skipForward();
    void skipBack();
    void skipTo(int value);

    void trayIconClicked(const QPoint&, int);
    void trayIconWheelMoved(const QPoint&, int delta, Qt::Orientation orient);

    bool eventFilter(QObject *o, QEvent *e);

  private slots:
    void updatePlaylistTree();

  private:
    void updatePlaylistTree(bool keep_selection, bool keep_scroll);
    tPlaylistNode *nodeFromItem(QListViewItem *item);
    tPlaylistNode *currentNode();
    tPlaylist *currentPlaylist();
    void insertAtPosition(QListViewItem *parent, QListViewItem *item, tIndex pos);

    QListViewItem *createPlaylistNodeItem(tPlaylistNode *node, QListViewItem *parent,
	tPlaylistNode *selected_node, tPlaylistNodeListViewItem *&selected_item);

    void loadSplitterAppearance(QSplitter *splitter, QSettings &settings, QString const &where);
    void saveSplitterAppearance(QSplitter *splitter, QSettings &settings, QString const &where);

    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);

    void saveGeometry(const QString &name);
    void loadGeometry(const QString &name);

  private slots:
    void updatePlaylistButtonMenu();
    void updateWindowCaption();
    void updatePlayerStatus();
    void showSongPopup();
    void updateTrayIconStatus();
    void updateCurrentSongInfo();
    void doContinuousAutoDJ();

    void noticeSongModified(const tSong *song, tSongField field);
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
