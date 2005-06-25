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




#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <stdexcept>

#include <qtoolbutton.h>
#include <qtextbrowser.h>
#include <qheader.h>
#include <qprogressdialog.h>
#include <qdragobject.h>
#include <qapplication.h>
#include <qstatusbar.h>
#include <qfiledialog.h>
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qfile.h>
#include <qstring.h>
#include <qaction.h>
#include <qsplitter.h>
#include <qslider.h>
#include <qinputdialog.h>

#include "mainwin.h"
#include "utility/player.h"
#include "utility/progress.h"
#include "database/auto_dj.h"
#include "database/song_set.h"
#include "database/criterion.h"
#include "database/song_list_tools.h"
#include "designer/helpbrowser.h"
#include "ui/song_actions.h"
#include "ui/set_list_view.h"
#include "ui/multiline.h"
#include "ui/overview.h"
#include "ui/trayicon.h"
#include "ui/passive_popup.h"
#include "ui/stock.h"
#include "ui/progress_impl.h"
#include "ui/clickable_label.h"




// tUIProgramBase -------------------------------------------------------------
tUIProgramBase::tUIProgramBase(tMainWindow *mw)
  : MainWindow(mw)
{
}



void tUIProgramBase::internal_setStatus(const QString &status)
{
  MainWindow->statusBar()->message(status, 10000);
}




void tUIProgramBase::internal_quitApplication()
{
  QTimer::singleShot(0, MainWindow, SLOT(close()));
}




// tMainWindow ----------------------------------------------------------------
tMainWindow::tMainWindow()
  : ProgramBase(this),
    SearchViewManager(*lstAllSongs, "/madman/allsongsview"),
    PlaylistEditor(*lstSetEditor, "/madman/songseteditor"),
    CurrentSearchProgress(0),
    HttpDaemon(0), SystemTrayIcon(0),
    OverviewManager(*lstOverview),
    PreviouslyPaused(true), IgnoreRatingChanges(false),
    EnableAutoDJ(false),
    PlaylistButtonPopup(0)
{
}




void tMainWindow::initialize(const QString &filename_to_open)
{
  setIcon(getStockPixmap("madman.png"));

  setStockIcon(btnNewSet, "new.png");
  setStockIcon(btnCopySet, "copy.png");
  setStockIcon(btnBookmarkSet, "bookmark.png");
  setStockIcon(btnImportSet, "import.png");
  setStockIcon(btnRemoveSet, "remove.png");
  setStockIcon(btnPlaySet, "play.png");

  setStockIcon(btnBack, "back.png");
  setStockIcon(btnPlayPause, "play.png");
  setStockIcon(btnStop, "stop.png");
  setStockIcon(btnForward, "forward.png");
  setStockIcon(btnPlaylist, "history.png");

  setStockIcon(actionFileExit, "exit.png");
  setStockIcon(actionFileNew, "new.png");
  setStockIcon(actionFileOpen, "open.png");
  setStockIcon(actionFilePreferences, "preferences.png");
  setStockIcon(actionFileSave, "save.png");
  setStockIcon(actionFileSaveAs, "save_as.png");
  setStockIcon(actionHelp, "help.png");
  setStockIcon(actionHelpAbout, "about.png");

  setStockIcon(actionPlaybackPlayPause, "play.png");
  setStockIcon(actionPlaybackStop, "stop.png");
  setStockIcon(actionPlaybackSkipForward, "forward.png");
  setStockIcon(actionPlaybackSkipBack, "back.png");

  setStockIcon(actionRereadTags, "refresh.png");
  setStockIcon(actionRescan, "refresh.png");
  setStockIcon(actionUpdateOverview, "refresh.png");

  setStockIcon(actionSongHighlightCurrent, "home.png");
  setStockIcon(actionSongRate0, "rate0.png");
  setStockIcon(actionSongRate1, "rate1.png");
  setStockIcon(actionSongRate2, "rate2.png");
  setStockIcon(actionSongRate3, "rate3.png");
  setStockIcon(actionSongRate4, "rate4.png");
  setStockIcon(actionSongRate5, "rate5.png");

  setStockIcon(btnHelpAll, "help.png");
  setStockIcon(btnHelpSet, "help.png");
  setStockIcon(btnEditSetCriterion, "edit.png");
  setStockIcon(btnUpdateSetCriterion, "set.png");

  // load and realize prefs ---------------------------------------------------
  ProgramBase.preferences().load(ProgramBase.settings());
  ProgramBase.autoDJ().setPreferences(ProgramBase.preferences().AutoDJPreferences);
  SearchViewManager.loadListViewAppearance(ProgramBase.settings());
  PlaylistEditor.loadListViewAppearance(ProgramBase.settings());

  loadSplitterAppearance(splitter1, ProgramBase.settings(), "/madman/splitters/splitter1.state");
  loadSplitterAppearance(splitter2, ProgramBase.settings(), "/madman/splitters/splitter2.state");
  loadSplitterAppearance(splitter3, ProgramBase.settings(), "/madman/splitters/splitter3.state");

  // set up gui ---------------------------------------------------------------
  lstOverview->setSorting(-1);
  lstOverview->header()->hide();
  lstSets->header()->hide();

  lstSets->setAcceptDrops(true);
  lstSets->viewport()->setAcceptDrops(true);
  
  if (ProgramBase.preferences().RememberGeometry)
    loadGeometry("full");

  // various buttons ----------------------------------------------------------
  connect(btnNewSet, SIGNAL(clicked()), this, SLOT(addPlaylist()));
  connect(btnBookmarkSet, SIGNAL(clicked()), this, SLOT(bookmarkCurrentSearch()));
  connect(btnImportSet, SIGNAL(clicked()), this, SLOT(importPlaylist()));
  connect(btnCopySet, SIGNAL(clicked()), this, SLOT(duplicatePlaylist()));
  connect(btnRemoveSet, SIGNAL(clicked()), this, SLOT(removePlaylist()));
  connect(btnPlaySet, SIGNAL(clicked()), this, SLOT(playPlaylist()));

  connect(btnEditSetCriterion, SIGNAL(clicked()), this, SLOT(editMultilinePlaylistCriterion()));

  connect(btnHelpAll, SIGNAL(clicked()), this, SLOT(help()));
  connect(btnHelpSet, SIGNAL(clicked()), this, SLOT(help()));
  connect(btnEditSetCriterion, SIGNAL(clicked()),
	  this, SLOT(songSetCriterionChanged()));
  connect(btnUpdateSetCriterion, SIGNAL(clicked()),
	  this, SLOT(songSetCriterionChanged()));

  connect(btnBack, SIGNAL(clicked()),
	  this, SLOT(skipBack()));
  connect(btnPlayPause, SIGNAL(clicked()),
	  this, SLOT(playPause()));
  connect(btnStop, SIGNAL(clicked()),
	  this, SLOT(stop()));
  connect(btnForward, SIGNAL(clicked()),
	  this, SLOT(skipForward()));

  btnPlaylist->installEventFilter(this);

  connect(btnClearSearch, SIGNAL(clicked()),
          editSearch, SLOT(clear()));

  // various UI events --------------------------------------------------------
  connect(lstSets, SIGNAL(itemRenamed(QListViewItem *, int, const QString &)),
	  this, SLOT(renamePlaylist(QListViewItem *, int, const QString &)));
  connect(lstSets, SIGNAL(selectionChanged()),
	  this, SLOT(songSetSelectionChanged()));
  connect(editPlaylistCriterion, SIGNAL(returnPressed()),
	  this, SLOT(songSetCriterionChanged()));
  connect(&SearchViewManager, SIGNAL(add (const tSongList &)),
	  this, SLOT(add (const tSongList &)));
  connect(editSearch, SIGNAL(textChanged(const QString &)),
	  this, SLOT(searchChanged(const QString &)));
  connect(lstOverview, SIGNAL(selectionChanged(QListViewItem *)),
	  this, SLOT(overviewSelectionChanged(QListViewItem *)));
  connect(&SearchViewManager, SIGNAL(notifySearchChangeRequested(const QString &, bool)),
	  this, SLOT(processSearchChangeRequest(const QString &, bool)));
  connect(&PlaylistEditor, SIGNAL(notifySearchChangeRequested(const QString &, bool)),
	  this, SLOT(processSearchChangeRequest(const QString &, bool)));
  connect(&PlaylistEditor, SIGNAL(notifySongSetChanged()),
	  this, SLOT(noticeSongSetChanged()));

  connect(comboRating, SIGNAL(activated(int)),
      this, SLOT(ratingBoxChanged(int)));
  connect(labelCurrentSong, SIGNAL(linkClicked(const QString &)),
      this, SLOT(followCurrentSongLink(const QString &)));
  connect(checkHideLists, SIGNAL(toggled(bool)),
      this, SLOT(hideLists(bool)));

  editSearch->addShortCut(new tKeyboardShortCut(Key_F9, this, SLOT(playSearchResultNow())));
  editSearch->addShortCut(new tKeyboardShortCut(Key_F10, this, SLOT(playSearchResultNext())));
  editSearch->addShortCut(new tKeyboardShortCut(Key_F11, this, SLOT(playSearchResultEventually())));
  connect(editSearch, SIGNAL(returnPressed()), lstAllSongs, SLOT(setFocus()));

  // menu ---------------------------------------------------------------------
  connect(actionFileNew, SIGNAL(activated()), this, SLOT(fileNew()));
  connect(actionFileOpen, SIGNAL(activated()), this, SLOT(fileOpen()));
  connect(actionFileSave, SIGNAL(activated()), this, SLOT(fileSave()));
  connect(actionFileSaveAs, SIGNAL(activated()), this, SLOT(fileSaveAs()));
  connect(actionFilePreferences, SIGNAL(activated()), this, SLOT(filePreferences()));
  connect(actionFileExit, SIGNAL(activated()), this, SLOT(close()));
  connect(actionRescan, SIGNAL(activated()), this, SLOT(rescan()));
  connect(actionRereadTags, SIGNAL(activated()), 
      this, SLOT(rereadTags()));
  connect(actionUpdateOverview, SIGNAL(activated()), this, SLOT(buildOverviewTree()));

  connect(actionPlaybackEnableAutoDJ, SIGNAL(toggled(bool)),
	  this, SLOT(enableAutoDJ(bool)));
  connect(actionPlaybackDoAutoDJ, SIGNAL(activated()),
	  this, SLOT(doAutoDJ()));
  connect(actionPlaybackClearPlaylist, SIGNAL(activated()),
	  this, SLOT(clearPlaylist()));

  connect(actionHelpAbout, SIGNAL(activated()), this, SLOT(helpAbout()));
  connect(actionHelp, SIGNAL(activated()), this, SLOT(help()));

  connect(actionMinimizeWindow, SIGNAL(activated()), this, SLOT(showMinimized()));
  connect(actionRestoreWindow, SIGNAL(activated()), this, SLOT(showNormal()));
  connect(actionHideWindow, SIGNAL(activated()), this, SLOT(hide()));

  connectAutoDJSourceSignals();

  // player control -----------------------------------------------------------
  connect(actionPlaybackPlayPause, SIGNAL(activated()), this, SLOT(playPause()));
  connect(actionPlaybackStop, SIGNAL(activated()), this, SLOT(stop()));
  connect(actionPlaybackSkipForward, SIGNAL(activated()), this, SLOT(skipForward()));
  connect(actionPlaybackSkipBack, SIGNAL(activated()), this, SLOT(skipBack()));
  connect(sliderSongPosition, SIGNAL(sliderMoved(int)), this, SLOT(skipTo(int)));

  connect(actionSongHighlightCurrent, SIGNAL(activated()), this, SLOT(highlightCurrentSong()));

  // rating -------------------------------------------------------------------
  connect(actionSongRate0, SIGNAL(activated()), this, SLOT(rate0()));
  connect(actionSongRate1, SIGNAL(activated()), this, SLOT(rate1()));
  connect(actionSongRate2, SIGNAL(activated()), this, SLOT(rate2()));
  connect(actionSongRate3, SIGNAL(activated()), this, SLOT(rate3()));
  connect(actionSongRate4, SIGNAL(activated()), this, SLOT(rate4()));
  connect(actionSongRate5, SIGNAL(activated()), this, SLOT(rate5()));

  // set up song set tree -----------------------------------------------------
  lstSets->setAcceptDrops(true);
  lstSets->viewport()->setAcceptDrops(true);
  lstSets->setSorting(-1);

  // song change notifications and player state -------------------------------
  connect(&ProgramBase.preferences().Player, SIGNAL(stateChanged()), 
      this, SLOT(updatePlayerStatus()));
  connect(&ProgramBase, SIGNAL(songChanged()),
	  this, SLOT(updateTrayIconStatus()));
  connect(&ProgramBase, SIGNAL(songChanged()),
	  this, SLOT(updateCurrentSongInfo()));
  connect(&ProgramBase, SIGNAL(songChanged()),
	  this, SLOT(showSongPopup()));
  connect(&ProgramBase, SIGNAL(songChanged()),
	  this, SLOT(updateWindowCaption()));
  connect(&ProgramBase, SIGNAL(songChanged()),
	  this, SLOT(doContinuousAutoDJ()));

  connect(&UpdateTimer, SIGNAL(timeout()),
      this, SLOT(updatePlayerStatus()));
  UpdateTimer.start(500);

  // load everything ----------------------------------------------------------
  try
  {
    auto_ptr<tDatabase> new_db(new tDatabase);

    CurrentFilename = filename_to_open;
    FilenameValid = !filename_to_open.isNull();

    if (!FilenameValid)
    {
      CurrentFilename = ProgramBase.settings().readEntry("/madman/startup_file", QString::null, &FilenameValid);
    }

    if (!FilenameValid)
      throw runtime_error(QString2string(tr("No startup file found")));

    loadDBWithBreakLockInteraction(*new_db, CurrentFilename);

    setDatabase(new_db.release());
  }
  catch (runtime_error &ex)
  {
    if (ex.what() != QString2string(tr("No startup file found")))
    {
      QMessageBox::information(this, tr("madman"),
	  tr("Couldn't restore previously opened file:\n%1\nCreating a new file.").
	  arg(QString(ex.what())),
	  QMessageBox::Ok);
    }
    else
    {
      QMessageBox::information(this, tr("Welcome to madman."),
	  tr("Welcome to madman.\n\n"
	  "We sincerely hope that you're about to discover a powerful new way\n"
	  "to enjoy your music. madman is a powerful tool, and we advise you to take\n"
	  "the time to familiarize yourself with its many features--the madman website at\n"
	  "http://madman.sf.net has lots of useful hints.\n\n"
	  "First, you might want to tell madman where you keep your music."), 
	  QMessageBox::Ok);
    }

    startNewDatabase();
  }

  // set up lists -------------------------------------------------------------
  SearchSongSet.setCriterion("" );
  SearchSongSet.reevaluateCriterion();

  SearchViewManager.setSongSet(&SearchSongSet);

  SearchViewManager.setup();
  PlaylistEditor.setup();

  realizeSystemTrayIconSettings();
  realizeHttpdSettings();

  // set up bogus popup so the arrow appears
  PlaylistButtonPopup = new QPopupMenu(this);
  btnPlaylist->setPopup(PlaylistButtonPopup);

  // load auto dj status ------------------------------------------------------
  actionPlaybackEnableAutoDJ->setOn(
    ProgramBase.settings().readNumEntry("/madman/enable_auto_dj", 0) != 0);
}




tMainWindow::~tMainWindow()
{
  ProgramBase.settings().writeEntry("/madman/enable_auto_dj", EnableAutoDJ ? 1 : 0);

  if (splitter1->isShown())
    saveGeometry("full");
  else
    saveGeometry("small");

  ProgramBase.preferences().save(ProgramBase.settings());

  SearchViewManager.saveListViewAppearance(ProgramBase.settings());
  PlaylistEditor.saveListViewAppearance(ProgramBase.settings());

  saveSplitterAppearance(splitter1, ProgramBase.settings(), "/madman/splitters/splitter1.state");
  saveSplitterAppearance(splitter2, ProgramBase.settings(), "/madman/splitters/splitter2.state");
  saveSplitterAppearance(splitter3, ProgramBase.settings(), "/madman/splitters/splitter3.state");

  if (HttpDaemon)
    delete HttpDaemon;

  if (SystemTrayIcon)
    delete SystemTrayIcon;

  if (PlaylistButtonPopup)
    delete PlaylistButtonPopup;
}




void tMainWindow::setDatabase(tDatabase *db)
{
  disconnect(&ProgramBase.database().SongCollection, NULL, this, NULL);

  PlaylistEditor.setSongSet(NULL);
  SearchSongSet.setSongCollection(&db->SongCollection);
  ProgramBase.setDatabase(db);
  updateAll();

  connect(&ProgramBase.database().SongCollection, SIGNAL(notifySongModified(const tSong*, tSongField)),
	  this, SLOT(noticeSongModified(const tSong*, tSongField)));
  connect(&ProgramBase.database(), SIGNAL(notifyPlaylistTreeChanged()),
	  this, SLOT(updatePlaylistTree()));

  actionAutoDJSourceAll->setOn(true);
}




void tMainWindow::closeEvent(QCloseEvent *event)
{
  hide();
  if (fileSaveWithResult())
  {
    ProgramBase.settings().writeEntry("/madman/startup_file", CurrentFilename);
    tMainWindowBase::closeEvent(event);
  }
  else
  {
    if (QMessageBox::warning(this, tr("madman"),
	  tr("Failed to save your database. Do you still want to quit?"),
	  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
      tMainWindowBase::closeEvent(event);
    else
    {
      event->ignore();
      show();
    }
  }
}




void tMainWindow::updateAll()
{
  updatePlaylistTree(false, false);
  songSetSelectionChanged();
  buildOverviewTree();
  updateWindowCaption();
}




void tMainWindow::realizeSystemTrayIconSettings()
{
  if (ProgramBase.preferences().EnableSystemTrayIcon && !SystemTrayIcon)
  {
    SystemTrayIcon = new TrayIcon(this);

    updateTrayIconStatus();

    QPopupMenu *rating_menu = new QPopupMenu(this, "tray_rating_menu");
    actionSongRate0->addTo(rating_menu);
    actionSongRate1->addTo(rating_menu);
    actionSongRate2->addTo(rating_menu);
    actionSongRate3->addTo(rating_menu);
    actionSongRate4->addTo(rating_menu);
    actionSongRate5->addTo(rating_menu);

    QPopupMenu *mainwin_menu = new QPopupMenu(this, "tray_mainwin_menu");
    actionMinimizeWindow->addTo(mainwin_menu);
    actionRestoreWindow->addTo(mainwin_menu);
    actionHideWindow->addTo(mainwin_menu);

    QPopupMenu *tray_menu = new QPopupMenu(this, "tray_menu");

    actionPlaybackPlayPause->addTo(tray_menu);
    actionPlaybackStop->addTo(tray_menu);

    tray_menu->insertSeparator();

    tray_menu->insertItem(tr("&Rate currently playing song"), rating_menu);

    tray_menu->insertSeparator();

    actionPlaybackSkipForward->addTo(tray_menu);
    actionPlaybackSkipBack->addTo(tray_menu);

    tray_menu->insertSeparator();

    tray_menu->insertItem(tr("&Main window"), mainwin_menu);

    tray_menu->insertSeparator();

    actionFileExit->addTo(tray_menu);

    SystemTrayIcon->setPopup(tray_menu);

    SystemTrayIcon->show();

    connect(SystemTrayIcon, SIGNAL(clicked(const QPoint &, int)),
	  this, SLOT(trayIconClicked(const QPoint &, int)));
    connect(SystemTrayIcon, SIGNAL(wheelMoved(const QPoint &, int, Qt::Orientation)),
	  this, SLOT(trayIconWheelMoved(const QPoint &, int, Qt::Orientation)));
  }
  if (!ProgramBase.preferences().EnableSystemTrayIcon && SystemTrayIcon)
  {
    delete SystemTrayIcon;
    SystemTrayIcon = NULL;
  }
}




void tMainWindow::realizeHttpdSettings()
{
  if (HttpDaemon)
  {
    delete HttpDaemon;
    HttpDaemon = NULL;
  }

  if (ProgramBase.preferences().HttpDaemonEnabled)
  {
    try
    {
      HttpDaemon = new tHttpDaemon(ProgramBase.preferences().HttpDaemonPort, 
				   ProgramBase.preferences().HttpRestrictToLocalhost);

      addResponders(HttpDaemon);
    }
    catch (runtime_error &ex)
    {
      if (QMessageBox::information(this, tr("madman"),
	    tr("Could not start web server:\n%1\n"
	      "This could be because another instance of madman is running,\n"
	      "which is not a good idea. Stop now?").
	    arg(QString(ex.what())),
	    QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	throw;
    }
  }
}




// slots ----------------------------------------------------------------------
void tMainWindow::hideLists(bool hide)
{
  if (splitter1->isShown() != !hide)
  {
    if (hide)
      saveGeometry("full");
    else
      saveGeometry("small");

    splitter1->setShown(!hide);
    adjustSize();

    if (ProgramBase.preferences().RememberGeometry)
    {
      if (hide)
        loadGeometry("small");
      else
        loadGeometry("full");
    }
  }
}




void tMainWindow::rescan()
{
  rescan(&ProgramBase.database());
}




void tMainWindow::rescan(tDatabase *db)
{
  try
  {
    auto_ptr<tProgressDialog> progress(new tProgressDialog(this, false));
    db->SongCollection.scan(db->DirectoryList, progress.get());
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
	tr("Error while rescanning:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tMainWindow::rereadTags()
{
  try
  {
    auto_ptr<tProgressDialog> progress(new tProgressDialog(this, true));
    ProgramBase.database().SongCollection.rereadTags(progress.get());
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
	tr("Error rereading tags:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tMainWindow::startNewDatabase()
{
  FilenameValid = false;
  auto_ptr<tDatabase> new_db(new tDatabase);
  new_db->startNew();
  showPreferences(new_db.get(), 2);
  setDatabase(new_db.release());
}




void tMainWindow::fileNew()
{
  if (QMessageBox::warning(this, tr("madman"),
	tr("Are you sure you want to start over from scratch?"),
	QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    startNewDatabase();
}




void tMainWindow::fileOpen()
{
  if (QMessageBox::warning(this, tr("madman"),
	tr("Save database before continuing?"),
	QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
  {
    if (!fileSaveWithResult())
      return;
  }

  try
  {
    QString filename = QFileDialog::getOpenFileName(QString::null,
	tr("madman Database (*.mad)"),
	this);
    if (filename.isNull())
      return;

    PlaylistEditor.setSongSet(NULL);

    auto_ptr<tDatabase> new_db(new tDatabase);

    loadDBWithBreakLockInteraction(*new_db, filename);

    CurrentFilename = filename;
    FilenameValid = true;
    setDatabase(new_db.release());
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
	tr("Error opening the file.\nReason: %1").arg(string2QString(ex.what())),
	QMessageBox::Ok, 0);
  }
}




void tMainWindow::saveDatabase(const QString &name)
{
  QFileInfo file_info(name);
  QString dir_name = file_info.dirPath();
  QString trunk = file_info.baseName();
  QString ext = file_info.extension();

  QDir my_dir(dir_name);

  if (ext.length())
    ext = "." + ext;

  QString tempname = trunk+"-part"+ext;
  QString full_tempname = dir_name+"/"+tempname;

  auto_ptr<tProgressDialog> progress(new tProgressDialog(this, false));
  progress->setWhat(tr("Saving database..."));
  ProgramBase.database().save(full_tempname, progress.get());

  if (ProgramBase.preferences().BackupCount)
  {
    my_dir.remove((trunk+"-backup-%1").arg(ProgramBase.preferences().BackupCount) + ext);

    for (int dest_i = ProgramBase.preferences().BackupCount; dest_i > 1; dest_i--)
    {
      QString srcname =(trunk+"-backup-%1").arg(dest_i - 1) + ext;
      QString destname =(trunk+"-backup-%1").arg(dest_i) + ext;
      my_dir.rename(srcname, destname);
    }

    QString srcname = trunk + ext;
    QString destname =(trunk+"-backup-%1").arg(1) + ext;
    my_dir.rename(srcname, destname);
  }

  my_dir.rename(tempname, name);
}




void tMainWindow::fileSave()
{
  fileSaveWithResult();
}





bool tMainWindow::fileSaveWithResult()
{
  if (!FilenameValid)
  {
    return fileSaveAsWithResult();
  }

  try
  {
    saveDatabase(CurrentFilename);
    return true;
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
	tr("Error saving file '%1'").arg(string2QString(ex.what())),
	QMessageBox::Ok, 0);
    return false;
  }
}




void tMainWindow::fileSaveAs()
{
  fileSaveAsWithResult();
}




bool tMainWindow::fileSaveAsWithResult()
{
  try
  {
    QString filename;
    while (true)
    {
      filename = QFileDialog::getSaveFileName(QString::null, "madman Database (*.mad)", this);
      if (filename.isNull())
	return false;

      QFileInfo info(filename);
      if (info.exists())
      {
	int result = QMessageBox::warning(this, tr("Warning"), 
	      tr("The file '%1' exists. Overwrite?").arg(filename), 
	      QMessageBox::Yes, QMessageBox::No | QMessageBox::Default,
	      QMessageBox::Cancel);
	if (result == QMessageBox::Yes)
	  break;
	if (result == QMessageBox::Cancel)
	  return false;
      }
      else
	break;
    }
    saveDatabase(filename);
    CurrentFilename = filename;
    FilenameValid = true;
    updateWindowCaption();
    return true;
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
	tr("Error saving file '%1'").arg(string2QString(ex.what())),
	QMessageBox::Ok, 0);
    return false;
  }
}





void tMainWindow::showPreferences(tDatabase *db, int tab)
{
  pair<bool,bool> ok_rescan = editPreferences(this, ProgramBase.preferences(), db->DirectoryList, ProgramBase.settings(), tab);
  if (ok_rescan.first)
  {
    SearchViewManager.redisplay(false);
    PlaylistEditor.redisplay(false);

    SearchViewManager.setup();
    PlaylistEditor.setup();

    ProgramBase.autoDJ().setPreferences(ProgramBase.preferences().AutoDJPreferences);
    realizeSystemTrayIconSettings();
    realizeHttpdSettings();

    if (ok_rescan.second)
      rescan(db);
  }
}




void tMainWindow::filePreferences()
{
  showPreferences(&ProgramBase.database(), 0);
}




void tMainWindow::enableAutoDJ(bool on)
{
  EnableAutoDJ = on;
  if (EnableAutoDJ)
    doContinuousAutoDJ();
}




void tMainWindow::doAutoDJ()
{
  try
  {
    tSongList songs;
    ProgramBase.autoDJ().selectSongs(songs, 20);
    tProgramBase::preferences().Player.playEventually(songs);
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::setAutoDJSourceAll(bool on)
{
  auto_ptr<tSearchSongSet> adjss(new tSearchSongSet());
  adjss->setSongCollection(&ProgramBase.database().SongCollection);
  adjss->setCriterion("~all");
  adjss->reevaluateCriterion();
  ProgramBase.autoDJ().setSongSet(adjss.get());
  adjss.release();

  PreviousAutoDJSourceAction = actionAutoDJSourceAll;
}




void tMainWindow::setAutoDJSourceSearch(bool on)
{
  if (!on)
    return;

  bool ok;

  auto_ptr<tSearchSongSet> adjss(new tSearchSongSet());
  adjss->setSongCollection(&ProgramBase.database().SongCollection);

  while (true) 
  {
    try
    {
      QString search_str = QInputDialog::getText(
        tr("madman AutoDJ"),
        tr("Search expression"),
        QLineEdit::Normal,
        editSearch->text(),
        &ok);
      if (!ok)
      {
        // revert UI
        disconnectAutoDJSourceSignals();
        PreviousAutoDJSourceAction->setOn(true);
        connectAutoDJSourceSignals();
        return;
      }
      adjss->setCriterion(search_str);
      break;
    }
    catch (runtime_error &ex)
    {
      QMessageBox::warning(this, tr("madman"),
                           tr("Error in search expression:\n%1")
                           .arg(string2QString(ex.what())),
                           QMessageBox::Ok, 0);
    }
  }

  adjss->reevaluateCriterion();
  ProgramBase.autoDJ().setSongSet(adjss.get());
  adjss.release();

  PreviousAutoDJSourceAction = actionAutoDJSourceSearch;
}




void tMainWindow::setAutoDJSourcePlaylist(bool on)
{
  if (!on)
    return;

  tPlaylistNode *node = currentNode();
  if (!node)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("There is no currently selected playlist."),
                         QMessageBox::Ok, 0);

    // revert UI
    disconnectAutoDJSourceSignals();
    PreviousAutoDJSourceAction->setOn(true);
    connectAutoDJSourceSignals();
    return;
  }

  tSongList sl;
  node->data()->render(sl);
  if (sl.size() == 0)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("Can't pick an empty playlist as AutoDJ source."),
                         QMessageBox::Ok, 0);
    
    // revert UI
    disconnectAutoDJSourceSignals();
    PreviousAutoDJSourceAction->setOn(true);
    connectAutoDJSourceSignals();
    return;
  }

  ProgramBase.autoDJ().setSongSet(node->data()->duplicate());

  PreviousAutoDJSourceAction = actionAutoDJSourcePlaylist;
}




void tMainWindow::disconnectAutoDJSourceSignals()
{
  disconnect(actionAutoDJSourceAll, NULL, 
             this, NULL);
  disconnect(actionAutoDJSourceSearch, NULL, 
             this, NULL);
  disconnect(actionAutoDJSourcePlaylist, NULL, 
             this, NULL);
}




void tMainWindow::connectAutoDJSourceSignals()
{
  connect(actionAutoDJSourceAll, SIGNAL(toggled(bool)), 
          this, SLOT(setAutoDJSourceAll(bool)));
  connect(actionAutoDJSourceSearch, SIGNAL(toggled(bool)), 
          this, SLOT(setAutoDJSourceSearch(bool)));
  connect(actionAutoDJSourcePlaylist, SIGNAL(toggled(bool)), 
          this, SLOT(setAutoDJSourcePlaylist(bool)));
}




void tMainWindow::clearPlaylist()
{
  try
  {
    ProgramBase.preferences().Player.clearPlaylist();
    doContinuousAutoDJ();
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}



    
#include "designer/about.h"
void tMainWindow::helpAbout()
{
  tAboutDialog about;
  about.labelMain->setText(about.labelMain->text().arg(STRINGIFY(MADMAN_VERSION)));
  about.labelAbout->setText(about.labelAbout->text().arg(STRINGIFY(MADMAN_VERSION)));
  about.exec();
}




void tMainWindow::help()
{
  tHelpBrowser *hb = new tHelpBrowser(this);
  hb->browserHelp->setTextFormat(Qt::RichText);
  hb->browserHelp->setText(
      tr(
	"<h2>Table of contents</h2>"
	"1. <a href=\"#search\">Searching in madman</a><br>"
	"2. <a href=\"#faq\">Frequently Asked Questions</a><p>"
	"<a name=\"search\"><h2>Searching in madman</h2>"
	"Criteria can be combined via parentheses, <tt>|</tt> (or), <tt>&</tt> (and), "
	"<tt>!</tt> (not). "
	"Plain text will be interpreted as <tt>~any(...)</tt>. "
	"Unconnected criteria will automatically be connected by \"and\". "
	"Comments can be included if enclosed in <tt>{ ... }</tt>. "
	"<h3>Text criteria</h3>"
	"The following criteria exist:<p>"
	"<ul>"
	"<li><tt>~any(...)</tt>: selects all songs that contain ... in any field."
	"<li><tt>~artist(...)</tt>: selects all songs that contain ... in the artist field."
	"<li><tt>~performer(...)</tt>: selects all songs that contain ... in the performer field."
	"<li><tt>~album(...)</tt>: selects all songs that contain ... in the album field."
	"<li><tt>~title(...)</tt>: selects all songs that contain ... in the title field."
	"<li><tt>~genre(...)</tt>: selects all songs that contain ... in the genre field."
	"<li><tt>~filename(...)</tt>: selects all songs that contain ... in the filename field."
	"<li><tt>~pathname(...)</tt>: selects all songs that contain ... in the pathname field."
	"<li><tt>~filename_without_path(...)</tt>: selects all songs that contain ... in the filename field, "
	"stripped of the path."
	"</ul>"
	"Texts mentioned above as '...' can take several match modifiers:<p>"
	"<ul>"
	"<li><tt>re:...</tt> for a regular expression"
	"<li><tt>complete:...</tt> for a complete match"
	"<li><tt>substring:...</tt> for a substring match"
	"<li><tt>fuzzy:...</tt> for a fuzzy match"
	"<li><tt>startswith:...</tt> for prefix match"
	"</ul>"
	"Substring matching is assumed if nothing is specified.<p>"

	"<h3>Numeric criteria</h3>"
	"The following criteria exist:<p>"
	"<ul>"
	"<li><tt>~year(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a a year less/greater/.. than ..."
	"<li><tt>~track_number(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a a track number less/greater/.. than ..."
	"<li><tt>~rating(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a a rating less/greater/.. than ..."
	"<li><tt>~unrated</tt>: selects songs that are as yet unrated."
	"<li><tt>~play_count(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a a play count less/greater/.. than ..."
	"<li><tt>~full_play_count(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a full play count "
	"less/greater/.. than ..."
	"<li><tt>~partial_play_count(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a partial play count "
	"less/greater/.. than ..."
	"<li><tt>~existed_for_days(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have been in your collection "
	"for less/greater/.. than ... days"
	"<li><tt>~full_play_ratio(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects all songs that have a ratio "
	"of being played in full to the total play count less/greater/.. than ... "
	"(argument should be between 0.0 and 1.0) "
	"<li><tt>~last_played_n_days_ago(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects songs that were last played "
	"within a number of days less/greater/.. than ... (argument may be a non-integer)"
	"<li><tt>~uniqueid(&lt;|&gt;|&lt;=|=|&gt;=...)</tt>: selects songs that have a database key unique ID "
	"less/greater/.. than ..."
	"</ul>"
	"Equal matching is assumed if nothing is specified.<p>"

	"<h3>Criteria examples</h3>"
	"<ul>"
	"<li> <tt>man moon</tt>: Songs that contain both the words \"man\" and \"moon\" "
	  "in one of their fields."
	"<li> <tt>man|moon</tt>: Songs that contain the word \"man\" or the word \"moon\" "
	  "in one of their fields."
	"<li> <tt>\"3 doors down\"</tt>: Just like on Google, quoting ensures that "
	  "you only get songs that contain the phrase \"3 doors down\" in succession."
	"<li> <tt>~artist(complete:creed)</tt>: That's how you say that you want "
	  "Creed, but not Creedence Clearwater Revival."
	"</ul>"
	"And here are some smart ideas for playlists that you could create:"
	"<ul>"
	"<li> <b>Absolute favorites:</b> <tt>~rating(&gt;=4)</tt>"
	"<li> <b>Can't stand to listen to them:</b> <tt>~full_play_ratio(&lt;0.3)&amp;~play_count(&gt;3)</tt>"
	"<li> <b>Always finish listening to them:</b> <tt>~full_play_ratio(&gt;0.5)&amp;~play_count(&gt;3)</tt>"
	"<li> <b>Never listened:</b> <tt>~play_count(=0)</tt>"
	"<li> <b>Love songs:</b> <tt>~title(love)|~album(love)</tt>"
	"</ul>"
	"<a name=\"faq\"><h2>FAQ</h2>"
	"<b>Q. Why can't I drag the column headers like I used to?</b><p>"
	"Just hold Control while dragging."
      ));
  hb->show();
}




void tMainWindow::playSearchResultNow()
{
  try
  {
    tSongList songs;
    SearchSongSet.render(songs);
    
    // don't let people add their whole database--takes too long
    if (songs.size() > 200)
      songs.erase(songs.begin()+200, songs.end());
    
    ProgramBase.preferences().Player.playNow(songs);
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::playSearchResultNext()
{
  try
  {
    tSongList songs;
    SearchSongSet.render(songs);
    
    // don't let people add their whole database--takes too long
    if (songs.size() > 200)
      songs.erase(songs.begin()+200, songs.end());
    
    ProgramBase.preferences().Player.playNext(songs);
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::playSearchResultEventually()
{
  try
  {
    tSongList songs;
    SearchSongSet.render(songs);
    
    // don't let people add their whole database--takes too long
    if (songs.size() > 200)
      songs.erase(songs.begin()+200, songs.end());
    
    ProgramBase.preferences().Player.playEventually(songs);
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::rateCurrentSong(int rating)
{
  try
  {
    tFilename song_file = ProgramBase.preferences().Player.currentFilename();

    tSong *new_song = ProgramBase.database().SongCollection.getByFilename(song_file);
    if (new_song)
    {
      if (new_song->rating() == rating)
        new_song->setRating(-1);
      else
        new_song->setRating(rating);
    }
    else
      QMessageBox::warning(this, tr("madman"),
                           tr("Currently playing song is not in database. Sorry."), QMessageBox::Ok, QMessageBox::NoButton);
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}



void tMainWindow::addPlaylist(tPlaylistNode *node, tPlaylistNode *parent)
{
  if (ProgramBase.database().playlistTree() == NULL)
    ProgramBase.database().setPlaylistTree(node);
  else if (parent == NULL)
    ProgramBase.database().playlistTree()->addChild(node);
  else
    parent->addChild(node);

  updatePlaylistTree(true, true);
}




void tMainWindow::addPlaylist()
{
  if (lstSets->isRenaming())
    return;

  try 
  {
    tPlaylistNode *parent = currentNode();

    auto_ptr<tPlaylistNode> node(new tPlaylistNode(&ProgramBase.database(), new tPlaylist()));
    node->data()->setSongCollection(&ProgramBase.database().SongCollection);
    bool successful = false;
    unsigned index = 1;
    while (!successful)
    {
      try 
      {
	node->setName(tr("New Playlist %1").arg(index));
	addPlaylist(node.get(), parent);
	successful = true;
      }
      catch (...)
      { }
      index++;
    }
    node.release();
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("Can't create playlist:\n%1").arg(ex.what()), 
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tMainWindow::bookmarkCurrentSearch()
{
  if (lstSets->isRenaming())
    return;

  try 
  {
    tPlaylistNode *parent = currentNode();

    auto_ptr<tPlaylistNode> node(new tPlaylistNode(&ProgramBase.database(), new tPlaylist()));
    node->data()->setCriterion(editSearch->text());
    node->data()->setSongCollection(&ProgramBase.database().SongCollection);

    bool successful = false;
    unsigned index = 1;
    while (!successful)
    {
      try 
      {
	node->setName(tr("Bookmarked Search %1").arg(index));
	addPlaylist(node.get(), parent);
	successful = true;
      }
      catch (...)
      { }
      index++;
    }
    node.release();
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("Can't create playlist from search:\n%1").arg(ex.what()), 
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tMainWindow::importPlaylist()
{
  if (lstSets->isRenaming())
    return;

  try
  {
    tPlaylistNode *parent = currentNode();

    auto_ptr<tPlaylistNode> node(new tPlaylistNode(&ProgramBase.database(), new tPlaylist()));
    node->data()->setSongCollection(&ProgramBase.database().SongCollection);

    QString fn = QFileDialog::getOpenFileName(QString::null,
	qApp->translate("importM3UIntoPlaylist", "Playlist (*.m3u)"),
	qApp->mainWidget());
    if (fn.isNull())
      return;

    importM3UIntoPlaylist(node->data(), fn);

    QFileInfo info(fn);
    node->setName(info.fileName());
    addPlaylist(node.get(), parent);
    node.release();
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("Can't import playlist:\n%1").arg(ex.what()), 
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tMainWindow::duplicatePlaylist()
{
  if (lstSets->isRenaming())
    return;

  tPlaylistNode *current_node = currentNode();
  if (!current_node)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("There is no currently selected playlist."),
                         QMessageBox::Ok, 0);
    return;
  }

  try 
  {
    auto_ptr<tPlaylistNode> new_node(new tPlaylistNode(&ProgramBase.database(), new tPlaylist));
    new_node->setName(tr("Copy of %1").arg(current_node->name()));
    *(new_node->data()) = *(current_node->data());
    current_node->addChild(new_node.get());
    new_node.release();

    updatePlaylistTree(true, true);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("Can't duplicate playlist:\n%1").arg(ex.what()), 
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tMainWindow::removePlaylist()
{
  if (lstSets->isRenaming())
    return;

  tPlaylistNode *node = currentNode();
  if (!node)
  {
    QMessageBox::warning(this, tr("madman"),
                         tr("There is no currently selected playlist."),
                         QMessageBox::Ok, 0);
    return;
  }

  if (node->begin() != node->end())
  {
    if (QMessageBox::warning(this, tr("Warning"), 
                             tr("This playlist has children. Do you still want to delete it?"), 
                             QMessageBox::Yes, 
                             QMessageBox::No | QMessageBox::Default) == QMessageBox::No)
      return ;
  }

  // delete from structure
  if (node->parent() == NULL)
    ProgramBase.database().setPlaylistTree(NULL);
  else
    node->parent()->removeChild(node);
  delete node;

  PlaylistEditor.setSongSet(NULL);

  updatePlaylistTree(false, true);
}




void tMainWindow::playPlaylist()
{
  try
  {
    if (PlaylistEditor.songSet())
    {
      tSongList songs;
      PlaylistEditor.songSet()->render(songs);
      ProgramBase.preferences().Player.playNow(songs);
    }
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::editMultilinePlaylistCriterion()
{
  if (!lstSets->currentItem())
    return ;
  tPlaylistNode *node = currentNode();
  QString criterion = node->data()->criterion();
  while (multilineEdit(tr("madman"), 
	tr("Criterion for dynamic playlist"), criterion, this))
  {
    try
    {
      if (node->data()->criterion() != criterion)
      {
	node->data()->setCriterion(criterion);
	node->data()->reevaluateCriterion();
	songSetSelectionChanged();
      }
      break;
    }
    catch (exception &ex)
    {
      QMessageBox::warning(this, tr("madman"),
                           tr("Error in Criterion: %1").arg(string2QString(ex.what())),
                           QMessageBox::Ok, 0);
    }
  }
}




void tMainWindow::slotDropPlaylistNode(const QString &node_name, tPlaylistNode *onto)
{
  if (lstSets->isRenaming())
    return;

  tPlaylistNode *node = ProgramBase.database().playlistTree()->resolve(node_name);
  if (node == NULL)
    return;

  // this also catches the case when root is being dragged away
  if (node == onto || onto->hasParent(node))
    return;

  // by now, node can't be root, so it has a parent.
  node->parent()->removeChild(node);
  onto->insertChild(node, onto->begin());

  updatePlaylistTree(false, true);
}




void tMainWindow::renamePlaylist(QListViewItem *item, int col, const QString &text)
{
  try
  {
    nodeFromItem(item)->setName(text);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(this, tr("madman"), string2QString(ex.what()), QMessageBox::Ok, 0);
  }
  updatePlaylistTree(true, true);
}




void tMainWindow::songSetSelectionChanged()
{
  tPlaylistNode *node = currentNode();
  PlaylistEditor.setSongSet(node ? node->data() : NULL);

  if (node && node->data())
  {
    tSongList list;
    node->data()->render(list);
    statusBar()->message(
	stringifySongListSummary(list), 5000);
  }
}




void tMainWindow::songSetCriterionChanged()
{
  tPlaylistNode *node = currentNode();

  if (node)
  {
    tPlaylist *song_set = node->data();
    try {
      QString newcrit = editPlaylistCriterion->text();
      if (song_set->criterion() != newcrit)
      {
	song_set->setCriterion(newcrit);
	song_set->reevaluateCriterion();

	{
	  tSongList list;
	  song_set->render(list);
	  statusBar()->message(
	      stringifySongListSummary(list), 5000);
	}
      }
    }
    catch (exception &ex)
    {
      QMessageBox::warning(this, tr("madman"),
	tr("Error in Criterion: %1").arg(string2QString(ex.what())),
	QMessageBox::Ok, 0);
    }
  }
}




void tMainWindow::noticeSongSetChanged()
{
  tSongSet *song_set = PlaylistEditor.songSet();
  if (song_set)
  {
    editPlaylistCriterion->setEnabled(true);
    editPlaylistCriterion->setText(song_set->criterion());
  }
  else
  {
    editPlaylistCriterion->setEnabled(false);
    editPlaylistCriterion->setText("");
  }
}




void tMainWindow::add(const tSongList &songlist)
{
  FOREACH_CONST(first, songlist, tSongList)
    PlaylistEditor.playlist()->add(*first);
}




namespace 
{
  tOverviewItem *findItemInOverview(QListView *lstOverview, const QString &criterion)
  {
    tOverviewItem *ov_root = dynamic_cast<tOverviewItem *>(lstOverview->firstChild());

    while (ov_root)
    {
      tOverviewItem *result = ov_root->findItemByCriterion(criterion);
      if (result)
        return result;
      ov_root = dynamic_cast<tOverviewItem *>(ov_root->nextSibling());
    }
    return NULL;
  }
}




void tMainWindow::followCurrentSongLink(const QString &href)
{
  try
  {
    tSong *current_song = NULL;
    current_song = ProgramBase.database().SongCollection.getByFilename(
      ProgramBase.preferences().Player.currentFilename());
    hideLists(false);

    tOverviewItem *item_to_select = NULL;

    if (href == "title")
    {
      if (SearchSongSet.criterion() != "")
        editSearch->setText("");

      lstAllSongs->setFocus();
      SearchViewManager.highlightCurrentSong();
      checkHideLists->setChecked(FALSE);
    }
    else if (href == "artist" && current_song)
      item_to_select = findItemInOverview(lstOverview,
                                          getArtistCriterion(current_song->artist()));
    else if (href == "album" && current_song)
      item_to_select = findItemInOverview(lstOverview,
                                          getAlbumCriterion(current_song->album()));

    if (item_to_select)
    {
      lstOverview->ensureItemVisible(item_to_select);
      lstOverview->setCurrentItem(item_to_select);
      if (editSearch->text() != item_to_select->criterion())
        editSearch->setText(item_to_select->criterion());
      checkHideLists->setChecked(FALSE);
    }
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::searchChanged()
{
  try 
  {
    QString new_crit = editSearch->text();
    if (SearchSongSet.criterion() != new_crit)
    {
      if (CurrentSearchProgress)
	CurrentSearchProgress->cancel();

      SearchSongSet.setCriterion(new_crit);

      tCancellableStatusBarProgress progress(statusBar());
      CurrentSearchProgress = &progress;

      SearchSongSet.reevaluateCriterion(&progress);

      if (CurrentSearchProgress == &progress)
	CurrentSearchProgress = NULL;
    }

    tSongList list;
    SearchSongSet.render(list);
    statusBar()->message(
	stringifySongListSummary(list), 5000);
  }
  catch (exception &ex)
  {
    statusBar()->message(tr("Error in expression: %1.").arg(string2QString(ex.what())), 2000);
  }
}




void tMainWindow::buildOverviewTree()
{
  lstOverview->clear();

  QListViewItem *paths = new QListViewItem(lstOverview, tr("By Path"));
  QListViewItem *last_item = NULL;

  FOREACH(first, ProgramBase.database().DirectoryList, tDirectoryList)
    last_item = new tPathOverviewItem(QString::fromUtf8(first->c_str()), paths, last_item,
				      true);

  new tGenreOverviewItem(ProgramBase.database(), lstOverview, NULL, tr("By Genre"), QString::null);
  new tAlbumOverviewItem(ProgramBase.database(), lstOverview, NULL, tr("By Album"), QString::null, 
                         true);
  new tArtistsOverviewItem(ProgramBase.database(), lstOverview, NULL, tr("By Artist"), QString::null, 
                           true);
  
  new tOverviewItem(lstOverview, NULL, tr("All"), "");
}




void tMainWindow::overviewSelectionChanged(QListViewItem *item)
{
  tOverviewItem *ov_item = dynamic_cast< tOverviewItem * >(item);
  if (ov_item)
  {
    if (ov_item->criterion() != QString::null)
      editSearch->setText(ov_item->criterion());
  }
}




void tMainWindow::highlightCurrentSong()
{
  if (ProgramBase.currentSong())
    highlightSong(ProgramBase.currentSong());
}




void tMainWindow::highlightSong(tSong *song)
{
  if (SearchSongSet.criterion() != "")
    editSearch->setText("");

  lstAllSongs->setFocus();
  SearchViewManager.highlight(song);
}




void tMainWindow::playPause()
{
  try
  {
    tPlayer &player = ProgramBase.preferences().Player;
    player.ensureValidStatus();
    if (player.isPaused() || !player.isPlaying())
    {
      if (player.getPlayListLength() == 0)
      {
        // OK, we have a bad case of stupid user.^W^Wnew user.
        tSongList songs;
        SearchSongSet.render(songs);

        if (songs.size() != 0)
        {
          // Phew. There's something we can do as a last resort.
          playSearchResultNow();
        }
        else
        {
          // Not very smooth.
          QMessageBox::information(this,
                                   tr("madman"),
                                   tr("madman presently doesn't "
                                      "know what it is supposed to "
                                      "play since there is no "
                                      "search result.\nMaybe you "
                                      "need to add songs to the "
                                      "database or try a different "
                                      "search."),
                                   QMessageBox::Ok);
        }
      }
      else
        player.play();
    }
    else
      player.pause();
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}
void tMainWindow::stop()
{
  try
  {
    ProgramBase.preferences().Player.stop();
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}
void tMainWindow::skipForward()
{
  try
  {
    ProgramBase.preferences().Player.skipForward();
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}
void tMainWindow::skipBack()
{
  try
  {
    ProgramBase.preferences().Player.skipBack();
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}
void tMainWindow::skipTo(int value)
{
  try
  {
    ProgramBase.preferences().Player.skipTo(float(value) / 100);
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
}




void tMainWindow::trayIconClicked(const QPoint &where, int button)
{
  if (isShown())
    hide();
  else
    showNormal();
}




void tMainWindow::trayIconWheelMoved(const QPoint&, int delta, Orientation orient)
{
  if (orient == Qt::Vertical)
  {
    if (delta > 0)
      skipBack();
    else if (delta < 0)
      skipForward();
  }
}




bool tMainWindow::eventFilter(QObject *o, QEvent *e)
{
  if (o == btnPlaylist && e->type() == QEvent::MouseButtonPress)
    updatePlaylistButtonMenu();
  return false;
}




// helpers --------------------------------------------------------------------
void tMainWindow::updatePlaylistTree()
{
  updatePlaylistTree(false, false);
}




void tMainWindow::updatePlaylistTree(bool keep_selection, bool keep_scroll)
{
  int contents_x = lstSets->contentsX();
  int contents_y = lstSets->contentsY();
  tPlaylistNodeListViewItem *selected_item = NULL;
  if (keep_selection)
    selected_item = dynamic_cast<tPlaylistNodeListViewItem *>(lstSets->currentItem());
  tPlaylistNode *selected_node = NULL;
  if (selected_item)
    selected_node = selected_item->node();

  lstSets->clear();
  if (ProgramBase.database().playlistTree())
    lstSets->insertItem(
	createPlaylistNodeItem(ProgramBase.database().playlistTree(), NULL, 
	  selected_node, selected_item));

  if (keep_scroll)
    lstSets->setContentsPos(contents_x, contents_y);
  if (keep_selection && selected_item)
    lstSets->setSelected(selected_item, true);
}




tPlaylistNode *tMainWindow::nodeFromItem(QListViewItem *item)
{
  tPlaylistNodeListViewItem *cast_item = dynamic_cast<tPlaylistNodeListViewItem *>(item);
  if (cast_item)
    return cast_item->node();
  else
    return NULL;
}





tPlaylistNode *tMainWindow::currentNode()
{
  QListViewItem *current_item = lstSets->currentItem();
  if (current_item == NULL)
    return NULL;
  else
    return nodeFromItem(current_item);
}




tPlaylist *tMainWindow::currentPlaylist()
{
  tPlaylistNode * node = currentNode();
  if (node)
    return node->data();
  else
    return NULL;
}




void tMainWindow::insertAtPosition(QListViewItem *parent, QListViewItem *item, tIndex pos)
{
  parent->insertItem(item);
  if (pos != 0)
  {
    QListViewItem * after = item;
    while (pos--)
      after = after->nextSibling();
    item->moveItem(after);
  }
}




QListViewItem *tMainWindow::createPlaylistNodeItem(tPlaylistNode *node, QListViewItem *parent,
    tPlaylistNode *selected_node, tPlaylistNodeListViewItem *&selected_item)
{
  tPlaylistNodeListViewItem *result;
  if (parent)
    result = new tPlaylistNodeListViewItem(parent, node);
  else
    result = new tPlaylistNodeListViewItem(lstSets, node);

  connect(result, SIGNAL(dropNode(const QString &,tPlaylistNode *)),
      this, SLOT(slotDropPlaylistNode(const QString &,tPlaylistNode *)));

  if (node == selected_node)
    selected_item = result;

  tPlaylistNode::iterator first = node->begin(), last = node->end();
  if (first != last)
    do
    {
      last--;
      createPlaylistNodeItem(*last, result, selected_node, selected_item);
    }
    while (first != last);

  lstSets->setOpen(result, true);
  return result;
}




void tMainWindow::loadSplitterAppearance(QSplitter *splitter, QSettings &settings, QString const &where)
{
  QString representation;
  representation = settings.readEntry(where);
  {
    QTextStream stream(&representation, IO_ReadOnly);
    stream >> *splitter;
  }
}




void tMainWindow::saveSplitterAppearance(QSplitter *splitter, QSettings &settings, QString const &where)
{
  QString representation;
  {
    QTextStream stream(&representation, IO_WriteOnly);
    stream << *splitter;
  }
  settings.writeEntry(where, representation);
}




void tMainWindow::showEvent(QShowEvent *e)
{
  tMainWindowBase::showEvent(e);
}




void tMainWindow::hideEvent(QHideEvent *e)
{
  tMainWindowBase::hideEvent(e);
  if (isMinimized() && ProgramBase.preferences().EnableSystemTrayIcon && ProgramBase.preferences().MinimizeToSystemTray)
    hide();
}




void tMainWindow::saveGeometry(const QString &name)
{
  QPoint p = pos();
  QSize s = size();

  {
    QString temp;
    {
      QTextOStream os(&temp);
      os << p.x() << ' ' << p.y();
    }
    ProgramBase.settings().writeEntry(
        QString("/madman/window_geometry/%1/position").arg(name), temp);
  }

  {
    QString temp;
    {
      QTextOStream os(&temp);
      os << s.width() << ' ' << s.height();
    }
    ProgramBase.settings().writeEntry(
        QString("/madman/window_geometry/%1/size").arg(name), temp);
  }
}




void tMainWindow::loadGeometry(const QString &name)
{
  {
    QString temp = ProgramBase.settings().readEntry(
        QString("/madman/window_geometry/%1/position").arg(name));
    if (!temp.isNull())
    {
      int x,y;
      QTextIStream is(&temp);
      is >> x >> y;
      QPoint p(x,y);
      move(p);
    }
  }

  {
    QString temp = ProgramBase.settings().readEntry(
        QString("/madman/window_geometry/%1/size").arg(name));
    if (!temp.isNull())
    {
      int x,y;
      QTextIStream is(&temp);
      is >> x >> y;
      QSize s(x,y);
      resize(s);
    }
  }
}




void tMainWindow::updatePlaylistButtonMenu()
{
  if (PlaylistButtonPopup)
    delete PlaylistButtonPopup;
  PlaylistButtonPopup = new QPopupMenu(this);

  QPopupMenu *menu = PlaylistButtonPopup;

  actionPlaybackEnableAutoDJ->addTo(menu);
  agAutoDJSource->addTo(menu);
  actionPlaybackDoAutoDJ->addTo(menu);
  actionPlaybackClearPlaylist->addTo(menu);

  try
  {
    tPlayer &player = ProgramBase.preferences().Player;
    vector<tFilename> plist;
    player.getPlayList(plist);
    int playlist_index = player.getPlayListIndex();
    int start_index = max(0, playlist_index - 4);
    int end_index = min((int) plist.size(), playlist_index + 5);

    menu->insertSeparator();

    for (int i = start_index; i < end_index; i++)
    {
      tSong *song = ProgramBase.database().SongCollection.getByFilename(
        plist[i]);
      if (!song)
        continue;

      QPopupMenu *submenu = new QPopupMenu(menu);
      (new tSetPlaylistIndexAction(i, tr("&Play"), submenu))->addTo(submenu);
      tRemovePlaylistIndexAction *remove_action = new tRemovePlaylistIndexAction(
        i, tr("&Remove"), submenu);
      remove_action->addTo(submenu);
      connect(remove_action, SIGNAL(songRemoved()),
              this, SLOT(doContinuousAutoDJ()));
      (new tHighlightAction(this, song->uniqueId(), tr("&Show"), submenu))->addTo(submenu);
      submenu->insertSeparator();
      (new tRateAction(song->uniqueId(), -1, tr("Mark &unrated"), submenu))->addTo(submenu);
      (new tRateAction(song->uniqueId(), 0, tr("Rate - (&0)"), submenu))->addTo(submenu);
      (new tRateAction(song->uniqueId(), 1, tr("Rate * (&1)"), submenu))->addTo(submenu);
      (new tRateAction(song->uniqueId(), 2, tr("Rate ** (&2)"), submenu))->addTo(submenu);
      (new tRateAction(song->uniqueId(), 3, tr("Rate *** (&3)"), submenu))->addTo(submenu);
      (new tRateAction(song->uniqueId(), 4, tr("Rate **** (&4)"), submenu))->addTo(submenu);
      (new tRateAction(song->uniqueId(), 5, tr("Rate ***** (&5)"), submenu))->addTo(submenu);

      QString text = substituteSongFields(ProgramBase.preferences().PlaylistMenuFormat, song, true);
      text.replace("&", "&&");
      if (i == playlist_index)
      {
        QPixmap pm(getStockPixmap("play.png").convertToImage().smoothScale(10,10));
        menu->insertItem(pm, text, submenu);
      }
      else
        menu->insertItem(text, submenu);
    }
  }
  catch (exception &ex)
  {
    statusBar()->message(string2QString(ex.what()), 2000);
  }
  
  btnPlaylist->setPopup(menu);
}




void tMainWindow::updateWindowCaption()
{
  tSong *current = ProgramBase.currentSong();

  QString caption;
  if (current)
    caption = ProgramBase.preferences().CaptionFormat;
  else
    caption = tr("%databasename% - madman");
  if (FilenameValid)
  {
    QFileInfo info(CurrentFilename);
    caption.replace("%databasename%", info.fileName());
  }
  else
    caption.replace("%databasename%", tr("<unnamed>"));

  if (current)
    caption = substituteSongFields(caption, current, true);
  setCaption(caption);
}




void tMainWindow::updatePlayerStatus()
{
  try
  {
    float current_time = ProgramBase.preferences().Player.currentTime();
    float total_time = ProgramBase.preferences().Player.totalTime();
    if (total_time == 0)
      sliderSongPosition->setValue (0);
    else
      sliderSongPosition->setValue (int(10000 * current_time / total_time));

    QString current,total_duration;
    current.sprintf("%d:%02d", int(current_time) / 60,
                    int (current_time) % 60);
    total_duration.sprintf("%d:%02d", int(total_time) / 60,
                           int (total_time) % 60);

    labelPlayTime->setText(QString("%1 / %2").arg(current).arg(total_duration));

    bool paused_now = ProgramBase.preferences().Player.isPaused()
      || !ProgramBase.preferences().Player.isPlaying();
    if (PreviouslyPaused != paused_now)
    {
      if (paused_now)
      {
        setStockIcon(btnPlayPause, "play.png");
        setStockIcon(actionPlaybackPlayPause, "play.png");
      }
      else
      {
        setStockIcon(btnPlayPause, "pause.png");
        setStockIcon(actionPlaybackPlayPause, "pause.png");
      }
    }

    PreviouslyPaused = paused_now;
  }
  catch (exception &ex)
  {
    labelPlayTime->setText(QString(""));
    sliderSongPosition->setValue (0);
  }
}




void tMainWindow::showSongPopup()
{
  tSong *current = ProgramBase.currentSong();

  if (!current)
    return;

  tPassivePopup *pp = showPopup(
    ProgramBase.preferences().PassivePopupMode,
    substituteSongFields(
      ProgramBase.preferences().PassivePopupFormat, 
      current, true), 
    ProgramBase.preferences().PassivePopupTimeout);

  if (pp)
  {
    connect(pp, SIGNAL(skipBack()), this, SLOT(skipBack()));
    connect(pp, SIGNAL(skipForward()), this, SLOT(skipForward()));
  }
}




void tMainWindow::updateTrayIconStatus()
{
  if (!SystemTrayIcon)
    return;

  tSong *current = ProgramBase.currentSong();

  // update tooltip -----------------------------------------------------------
  if (current)
    SystemTrayIcon->setToolTip(substituteSongFields(ProgramBase.preferences().TrayTooltipFormat, 
						    current, true));
  else
    SystemTrayIcon->setToolTip(tr("madman"));

  // update icon --------------------------------------------------------------
  QImage tray_img = icon()->convertToImage();
  tray_img = tray_img.smoothScale(24, 24);

  QPixmap tray_pixmap;
  tray_pixmap.convertFromImage(tray_img);

  SystemTrayIcon->setIcon(tray_pixmap);
}




void tMainWindow::updateCurrentSongInfo()
{
  tSong *current = ProgramBase.currentSong();

  // rating actions -----------------------------------------------------------
  int current_rating = current ? current->rating() : -1;

  int const action_count = 6;
  QAction *actions[] = {
    actionSongRate0,
    actionSongRate1,
    actionSongRate2,
    actionSongRate3,
    actionSongRate4,
    actionSongRate5
  };
  
  for (int i = 0; i < action_count; i++)
    actions[i]->setOn(false);

  if (current_rating >= 0 && current_rating < action_count)
    actions[current_rating]->setOn(true);

  // display ------------------------------------------------------------------
  if (current)
  {
    QString label = tr("<a href=\"title\"><font size=\"+2\"><b>%1</b></font></a><br>")
      .arg(current->title());
    if (current->album() == "")
    {
      if (current->artist() != "")
        label += tr("by <a href=\"artist\">%1</a>")
	  .arg(current->artist());
    }
    else
    {
      if (current->artist() == "")
        label += tr("from <a href=\"album\">%1</a>")
	  .arg(current->album());
      else
	label += tr("from <a href=\"album\">%1</a> by <a href=\"artist\">%2</a>")
	  .arg(current->album())
	  .arg(current->artist());
    }
    labelCurrentSong->setText(label);

    if (current->rating() >= -1 && current->rating() <= 5)
    {
      IgnoreRatingChanges = true;
      comboRating->setCurrentItem(current->rating()+1);
      IgnoreRatingChanges = false;
    }
  }
  else
  {
    labelCurrentSong->setText("");
    comboRating->setCurrentItem(0);
  }
}




void tMainWindow::doContinuousAutoDJ()
{
  tPlayer &player = ProgramBase.preferences().Player;

  try
  {
    if (EnableAutoDJ && player.canGetValidStatus())
    {
      while (true)
      {
        int playlist_index = player.getPlayListIndex();
        unsigned playlist_length = player.getPlayListLength();
      
        int songs_needed = 6;
        int have_songs = playlist_length - playlist_index;
        if (have_songs >= songs_needed)
          break;
        songs_needed -= have_songs;
        
        tSongList slist;
        ProgramBase.autoDJ().reEvaluateScores();
        ProgramBase.autoDJ().selectSongs(slist, songs_needed);

        player.playEventually(slist);
      }
    }
  }
  catch (exception &ex)
  {
    statusBar()->message(tr("AutoDJ error: %1")
                         .arg(string2QString(ex.what())),
                         10000);
  }
}




void tMainWindow::noticeSongModified(const tSong *song, tSongField field)
{
  if (song == ProgramBase.currentSong())
  {
    updateCurrentSongInfo();
    updateTrayIconStatus();
    updateWindowCaption();
  }
}




void tMainWindow::processSearchChangeRequest(const QString &crit, bool restrict)
{
  if (restrict)
  {
    // this will never be perfect...
    if (editSearch->text().length() > 0)
      editSearch->setText(editSearch->text() + "&" + crit);
    else
      editSearch->setText(crit);
  }
  else
    editSearch->setText(crit);
}




void tMainWindow::loadDBWithBreakLockInteraction(
  tDatabase &db, const QString &filename)
{
  bool break_lock = false;
  while (true)
  {
    try
    {
      auto_ptr<tProgressDialog> progress(new tProgressDialog(this, false));
      progress->setWhat(tr("Loading database..."));
      db.load(filename, break_lock, progress.get());

      if (ProgramBase.preferences().ScanAtStartup)
        rescan(&db);
      return;
    }
    catch (tFileLocked &ex)
    {
      int result = QMessageBox::question(
        this,
        "madman",
        tr("The file\n%1\nwas locked when madman tried to read it. \n"
           "This might be because another copy of madman is "
           "accessing the file.\n"
           "What do you want to do?").arg(filename),
        tr("Cancel"),
        tr("Retry"),
        tr("Break the lock"));
      if (result == 0)
        throw;
      else if (result == 2)
        break_lock = true;
    }
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
