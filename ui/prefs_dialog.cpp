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




#include <qlistbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <qlistview.h>
#include <qlabel.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qmessagebox.h>
#include <qtoolbutton.h>
#include <qtable.h>

#include <stdexcept>

#include "database/song_set.h"
#include "utility/player.h"
#include "utility/plugin.h"
#include "utility/prefs.h"
#include "database/criterion.h"
#include "designer/prefs.h"
#include "ui/mainwin.h"
#include "ui/stock.h"
#include "ui/prefs_dialog.h"






// class declarations ---------------------------------------------------------
class tCriterionTableItem : public QTableItem
{
    typedef QTableItem super;
    QString Criterion;
    QLabel  &StatusLabel;
    
  public:
    tCriterionTableItem(QTable *table, const QString &crit, QLabel &status);
    QWidget *createEditor() const;
    void setContentFromEditor(QWidget *w);
    QString text() const
    {
      return Criterion;
    }
};




class tAutoDJScoreTableItem : public QComboTableItem
{
    typedef QComboTableItem super;
    QString NeverString;

  public:
    tAutoDJScoreTableItem(QTable *table, int score);

    int scoreAdjustment();
};




class tPreferencesDialog : public tPreferencesDialogBase
{
    Q_OBJECT;

    QString SelectedPlugin;

  public:
    tDirectoryListManager 	MediaDirectoryListManager;
    tDirectoryListManager	PluginDirectoryListManager;
    QSettings			&Settings;
    bool 			MediaDirectoriesChanged;

    tSortingPreferences         &SortingPreferences;

  public:
    tPreferencesDialog(QWidget* parent, tDirectoryList &media_dir_list, 
	tDirectoryList &plugin_dir_list, tSortingPreferences &sort_prefs, QSettings &settings);

  public slots:
    void noticeMediaDirectoriesChanged();
    void updatePluginList();
    void updateSelectedPluginInfo(int index);
    void configurePlugin();
    void newAutoDJScoringRule();
    void deleteAutoDJScoringRule();
    void moveUpAutoDJScoringRule();
    void moveDownAutoDJScoringRule();
    void resetAutoDJScoringRules();
    void showAutoDJPreferences(tAutoDJPreferences &adjp);

    void selectedInListWhenSorting(int index);
    void selectedInListSort2(int index);
    void selectedInListSort3(int index);

  protected:
    void swapAutoDJScoringRuleRows(unsigned row1, unsigned row2);
};




// tCriterionTableItem --------------------------------------------------------
tCriterionTableItem::tCriterionTableItem(QTable *table, 
    const QString &crit, QLabel &status)
: super(table, OnTyping, ""), Criterion(crit), StatusLabel(status)
{
}




QWidget *tCriterionTableItem::createEditor() const
{
  QLineEdit *e = new QLineEdit(table()->viewport());
  e->setText(Criterion);
  e->setFrame(false);
  return e;
}




void tCriterionTableItem::setContentFromEditor(QWidget *w)
{
  QLineEdit *e = (QLineEdit*) w;
  QString new_value = e->text();
  try
  {
    parseCriterion(new_value);
    Criterion = new_value;
  }
  catch (exception &ex)
  {
    StatusLabel.setText(
	qApp->translate("ErrorMessages", "<b>Error in Criterion: %1</b>")
	.arg(string2QString(ex.what())));
    QTimer::singleShot(5000, &StatusLabel, SLOT(clear()));
  }
}




// tAutoDJScoreTableItem ------------------------------------------------------
tAutoDJScoreTableItem::tAutoDJScoreTableItem(QTable *table, int score)
: super(table, QStringList(), false)
{
  NeverString = qApp->translate("AutoDJScoringRule", "Never");

  QStringList list;
  list.push_back(NeverString);
  for (int i = tAutoDJScoringRule::ScoreMinimum; i <= tAutoDJScoringRule::ScoreMaximum; i++)
    list.push_back(QString::number(i));
  setStringList(list);

  setCurrentItem(score - tAutoDJScoringRule::ScorePlayNever);
}




int tAutoDJScoreTableItem::scoreAdjustment()
{
  return currentItem() + tAutoDJScoringRule::ScorePlayNever;
}




// tPreferencesDialog ---------------------------------------------------------
tPreferencesDialog::tPreferencesDialog(QWidget* parent, tDirectoryList &media_dir_list, 
    tDirectoryList &plugin_dir_list, tSortingPreferences &sort_prefs, QSettings &settings)
: tPreferencesDialogBase(parent, "prefs", true),
  MediaDirectoryListManager(this, media_dir_list, lstMediaDirectories, btnAddMediaDirectory, btnRemoveMediaDirectory),
  PluginDirectoryListManager(this, plugin_dir_list, lstPluginDirectories, btnAddPluginDirectory, btnRemovePluginDirectory),
  Settings(settings), SortingPreferences(sort_prefs)
{
  connect(&PluginDirectoryListManager, SIGNAL(changed()),
      this, SLOT(updatePluginList()));
  connect(&MediaDirectoryListManager, SIGNAL(changed()),
      this, SLOT(noticeMediaDirectoriesChanged()));
  connect(lstPlugins, SIGNAL(highlighted(int)),
      this, SLOT(updateSelectedPluginInfo(int)));
  connect(btnConfigurePlugin, SIGNAL(clicked()),
      this, SLOT(configurePlugin()));
  connect(btnNewAutoDJScoringRule, SIGNAL(clicked()),
      this, SLOT(newAutoDJScoringRule()));
  connect(btnDeleteAutoDJScoringRule, SIGNAL(clicked()),
      this, SLOT(deleteAutoDJScoringRule()));
  connect(btnMoveUpAutoDJScoringRule, SIGNAL(clicked()),
      this, SLOT(moveUpAutoDJScoringRule()));
  connect(btnMoveDownAutoDJScoringRule, SIGNAL(clicked()),
      this, SLOT(moveDownAutoDJScoringRule()));
  connect(btnResetAutoDJScoringRules, SIGNAL(clicked()),
      this, SLOT(resetAutoDJScoringRules()));
  connect(listWhenSorting, SIGNAL(highlighted(int)),
      this, SLOT(selectedInListWhenSorting(int)));
  connect(listSort2, SIGNAL(highlighted(int)),
      this, SLOT(selectedInListSort2(int)));
  connect(listSort3, SIGNAL(highlighted(int)),
      this, SLOT(selectedInListSort3(int)));
  updatePluginList();

  MediaDirectoriesChanged = false;

  tblAutoDJScoringRules->setColumnWidth(1, 80);
  tblAutoDJScoringRules->setColumnStretchable(0, true);

  listSort2->insertItem(tr("<none>"));
  listSort3->insertItem(tr("<none>"));

  for (int i = 0; i < FIELD_COUNT; ++i)
  {
    listWhenSorting->insertItem(getFieldName((tSongField) i));
    listSort2->insertItem(getFieldName((tSongField) i));
    listSort3->insertItem(getFieldName((tSongField) i));
  }
  listWhenSorting->setCurrentItem(0);

  setStockIcon(btnNewAutoDJScoringRule, "new.png");
  setStockIcon(btnDeleteAutoDJScoringRule, "remove.png");
  setStockIcon(btnMoveUpAutoDJScoringRule, "up.png");
  setStockIcon(btnMoveDownAutoDJScoringRule, "down.png");
  setStockIcon(btnResetAutoDJScoringRules, "refresh.png");
}




void tPreferencesDialog::noticeMediaDirectoriesChanged()
{
  MediaDirectoriesChanged = true;
}




void tPreferencesDialog::updatePluginList()
{
  tPluginList plugins;
  enumeratePlugins(PluginDirectoryListManager.directoryList(),
      plugins);

  lstPlugins->clear();
  FOREACH_CONST(first, plugins, tPluginList)
    lstPlugins->insertItem(first->Name);
}




void tPreferencesDialog::updateSelectedPluginInfo(int index)
{
  tPluginList plugins;
  enumeratePlugins(PluginDirectoryListManager.directoryList(),
      plugins);

  if (index < (int) plugins.size())
  {
    SelectedPlugin = plugins[ index ].Filename;
    lblPluginName->setText(plugins[ index ].Name);
    lblPluginDescription->setText(plugins[ index ].Description);
  }
}




void tPreferencesDialog::configurePlugin()
{
  tPluginList plugins;
  enumeratePlugins(PluginDirectoryListManager.directoryList(),
      plugins);
  FOREACH(first, plugins, tPluginList)
    if (first->Filename == SelectedPlugin)
    {
      first->configure(Settings);
      return;
    }
}




void tPreferencesDialog::newAutoDJScoringRule()
{
  const unsigned new_row = tblAutoDJScoringRules->numRows();
  tblAutoDJScoringRules->setNumRows(new_row + 1);
  tblAutoDJScoringRules->setText(new_row, 0, "New score adjustment");
  tblAutoDJScoringRules->setItem(
      new_row, 1, new tAutoDJScoreTableItem(
	tblAutoDJScoringRules, 0));
  tblAutoDJScoringRules->setItem(
      new_row, 2, new tCriterionTableItem(
	tblAutoDJScoringRules, "~none", *labelAutoDJStatus));
}




void tPreferencesDialog::deleteAutoDJScoringRule()
{
  tblAutoDJScoringRules->removeRow(tblAutoDJScoringRules->currentRow());
}




void tPreferencesDialog::moveUpAutoDJScoringRule()
{
  unsigned current_row = tblAutoDJScoringRules->currentRow();
  if (current_row == 0)
    return;
  unsigned target_row = current_row - 1;

  swapAutoDJScoringRuleRows(target_row, current_row);
  tblAutoDJScoringRules->setCurrentCell(target_row, tblAutoDJScoringRules->currentColumn());
}




void tPreferencesDialog::moveDownAutoDJScoringRule()
{
  int current_row = tblAutoDJScoringRules->currentRow();
  if (current_row + 1 >= tblAutoDJScoringRules->numRows())
    return;
  unsigned target_row = current_row + 1;

  swapAutoDJScoringRuleRows(target_row, current_row);
  tblAutoDJScoringRules->setCurrentCell(target_row, tblAutoDJScoringRules->currentColumn());
}




void tPreferencesDialog::resetAutoDJScoringRules()
{
  tAutoDJPreferences adjp;
  adjp.resetToDefault();
  showAutoDJPreferences(adjp);
}




void tPreferencesDialog::showAutoDJPreferences(tAutoDJPreferences &adjp)
{
  tblAutoDJScoringRules->setNumRows(adjp.ScoringRuleList.size());
  unsigned index = 0;

  FOREACH_CONST(first, adjp.ScoringRuleList, tAutoDJScoringRuleList)
  {
    tblAutoDJScoringRules->setText(index, 0, first->Comment);
    tblAutoDJScoringRules->setItem(
	index, 1, new tAutoDJScoreTableItem(
	  tblAutoDJScoringRules,
	  first->ScoreAdjustment));
    tblAutoDJScoringRules->setItem(
	index, 2, new tCriterionTableItem(
	  tblAutoDJScoringRules,
	  first->Criterion,
	  *labelAutoDJStatus));
    index++;
  }
}




void tPreferencesDialog::selectedInListWhenSorting(int index)
{
  if (SortingPreferences.SecondarySortField[index] == FIELD_INVALID)
    listSort2->setCurrentItem(0);
  else
    listSort2->setCurrentItem(SortingPreferences.SecondarySortField[index]+1);

  if (SortingPreferences.TertiarySortField[index] == FIELD_INVALID)
    listSort3->setCurrentItem(0);
  else
    listSort3->setCurrentItem(SortingPreferences.TertiarySortField[index]+1);
}




void tPreferencesDialog::selectedInListSort2(int index)
{
  tSongField primary_field = (tSongField) listWhenSorting->index(
      listWhenSorting->selectedItem());

  tSongField secondary_field;
  if (index == 0)
  {
    secondary_field = FIELD_INVALID;
    listSort3->setCurrentItem(0);
    listSort3->setEnabled(false);
  }
  else
  {
    secondary_field = (tSongField) (index-1);
    listSort3->setEnabled(true);
  }

  SortingPreferences.SecondarySortField[primary_field] = secondary_field;
}




void tPreferencesDialog::selectedInListSort3(int index)
{
  tSongField primary_field = (tSongField) listWhenSorting->index(
      listWhenSorting->selectedItem());

  tSongField tertiary_field;
  if (index == 0)
    tertiary_field = FIELD_INVALID;
  else
    tertiary_field = (tSongField) (index-1);
  SortingPreferences.TertiarySortField[primary_field] = tertiary_field;
}




void tPreferencesDialog::swapAutoDJScoringRuleRows(unsigned row1, unsigned row2)
{
  QString buf_row2_comment = tblAutoDJScoringRules->text(row2, 0);
  QTableItem *buf_criterion1 = tblAutoDJScoringRules->item(row1, 1);
  QTableItem *buf_score1 = tblAutoDJScoringRules->item(row1, 2);
  QTableItem *buf_criterion2 = tblAutoDJScoringRules->item(row2, 1);
  QTableItem *buf_score2 = tblAutoDJScoringRules->item(row2, 2);

  tblAutoDJScoringRules->takeItem(buf_criterion1);
  tblAutoDJScoringRules->takeItem(buf_score1);
  tblAutoDJScoringRules->takeItem(buf_criterion2);
  tblAutoDJScoringRules->takeItem(buf_score2);

  tblAutoDJScoringRules->setText(row2, 0, tblAutoDJScoringRules->text(row1, 0));
  tblAutoDJScoringRules->setText(row1, 0, buf_row2_comment);

  tblAutoDJScoringRules->setItem(row2, 1, buf_criterion1);
  tblAutoDJScoringRules->setItem(row1, 1, buf_criterion2);

  tblAutoDJScoringRules->setItem(row2, 2, buf_score1);
  tblAutoDJScoringRules->setItem(row1, 2, buf_score2);
}




// public functions -----------------------------------------------------------
pair<bool,bool> editPreferences(tMainWindow *mainwin, tPreferences &prefs, 
    tDirectoryList &media_dir_list, 
    QSettings &settings, int starting_tab)
{
  tSortingPreferences sort_prefs = prefs.SortingPreferences;

  // create the dialog
  tDirectoryList work_media_dir_list,work_plugin_dir_list;
  work_media_dir_list = media_dir_list;
  work_plugin_dir_list = prefs.PluginDirectories;

  tPreferencesDialog prefsdialog(mainwin, 
      work_media_dir_list, 
      work_plugin_dir_list, 
      sort_prefs,
      settings);
  prefsdialog.tabMain->setCurrentPage(starting_tab);

  // populate the dialog
  vector<QString> players;
  listPlayers(players);
  int index = 0,current_index = 0;
  FOREACH_CONST(first, players, vector<QString>)
  {
    if (prefs.Player.name() == *first)
      current_index = index;
    prefsdialog.comboPlayer->insertItem(*first);
    index++;
  }
  prefsdialog.comboPlayer->setCurrentItem(current_index);

#define SET_BOOL(NAME) \
  prefsdialog.check##NAME->setChecked( prefs.NAME);

  SET_BOOL(ScanAtStartup);
  SET_BOOL(RememberGeometry);
  SET_BOOL(CollectHistory);
  SET_BOOL(EnableSystemTrayIcon);
  SET_BOOL(MinimizeToSystemTray);

  prefsdialog.spinBackupCount->setValue(prefs.BackupCount);

  prefsdialog.checkHttpDaemonEnabled->setChecked(false);
  prefsdialog.checkHttpDaemonEnabled->setChecked(true);
  SET_BOOL(HttpDaemonEnabled);

  prefsdialog.spinHttpDaemonPort->setValue(prefs.HttpDaemonPort);
  SET_BOOL(HttpDownloadsEnabled);
  SET_BOOL(HttpScriptingEnabled);
  SET_BOOL(HttpWriteScriptingEnabled);
  SET_BOOL(HttpLocalPlayEnabled);
  SET_BOOL(HttpBrowsingEnabled);
  SET_BOOL(HttpRestrictToLocalhost);

  prefsdialog.showAutoDJPreferences(prefs.AutoDJPreferences);

  prefsdialog.comboPassivePopupMode->setCurrentItem((int) prefs.PassivePopupMode);
  prefsdialog.comboID3ReadPreference->setCurrentItem((int) prefs.ID3ReadPreference);

  // show
  int returncode = prefsdialog.exec();

  // process results
  if (returncode == QDialog::Accepted)
  {
    media_dir_list = work_media_dir_list;
    prefs.PluginDirectories = work_plugin_dir_list;

    prefs.setPlayer(createPlayer(prefsdialog.comboPlayer->currentText()));

#define GET_BOOL(NAME) \
    prefs.NAME = prefsdialog.check##NAME->isChecked();

    GET_BOOL(ScanAtStartup);
    GET_BOOL(RememberGeometry);
    GET_BOOL(CollectHistory);
    GET_BOOL(EnableSystemTrayIcon);
    GET_BOOL(MinimizeToSystemTray);

    prefs.BackupCount = prefsdialog.spinBackupCount->value();

    GET_BOOL(HttpDaemonEnabled);
    prefs.HttpDaemonPort = prefsdialog.spinHttpDaemonPort->value();
    GET_BOOL(HttpDownloadsEnabled);
    GET_BOOL(HttpScriptingEnabled);
    GET_BOOL(HttpWriteScriptingEnabled);
    GET_BOOL(HttpLocalPlayEnabled);
    GET_BOOL(HttpBrowsingEnabled);
    GET_BOOL(HttpRestrictToLocalhost);

    {
      // Auto DJ stuff
      tAutoDJPreferences &adjp = prefs.AutoDJPreferences;
      adjp.ScoringRuleList.clear();

      for (int i = 0; i < prefsdialog.tblAutoDJScoringRules->numRows(); i++)
      {
	tAutoDJScoringRule crit;
	crit.Comment = prefsdialog.tblAutoDJScoringRules->text(i, 0);
	crit.ScoreAdjustment = dynamic_cast<tAutoDJScoreTableItem *>(prefsdialog.tblAutoDJScoringRules->item(i, 1) )->scoreAdjustment();
	crit.Criterion = prefsdialog.tblAutoDJScoringRules->text(i, 2);
	adjp.ScoringRuleList.push_back(crit);
      }
    }

    prefs.SortingPreferences = sort_prefs;

    prefs.PassivePopupMode = (tPassivePopupMode) 
      prefsdialog.comboPassivePopupMode->currentItem();
    prefs.ID3ReadPreference = (tID3ReadPreference) 
      prefsdialog.comboID3ReadPreference->currentItem();
    
    return make_pair(true, prefsdialog.MediaDirectoriesChanged);
  }
  else
    return make_pair(false, false);
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
#include "ui/prefs_dialog.moc"
