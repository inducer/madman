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




#include <qcombobox.h>
#include <qaction.h>
#include <qcolor.h>
#include <qlineedit.h>
#include <qpalette.h>
#include <qpainter.h>
#include <qlistview.h>
#include <qheader.h>
#include <qregexp.h>
#include <qmessagebox.h>
#include <qaccel.h>
#include <qdragobject.h>
#include <qdatetime.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qstatusbar.h>
#include <qprogressdialog.h>

#include <unistd.h>
#include <algorithm>

#include "utility/player.h"
#include "utility/plugin.h"
#include "utility/progress.h"
#include "utility/prefs.h"
#include "database/auto_dj.h"
#include "database/song_list_tools.h"
#include "ui/song_set_view.h"
#include "ui/mainwin.h"
#include "ui/auto_tag.h"
#include "ui/progress_impl.h"




// tSongRetag -----------------------------------------------------------------
void tSongRetag::executeOn(tSong *song)
{
  try
  {
    song->setFieldText(Field, NewText);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(qApp->mainWidget(), qApp->translate("ErrorMessages", "madman"),
	qApp->translate("ErrorMessages", "Error while writing tag:\n%1").arg(ex.what()), 
	QMessageBox::Ok, QMessageBox::NoButton);
  }
}




// tDeferredSongRetag ---------------------------------------------------------
void tDeferredSongRetag::execute()
{
  Retag.executeOn(Song);

  delete this;
}




// tVisibilityAction ----------------------------------------------------------
tVisibilityAction::tVisibilityAction(QObject *parent, tSongListView *view, tSongField field)
: QAction(parent), ListView(view), Field(field)
{
  setText(getFieldName(Field));
  setToggleAction(true);
  setOn(ListView->isFieldVisible(Field));

  connect(this, SIGNAL(toggled(bool)), this, SLOT(onToggle(bool)));
}




void tVisibilityAction::onToggle(bool on)
{
  if (on)
    ListView->showField(Field);
  else
    ListView->hideField(Field);
}




// tSongListView --------------------------------------------------------------
tSongListView::tSongListView(QWidget *parent, const char *name)
: tAcceleratorTable(parent, name)
{
  setShowGrid(false);
  setSelectionMode(MultiRow);
  verticalHeader()->hide();
  setLeftMargin(0);
  setColumnMovingEnabled(true);
  setDragEnabled(true);
  setFocusStyle(SpreadSheet);

  SortIndicatorEnable = false;
  
  FieldToColumn.resize(FIELD_COUNT);
  ColumnToField.resize(FIELD_COUNT);
  FieldWidths.resize(FIELD_COUNT);

  tSongField default_fields[] = 
    {FIELD_ARTIST, 
     FIELD_TITLE_OR_FILENAME,
     FIELD_ALBUM, 
     FIELD_TRACKNUMBER,
     FIELD_DURATION,
     FIELD_RATING,
     FIELD_INVALID
    };

  for (int field = 0; field < FIELD_COUNT; field++)
  {
    FieldToColumn[field] = -1;
    FieldWidths[field] = 100;
  }

  ShownColumns = 0;

  for (unsigned i = 0; default_fields[i] != FIELD_INVALID; i++)
  {
    tSongField f = default_fields[i];
    FieldToColumn[f] = ShownColumns;
    ColumnToField[ShownColumns] = f;
    ShownColumns++;
  }

  connect(
      this, SIGNAL(contextMenuRequested(int, int, const QPoint &)),
      this, SLOT(slotContextMenuRequested(int, int, const QPoint &)));

  horizontalHeader()->installEventFilter(this);
  viewport()->installEventFilter(this);
}




void tSongListView::initialize()
{
  setup();

  connect(&tProgramBase::preferences().Player, SIGNAL(currentSongChanged()),
      this, SLOT(currentSongChanged()));

  // Fields menu --------------------------------------------------------------
  FieldVisibilityMenu = new QPopupMenu(this);
  for (unsigned i = 0; i < FIELD_COUNT; i++)
  {
    QAction *action = new tVisibilityAction(FieldVisibilityMenu, this, (tSongField) i);
    action->addTo(FieldVisibilityMenu);
  }
}




void tSongListView::setSongList(const tSongList &list, bool preserve_scroll_pos)
{
  int contents_x = contentsX();
  int contents_y = contentsY();

  setUpdatesEnabled(false);
  verticalHeader()->setUpdatesEnabled(false);

  SongList = list;
  setNumRows(list.size());

  int row_height = fontMetrics().height() + 2;

  // set the stage for the "big" row height update below
  if (list.size())
    setRowHeight(0, 0);

  // set all the in-between row heights
  for (unsigned row = 1; row < list.size(); row++)
    setRowHeight(row, row_height);

  if (preserve_scroll_pos)
    setContentsPos(contents_x, contents_y);

  verticalHeader()->setUpdatesEnabled(true);
  setUpdatesEnabled(true);

  // this actually triggers the size update for the entire table.
  if (list.size())
    setRowHeight(0, row_height);

  update();
  viewport()->update();
  verticalHeader()->update();
}




void tSongListView::saveListViewAppearance(QSettings &settings, const QString &prefix)
{
  for (int field = 0; field < FIELD_COUNT; field++)
  {
    settings.writeEntry(QString("%1/listview.fields.%2.column").
	arg(prefix).arg(field),
	FieldToColumn[ field ]);
    settings.writeEntry(QString("%1/listview.fields.%2.width").
	arg(prefix).arg(field),
	FieldWidths[ field ]);
  }
}




void tSongListView::loadListViewAppearance(QSettings &settings, const QString &prefix)
{
  vector<int> ftc;
  vector<int> ctf;
  vector<int> widths;

  ftc.resize(FIELD_COUNT);
  ctf.resize(FIELD_COUNT);
  widths.resize(FIELD_COUNT);

  for (int i = 0; i < FIELD_COUNT; i++)
    ftc[ i ] = -1;
  for (int i = 0; i < FIELD_COUNT; i++)
    ctf[ i ] = -1;

  bool ok;
  int shown_cols = 0;
  for (int field = 0; field < FIELD_COUNT; field++)
  {
    int column = settings.readNumEntry(
	QString("%1/listview.fields.%2.column").
	arg(prefix).arg(field), 0, &ok); 
    if (!ok)
      return;

    if (column >= 0 && column < FIELD_COUNT)
    {
      shown_cols++;
      if (ctf[ column ] == -1)
      {
	ftc[ field ] = column;
	ctf[ column ] = field;
      }
      else
      {
	// this is not an injective map. disregard it.
	cout << prefix << " not injective, discarding." << endl;
	return;
      }
    }

    int width = settings.readNumEntry(
	QString("%1/listview.fields.%2.width").
	arg(prefix).arg(field), 0, &ok); 
    if (ok)
      widths[ field ] = width;
  }

  for (int i = 0; i < shown_cols; i++)
    if (ctf[ i ] == -1)
    {
      // this is not a surjective map. disregard it.
      cout << prefix << " not surjective, discarding." << endl;
      return;
    }

  FieldToColumn = ftc;
  ColumnToField = ctf;
  FieldWidths = widths;
  ShownColumns = shown_cols;

  setup();
}




void tSongListView::setup()
{
  setUpdatesEnabled(false);
  horizontalHeader()->setUpdatesEnabled(false);

  // straighten out header
  setNumCols(0);

  // rebuild header
  setNumCols(ShownColumns);
  for (int column = 0; column < ShownColumns; column++)
  {
    tSongField field = (tSongField) ColumnToField[ column ];
    setColumnWidth(column, FieldWidths[ field ]);
    horizontalHeader()->setLabel(column, getFieldName(field));
  }

  showSortIndicator();

  horizontalHeader()->setUpdatesEnabled(true);
  setUpdatesEnabled(true);

  update();
  viewport()->update();
  horizontalHeader()->update();
  updateScrollBars();
}




tSong *tSongListView::highlightedSong()
{
  int row = currentRow();
  if (row >= 0 && row < (int) SongList.size())
    return SongList[ row ];
  else
    return NULL;
}




void tSongListView::highlightSong(tSong *song)
{
  tSongList::iterator it = ::find(SongList.begin(), SongList.end(), song);
  if (it != SongList.end())
  {
    clearSelection();
    setCurrentCell(it - SongList.begin(), currentColumn());
  }
}




void tSongListView::getSelection(tSongList &songlist) const
{
  songlist.clear();

  tIndexList ilist;
  getSelection(ilist);
  FOREACH_CONST(first, ilist, tIndexList)
  {
    if (*first >= 0 && unsigned(*first) < SongList.size())
      songlist.push_back(SongList[ *first ]);
  }
}




void tSongListView::getSelection(tIndexList &indexlist) const
{
  indexlist.clear();

  unsigned num = numSelections();

  for (unsigned i = 0; i < num; i++)
  {
    QTableSelection sel = selection(i);
    if (sel.isActive())
      for (int row = sel.topRow(); row <= sel.bottomRow(); row++)
	indexlist.push_back(row);
  }
}




void tSongListView::setSongDropEnable(bool enabled)
{
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
}




void tSongListView::setSortIndicator(bool enable, tSongField field, bool ascending)
{
  SortIndicatorEnable = enable;
  SortIndicatorField = field;
  IndicatorAscending = ascending;

  showSortIndicator();
}




void tSongListView::columnWidthChanged(int col)
{
  super::columnWidthChanged(col);
  FieldWidths[ ColumnToField[ col ] ] = columnWidth(col);
}




void tSongListView::swapColumns (int col1, int col2, bool swapHeader)
{
  swap(ColumnToField[ col1 ], ColumnToField[ col2 ]);

  for (int col = 0; col < ShownColumns; col++)
    FieldToColumn[ ColumnToField[ col ] ] = col;

  setup();
}




void tSongListView::sortColumn (int col, bool ascending, bool wholeRows)
{
  emit sortingChanged((tSongField) ColumnToField[ col ], ascending);
}




bool tSongListView::isFieldVisible(tSongField field)
{
  return FieldToColumn[ field ] >= 0;
}




void tSongListView::showField(tSongField field)
{
  if (FieldToColumn[ field ] >= 0)
    return;

  FieldToColumn[ field ] = ShownColumns;
  ColumnToField[ ShownColumns ] = field;
  ShownColumns++;

  setup();
}




void tSongListView::hideField(tSongField field)
{
  int column = FieldToColumn[ field ];
  if (column < 0)
    return;

  FieldToColumn[ field ] = -1;
  ShownColumns--;

  for (int i = 0; i < FIELD_COUNT; i++)
  {
    tSongField f = (tSongField) i;
    if (FieldToColumn[ f ] > column)
      FieldToColumn[ f ]--;
  }

  // update ColumnToField 
  for (int i = 0; i < FIELD_COUNT; i++)
    ColumnToField[ i ] = -1;
  
  for (int i = 0; i < FIELD_COUNT; i++)
  {
    tSongField f = (tSongField) i;
    int c = FieldToColumn[ f ];
    if (c >= 0)
      ColumnToField[ c ] = f;
  }

  setup();
}




void tSongListView::slotContextMenuRequested (int row, int col, const QPoint & pos)
{
  if (row >= 0 && row < int(SongList.size()))
    emit contextMenuRequested(
        SongList[ row ], 
        (tSongField) ColumnToField[ col ],
        pos);
}




void tSongListView::currentSongChanged()
{
  updateContents();
}




// QTable backend -------------------------------------------------------------
QTableItem *tSongListView::item(int r, int c)
{
  return NULL;
}





void tSongListView::setItem(int r, int c, QTableItem *i)
{
}




void tSongListView::takeItem(QTableItem *item)
{
  QTable::takeItem(item);
}




void tSongListView::clearCell(int r, int c)
{
}




void tSongListView::insertWidget(int r, int c, QWidget *w)
{
  WidgetDictionary.replace(indexOf(r, c), w);
}




QWidget *tSongListView::cellWidget(int r, int c) const
{
  return WidgetDictionary.find(indexOf(r, c));
}




void tSongListView::clearCellWidget(int r, int c)
{
  QWidget *w = WidgetDictionary.take(indexOf(r, c));
  if (w)
    w->deleteLater();
}




QWidget *tSongListView::createEditor (int row, int col, bool initFromCell) const
{
  // This does happen. Poo on Qt.
  if (row < 0 || row >= int(SongList.size()))
    return NULL;

  tSongField field = (tSongField) ColumnToField[col];
  tSong *song = SongList[row];

  if (song->isFieldWritable(field))
  {
    if (field == FIELD_GENRE)
    {
      QComboBox *cb = new QComboBox(viewport(), "qt_combobox_in_qtable");
      cb->setEditable(true);
      for (int i = 0; i < GENRE_COUNT; i++)
        cb->insertItem(genreIdToString(i));
      cb->setCurrentText(song->fieldText(field));
      cb->lineEdit()->selectAll();
      cb->lineEdit()->setFrame(false);
      cb->setAutoCompletion(true);
      return cb;
    }
    else
    {
      QLineEdit *e = new QLineEdit(viewport(), "qt_lineeditor_in_qtable");
      e->setText(song->fieldText(field));
      e->selectAll();
      return e;
    }
  }
  else
  {
    QMessageBox::warning(
      topLevelWidget(), 
      tr("madman"),
      tr("This field is not writable on this song."));
    return NULL;
  }
}




void tSongListView::setCellContentFromEditor (int row, int col)
{
  tSongField field = (tSongField) ColumnToField[ col ];

  QString new_text;
  if (field == FIELD_GENRE)
  {
    QComboBox *e = (QComboBox*) cellWidget(row, col);
    new_text = e->currentText();
  }
  else
  {
    QLineEdit *e = (QLineEdit*) cellWidget(row, col);
    new_text = e->text();
  }

  tSong *song = SongList[ row ];
  emit fieldChanged(song, field, new_text);
}




void tSongListView::paintCell (QPainter *p, int row, int col, const QRect & cr, bool selected, const QColorGroup & cg)
{
  p->save();
  tSongField field = (tSongField) ColumnToField[col];
  tSong *song = SongList[row];

  QString field_text = song->humanReadableFieldText(field);

  QColor background;
  if (selected)
  {
    if (row % 2 == 0)
      background = cg.color(QColorGroup::Highlight).light(120);
    else
      background = cg.color(QColorGroup::Highlight);
  }
  else
    if (row % 2 == 0)
      background = cg.color(QColorGroup::Base);
    else
      background = cg.color(QColorGroup::Base).dark(105);

  p->fillRect(0, 0, cr.width(), cr.height(), background);

  int w = cr.width();
  int h = cr.height();

  if (selected)
    p->setPen(cg.highlightedText());
  else
    p->setPen(cg.text());

  if (song == tProgramBase::currentSong())
  {
    QFont bold_font = p->font();
    bold_font.setBold(true);
    p->setFont(bold_font);
  }

  QFontMetrics metrics = p->fontMetrics();
  if (metrics.width(field_text) > w)
  {
    while (metrics.width(field_text + "...") > w && field_text.length())
      field_text.remove(field_text.length() - 1, 1);
    field_text += "...";
  }

  p->drawText(1, 1, w - 2, h, Qt::AlignLeft, field_text);
  p->restore();
}




void tSongListView::noticeSongModified(const tSong *song, tSongField field)
{
  for (unsigned i = 0; i < SongList.size(); i++)
    if (song == SongList[ i ])
    {
      int column = FieldToColumn[ field ];
      if (column != -1)
	updateCell(i, column);
    }
}




bool tSongListView::eventFilter (QObject *watched, QEvent *e)
{
  if (watched == horizontalHeader() && e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *me = (QMouseEvent *) e;
    if ( me->button() == Qt::RightButton)
    {
      FieldVisibilityMenu->popup(me->globalPos());
      return true;
    }
    else
      return super::eventFilter(watched, e);
  }
  else if (watched == viewport() && e->type() == QEvent::MouseButtonDblClick)
  {
    QMouseEvent *me = (QMouseEvent *) e;
    if ( me->button() == Qt::LeftButton)
    {
      int row = rowAt(viewportToContents(me->pos()).y());
      if (row >= 0)
	emit doubleClicked(SongList[ row ]);
      return true;
    }
    else
      return false;
  }
  else
    return super::eventFilter(watched, e);
}




void tSongListView::contentsDragEnterEvent(QDragEnterEvent *event)
{
  if (QUriDrag::canDecode(event))
    event->accept();
}




void tSongListView::contentsDragMoveEvent(QDragMoveEvent *event)
{
  if (QUriDrag::canDecode(event))
    event->accept();
}




void tSongListView::contentsDropEvent(QDropEvent *event)
{
  if (QUriDrag::canDecode(event))
  {
    QStrList uri_list;
    QUriDrag::decode(event, uri_list);

    vector<tFilename> filenames;
    char *uri = uri_list.first();
    while (uri)
    {
      QString filename = QUriDrag::uriToLocalFile(uri);
      if (!filename.isNull())
	filenames.push_back((const char *) filename.utf8());
      uri = uri_list.next();
    }

    int row = rowAt(event->pos().y());
    tSong *onto_song = NULL;
    if (row >= 0 && row < (int) SongList.size())
      onto_song = SongList[ row ];
    
    emit songsDropped(event->source() == this, onto_song, filenames);
  }
}




void tSongListView::contentsMousePressEvent (QMouseEvent * e)
{
  super::contentsMousePressEvent(e);
}




void tSongListView::contentsMouseReleaseEvent (QMouseEvent * e)
{
  super::contentsMouseReleaseEvent(e);
}




QDragObject *tSongListView::dragObject()
{
  QStrList uris;
  tSongList songlist;
  getSelection(songlist);

  FOREACH_CONST(first, songlist, tSongList)
    uris.append(QUriDrag::localFileToUri(QString::fromUtf8((*first)->filename().c_str())));

  return new QUriDrag(uris, this);
}



void tSongListView::showSortIndicator()
{
  if (SortIndicatorEnable)
    horizontalHeader()->setSortIndicator(FieldToColumn[ SortIndicatorField ], IndicatorAscending);
  else
    horizontalHeader()->setSortIndicator(-1);
}




// tSongSetViewManager --------------------------------------------------------
tSongSetViewManager::tSongSetViewManager(tSongListView &lv, const QString & pref_prefix )
: ListView(lv), LastRetagValid(false), PreferencesPrefix(pref_prefix)
{
  connect(&ListView, SIGNAL(fieldChanged(tSong *, tSongField, const QString &)),
           this, SLOT(fieldChanged(tSong *, tSongField, const QString &)));
  connect(&ListView, SIGNAL(contextMenuRequested(tSong *,tSongField, const QPoint &)),
           this, SLOT(contextMenuRequested(tSong *, tSongField, const QPoint &)));
  connect(&ListView, SIGNAL(doubleClicked(tSong *)),
           this, SLOT(slotPlayNow(tSong *)));
  connect(&ListView, SIGNAL(selectionChanged()),
           this, SLOT(selectionChanged()));
  connect(&ListView, SIGNAL(sortingChanged(tSongField, bool)),
           this, SLOT(sortingChanged(tSongField, bool)));
}




void tSongSetViewManager::setup()
{
  ListView.initialize();
  auto_ptr<QPopupMenu> new_menu (buildPopupMenu());
  Menu = new_menu;
}




void tSongSetViewManager::saveListViewAppearance(QSettings &settings)
{
  ListView.saveListViewAppearance(settings, PreferencesPrefix);
}




void tSongSetViewManager::loadListViewAppearance(QSettings &settings)
{
  ListView.loadListViewAppearance(settings, PreferencesPrefix);
}




void tSongSetViewManager::changeSongSet(tSongSet *from, tSongSet *to)
{
  if (from)
  {
    disconnect(from, NULL, this, NULL);
  }
  if (to)
  {
    connect(
	to, SIGNAL(notifyChange(bool, tProgress *)), 
	this, SLOT(noticeSongSetModified(bool, tProgress *)));
    connect(
	to, SIGNAL(notifySongModified(const tSong *, tSongField)), 
	&ListView, SLOT(noticeSongModified (const tSong *, tSongField)));
  }

  redisplay(false);
  emit notifySongSetChanged();
}




QPopupMenu *tSongSetViewManager::buildPopupMenu()
{
  QPopupMenu *menu = new QPopupMenu(&ListView);

  menu->insertItem(tr("Edit this &tag (F2)"), this, SLOT(slotEditThisTag()));
  menu->insertSeparator();

  // Play menu ----------------------------------------------------------------
  QPopupMenu *play_menu = new QPopupMenu(&ListView);
  play_menu->insertItem(tr("&Play now (F9)"), this, SLOT(slotPlayNow()));
  play_menu->insertItem(tr("Play &next (F10)"), this, SLOT(slotPlayNext()));
  play_menu->insertItem(tr("Play &eventually (F11)"), this, SLOT(slotPlayEventually()));
  play_menu->insertSeparator();
  play_menu->insertItem(tr("Auto DJ 20 songs"), this, SLOT(slotAutoDJ()));

  menu->insertItem(tr("&Play"), play_menu);

  ListView.addShortCut(new tKeyboardShortCut(Key_F9, this, SLOT(slotPlayNow())));
  ListView.addShortCut(new tKeyboardShortCut(Key_F10, this, SLOT(slotPlayNext())));
  ListView.addShortCut(new tKeyboardShortCut(Key_F11, this, SLOT(slotPlayEventually())));

  // Rate menu ----------------------------------------------------------------
  QPopupMenu *rate_menu = new QPopupMenu(&ListView);
  rate_menu->insertItem(tr("&No rating"), this, SLOT(rateNone()));
  rate_menu->insertSeparator();
  rate_menu->insertItem(tr("&0: -"), this, SLOT(rate0()));
  rate_menu->insertItem(tr("&1: *"), this, SLOT(rate1()));
  rate_menu->insertItem(tr("&2: **"), this, SLOT(rate2()));
  rate_menu->insertItem(tr("&3: ***"), this, SLOT(rate3()));
  rate_menu->insertItem(tr("&4: ****"), this, SLOT(rate4()));
  rate_menu->insertItem(tr("&5: *****"), this, SLOT(rate5()));

  menu->insertItem(tr("&Rate"), rate_menu);

  ListView.addShortCut(new tKeyboardShortCut(Key_0, this, SLOT(rate0())));
  ListView.addShortCut(new tKeyboardShortCut(Key_1, this, SLOT(rate1())));
  ListView.addShortCut(new tKeyboardShortCut(Key_2, this, SLOT(rate2())));
  ListView.addShortCut(new tKeyboardShortCut(Key_3, this, SLOT(rate3())));
  ListView.addShortCut(new tKeyboardShortCut(Key_4, this, SLOT(rate4())));
  ListView.addShortCut(new tKeyboardShortCut(Key_5, this, SLOT(rate5())));

  // Like songs ---------------------------------------------------------------
  QPopupMenu *like_songs_menu = new QPopupMenu(&ListView);

  QPopupMenu *ls_action_menu = new QPopupMenu(&ListView);
  ls_action_menu->insertItem(tr("Play now"), this, SLOT(playNowAllByThisArtist()));
  ls_action_menu->insertItem(tr("Play next"), this, SLOT(playNextAllByThisArtist()));
  ls_action_menu->insertItem(tr("Play eventually"), this, SLOT(playEventuallyAllByThisArtist()));
  ls_action_menu->insertItem(tr("Restrict view"), this, SLOT(restrictViewToThisArtist()));

  like_songs_menu->insertItem(tr("&By this artist"), ls_action_menu);

  ls_action_menu = new QPopupMenu(&ListView);
  ls_action_menu->insertItem(tr("Play now"), this, SLOT(playNowAllOnThisAlbum()));
  ls_action_menu->insertItem(tr("Play next"), this, SLOT(playNextAllOnThisAlbum()));
  ls_action_menu->insertItem(tr("Play eventually"), this, SLOT(playEventuallyAllOnThisAlbum()));
  ls_action_menu->insertItem(tr("Restrict view"), this, SLOT(restrictViewToThisAlbum()));

  like_songs_menu->insertItem(tr("&On this album"), ls_action_menu);

  ls_action_menu = new QPopupMenu(&ListView);
  ls_action_menu->insertItem(tr("Play now"), this, SLOT(playNowAllInThisDirectory()));
  ls_action_menu->insertItem(tr("Play next"), this, SLOT(playNextAllInThisDirectory()));
  ls_action_menu->insertItem(tr("Play eventually"), this, SLOT(playEventuallyAllInThisDirectory()));
  ls_action_menu->insertItem(tr("Restrict view"), this, SLOT(restrictViewToThisDirectory()));

  like_songs_menu->insertItem(tr("&In this directory"), ls_action_menu);

  menu->insertItem(tr("&Songs like this one"), like_songs_menu);

  // Tag menu -----------------------------------------------------------------
  QPopupMenu *tag_menu = new QPopupMenu(&ListView);
  tag_menu->insertItem(tr("AutoTag"), this, SLOT(autoTag()));
  tag_menu->insertSeparator();
  tag_menu->insertItem(tr("Repeat last retag on selection"), this, SLOT(redoRetag()));
  tag_menu->insertItem(tr("R&eread tag"), this, SLOT(rereadTag()));
  tag_menu->insertItem(tr("Re&write tag"), this, SLOT(rewriteTag()));
  tag_menu->insertItem(tr("&Strip tag"), this, SLOT(stripTag()));

  menu->insertItem(tr("&Tag"), tag_menu);

  // Plugins menu -------------------------------------------------------------
  QPopupMenu *plugins_menu = new QPopupMenu(&ListView);
  MenuIdToPluginFilenameMap.clear();
  tPluginList plugins;
  enumeratePlugins(tProgramBase::preferences().PluginDirectories, plugins);
  FOREACH(first, plugins, tPluginList)
  {
    int id = plugins_menu->insertItem(first->MenuString);
    MenuIdToPluginFilenameMap[ id ] = first->Filename;
  }
  connect(plugins_menu, SIGNAL(activated(int)),
      this, SLOT(pluginActivated(int)));

  menu->insertItem(tr("Run &Plugin"), plugins_menu);

  // Others -------------------------------------------------------------------
  menu->insertSeparator();
  menu->insertItem(tr("&Delete from disk (Ctrl+Del)"), this, SLOT(deleteFromDisk()));
  menu->insertItem(tr("R&eset play statistics"), this, SLOT(resetStatistics()));
  menu->insertItem(tr("&Highlight current song (F12)"), this, SLOT(highlightCurrentSong()));

  ListView.addShortCut(new tKeyboardShortCut(Key_F12, this, SLOT(highlightCurrentSong())));
  ListView.addShortCut(new tKeyboardShortCut(CTRL + Key_Delete, this, SLOT(deleteFromDisk())));

  ListView.addShortCut(new tKeyboardShortCut(CTRL + Key_A, this, SLOT(selectAll())));

  return menu;
}




void tSongSetViewManager::noticeSongSetModified(bool substantial_change, tProgress *progress)
{
  redisplay(!substantial_change, progress);
  emit notifySongSetChanged();
}




void tSongSetViewManager::redisplay(bool preserve_scroll_pos, tProgress *progress)
{
  if (songSet())
  {
    tSongList list;
    songSet()->render(list, progress);
    ListView.setSongList(list, preserve_scroll_pos);
  }
  else
    ListView.setSongList(tSongList(), false);
}





void tSongSetViewManager::fieldChanged(tSong *song, tSongField field, const QString &new_text)
{
  tDeferredSongRetag *retag = new tDeferredSongRetag;
  retag->Song = song;
  retag->Retag.NewText = new_text;
  retag->Retag.Field = field;
  QTimer::singleShot(0, retag, SLOT(execute()));

  LastRetagValid = true;
  LastRetag = retag->Retag;
}




void tSongSetViewManager::contextMenuRequested(tSong *song, tSongField field, const QPoint &pos)
{
  Menu->popup(pos);
}




void tSongSetViewManager::slotEditThisTag()
{
  ListView.editCell(ListView.currentRow(), ListView.currentColumn(), false);
}




void tSongSetViewManager::slotPlayNow()
{
  try
  {
    tSongList list;
    ListView.getSelection(list);
    tProgramBase::preferences().Player.playNow(list);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::slotPlayNow(tSong *item)
{
  slotPlayNow();
}




void tSongSetViewManager::slotPlayNext()
{
  try
  {
    tSongList list;
    ListView.getSelection(list);
    
    tProgramBase::preferences().Player.playNext(list);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::slotPlayEventually()
{
  try
  {
    tSongList list;
    ListView.getSelection(list);
    
    tProgramBase::preferences().Player.playEventually(list);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::slotAutoDJ()
{
  try
  {
    if (songSet() == NULL)
      return;

    tSongList songs;

    tAutoDJ dj(tProgramBase::preferences().AutoDJPreferences, songSet());
    dj.selectSongs(songs, 20);

    tProgramBase::preferences().Player.playEventually(songs);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Auto DJ encountered an error:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::rate(int rating)
{
  if (songSet())
  {
    tSongList list;
    ListView.getSelection(list);

    if (list.size() > 5)
      songSet()->beginBulkChange();

    FOREACH(first, list, tSongList)
      (*first)->setRating(rating);

    if (list.size() > 5)
      songSet()->endBulkChange();
  }
}




void tSongSetViewManager::restrictViewByCriterion(const QString &crit)
{
  emit notifySearchChangeRequested(crit, true);
}




void tSongSetViewManager::playByCriterion(const QString &crit, tPlayWhen when)
{
  try
  {
    tPlaylist set;
    set.setSongCollection(&tProgramBase::database().SongCollection);
    set.setCriterion(crit);
    set.reevaluateCriterion();

    tSongList list;
    set.render(list);

    sortBeforePlay(list);

    switch (when)
    {
    case PLAY_NOW:
      tProgramBase::preferences().Player.playNow(list);
      return;
    case PLAY_NEXT:
      tProgramBase::preferences().Player.playNext(list);
      return;
    case PLAY_EVENTUALLY:
      tProgramBase::preferences().Player.playEventually(list);
      return;
    }
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::resetStatistics()
{
  if (songSet())
  {
    tSongList list;
    ListView.getSelection(list);

    if (list.size() > 5)
      songSet()->beginBulkChange();

    FOREACH(first, list, tSongList)
      (*first)->resetStatistics();

    if (list.size() > 5)
      songSet()->endBulkChange();
  }
}




void tSongSetViewManager::deleteFromDisk()
{
  try
  {
    tSongList list;
    ListView.getSelection(list);

    if (QMessageBox::warning(&ListView, tr("File Deletion"),
	  tr("OK to physically delete %1 files?").arg(list.size()),
	  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
      tProgramBase::database().SongCollection.beginBulkChange();
      FOREACH_CONST(first, list, tSongList)
      {
	if (tProgramBase::database().SongCollection.isValid(*first))
	{
	  tProgramBase::database().SongCollection.remove(*first);
	  unlink((*first)->filename().c_str());
	}
      }
      tProgramBase::database().SongCollection.endBulkChange();
    }
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Error while deleting file:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::highlightCurrentSong()
{
  try
  {
    tFilename current_song = tProgramBase::preferences().Player.currentFilename();
    tSong *song = tProgramBase::database().SongCollection.getByFilename(current_song);
    if (song)
      highlight(song);
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::selectAll()
{
  ListView.clearSelection(false);
  ListView.selectCells(0, 0, ListView.numRows(), ListView.numCols());
}




void tSongSetViewManager::autoTag()
{
  tSongList list;
  ListView.getSelection(list);
  showAutoTagger(ListView.topLevelWidget(), list, tProgramBase::database());
}




void tSongSetViewManager::redoRetag()
{
  if (LastRetagValid)
  {
    tSongList list;
    ListView.getSelection(list);

    if (QMessageBox::warning(&ListView, tr("madman"),
	  tr("OK to set the '%1' column of %2 songs to '%3'?")
	  .arg(getFieldName(LastRetag.Field))
	  .arg(list.size())
	  .arg(LastRetag.NewText) ,
	  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
      QProgressDialog progress(tr("Retagging songs..."), tr("Cancel"), 
	  list.size(), &ListView, "progress", true);
      unsigned int index = 0;
      FOREACH_CONST(first, list, tSongList)
      {
	progress.setProgress(index);
	if (progress.wasCancelled())
	  break;

	LastRetag.executeOn(*first);
	index++;
      }
    }
  }
  else
    QMessageBox::information(&ListView, tr("madman"),
	tr("There was no previous retag. Sorry."), QMessageBox::Ok);
}




void tSongSetViewManager::rereadTag()
{
  tSongList list;
  ListView.getSelection(list);

  try
  {
    FOREACH_CONST(first, list, tSongList)
    {
      (*first)->invalidateCache();
      (*first)->ensureInfoIsThere();
    }
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Error while rereading tag:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::rewriteTag()
{
  tSongList list;
  ListView.getSelection(list);

  try
  {
    FOREACH_CONST(first, list, tSongList)
      (*first)->rewriteTag();
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Error while rewriting tag:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::stripTag()
{
  tSongList list;
  ListView.getSelection(list);

  try
  {
    FOREACH_CONST(first, list, tSongList)
      (*first)->stripTag();
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Error while stripping tag:\n%1").arg(ex.what()), QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tSongSetViewManager::highlight(tSong *song)
{
  ListView.highlightSong(song);
}




void tSongSetViewManager::pluginActivated(int id)
{
  try
  {
    tPluginList plugins;
    enumeratePlugins(tProgramBase::preferences().PluginDirectories, plugins);
    hash_map<int,QString>::iterator it = MenuIdToPluginFilenameMap.find(id);
    if (it != MenuIdToPluginFilenameMap.end())
    {
      QString name = it->second;

      tSongList selection;
      ListView.getSelection(selection);

      FOREACH(first, plugins, tPluginList)
	if (first->Filename == name)
	{
	  bool confirmed = !first->Confirm || QMessageBox::warning(&ListView, tr("madman"),
		tr("Are you sure you want to run plugin '%1' on %2 songs?")
		.arg(first->Name).arg(selection.size()),
		QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;

	  if (confirmed)
	  {
	    first->run(tProgramBase::settings(), selection);
	    if (first->RescanAfter)
	    {
              try
              {
                auto_ptr<tProgressDialog> progress(
                  new tProgressDialog(ListView.topLevelWidget(), 
                                      false));
                tProgramBase::database().SongCollection.scan(
                  tProgramBase::database().DirectoryList, progress.get());
              }
              catch (exception &ex)
              {
                QMessageBox::warning(
                  ListView.topLevelWidget(), 
                  tr("madman"),
                  tr("Error while rescanning:\n%1").arg(ex.what()), 
                  QMessageBox::Ok, QMessageBox::NoButton);
              }
	    }
	  }
	  return;
	}
    }
    else
      throw tRuntimeError(tr("This plugin has mysteriously disappeared"));
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"), 
	ex.what(), QMessageBox::Ok, 0);
  }
}




void tSongSetViewManager::selectionChanged()
{
  tSongList list;
  ListView.getSelection(list);
  tProgramBase::setStatus(stringifySongListSummary(list));
}




QString tSongSetViewManager::getCurrentArtistCriterion()
{
  tSong *hlsong = ListView.highlightedSong();
  if (hlsong == NULL)
    return QString::null;

  return QString("~artist(complete:%1)").arg(quoteString(hlsong->artist()));
}




QString tSongSetViewManager::getCurrentAlbumCriterion()
{
  tSong *hlsong = ListView.highlightedSong();
  if (hlsong == NULL)
    return QString::null;

  return QString("~artist(complete:%1)&~album(complete:%2)")
    .arg(quoteString(hlsong->artist()))
    .arg(quoteString(hlsong->album()));
}




QString tSongSetViewManager::getCurrentDirectoryCriterion()
{
  tSong *hlsong = ListView.highlightedSong();
  if (hlsong == NULL)
    return QString::null;

  return QString("~pathname(complete:%1)")
    .arg(quoteString(QFile::decodeName(hlsong->pathname().c_str())));
}




// tPlaylistViewManager -------------------------------------------------------
tPlaylistViewManager::tPlaylistViewManager(tSongListView &lv, const QString & pref_prefix)
: tSongSetViewManager(lv, pref_prefix), Playlist(NULL)
{
  ListView.setSorting(true);
  ListView.setSongDropEnable(true);

  connect(&ListView, SIGNAL(songsDropped(bool,tSong *,vector<tFilename> &)),
	this, SLOT(songsDropped(bool,tSong *,vector<tFilename> &)));
}




QPopupMenu *tPlaylistViewManager::buildPopupMenu()
{
  QPopupMenu *menu = super::buildPopupMenu();

  menu->insertItem(tr("Remo&ve from playlist (Del)"), this, SLOT(removeSelected()));
  QPopupMenu *reorder_menu = new QPopupMenu(&ListView);
  reorder_menu->insertItem(tr("Sort by &Artist"), this, SLOT(sortByArtist()));
  reorder_menu->insertItem(tr("Sort by &Title"), this, SLOT(sortByTitle()));
  reorder_menu->insertItem(tr("Sort by A&lbum"), this, SLOT(sortByAlbum()));
  reorder_menu->insertItem(tr("Sort by &Track number"), this, SLOT(sortByTrackNumber()));
  reorder_menu->insertItem(tr("Sort by &Year"), this, SLOT(sortByYear()));
  reorder_menu->insertItem(tr("Sort by &Genre"), this, SLOT(sortByGenre()));
  reorder_menu->insertSeparator();
  reorder_menu->insertItem(tr("Jumble"), this, SLOT(jumble()));
  reorder_menu->insertItem(tr("Reverse"), this, SLOT(reverse()));
  menu->insertSeparator();
  menu->insertItem(tr("&Reorder"), reorder_menu);
  menu->insertSeparator();
  menu->insertItem(tr("Export to player's current playlist"), this, SLOT(uploadPlaylist()));
  menu->insertItem(tr("Import player's current playlist"), this, SLOT(downloadPlaylist()));
  menu->insertItem(tr("Import M3U"), this, SLOT(importM3U()));

  ListView.addShortCut(new tKeyboardShortCut(Key_Delete, this, SLOT(removeSelected())));

  return menu;
}




void tPlaylistViewManager::setSongSet(tPlaylist *pl)
{
  Playlist = pl;
  changeSongSet(Playlist, pl);
}




void tPlaylistViewManager::removeSelected()
{
  if (Playlist == NULL)
    return;

  tIndexList list;
  ListView.getSelection(list);
  sort(list.begin(), list.end());
  ::std::reverse(list.begin(), list.end());

  FOREACH(first, list, tIndexList)
    Playlist->remove(*first);
}




void tPlaylistViewManager::uploadPlaylist()
{
  try
  {
    if (Playlist)
    {
      tSongList rendering;
      Playlist->render(rendering);
      tProgramBase::preferences().Player.setPlayList(rendering);
    }
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tPlaylistViewManager::downloadPlaylist()
{
  try
  {
    if (Playlist == NULL)
      return;
    
    vector<tFilename> playlist;
    tProgramBase::preferences().Player.getPlayList(playlist);
    
    FOREACH(first, playlist, vector<tFilename>)
      {
        try
        {
          Playlist->add(*first);
        }
        catch (...)
        {
        }
      }
  }
  catch (exception &ex)
  {
    QMessageBox::warning(&ListView, tr("madman"),
	tr("Player encountered an error:\n%1").arg(ex.what()),
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}




void tPlaylistViewManager::importM3U()
{
  QString filename = QFileDialog::getOpenFileName(QString::null,
      qApp->translate("importM3UIntoPlaylist", "Playlist (*.m3u)"),
      qApp->mainWidget());

  if (filename.isNull())
    return;

  importM3UIntoPlaylist(Playlist, filename);
}




void tPlaylistViewManager::sortBy(tSongField field)
{
  if (Playlist)
    Playlist->sortBy(field,
        tProgramBase::preferences().SortingPreferences.SecondarySortField[field],
        tProgramBase::preferences().SortingPreferences.TertiarySortField[field]
        );
}




void tPlaylistViewManager::jumble()
{
  if (Playlist)
    Playlist->jumble();
}




void tPlaylistViewManager::reverse()
{
  if (Playlist)
    Playlist->reverse();
}




void tPlaylistViewManager::songsDropped(bool from_same_widget, tSong *onto, vector<tFilename> &songs)
{
  if (Playlist == NULL)
    return;

  int preferred_index = -1;

  Playlist->beginBulkChange();

  if (from_same_widget)
  {
    removeSelected();
  }

  if (onto)
  {
    tSongList rendering;
    Playlist->render(rendering);

    int index = 0;
    FOREACH_CONST(first, rendering, tSongList)
    {
      if (*first == onto)
      {
	preferred_index = index;
	break;
      }
      index++;
    }
  }

  FOREACH_CONST(first, songs, vector<tFilename>)
  {
    try
    {
      Playlist->add(*first, preferred_index);
      if (preferred_index != -1)
	++preferred_index;
    }
    catch (exception &ex)
    {
      qWarning("%s", ex.what());
    }
  }

  Playlist->endBulkChange();
}




void tPlaylistViewManager::sortingChanged(tSongField field, bool ascending)
{
  if (Playlist)
  {
    Playlist->sortBy(field, 
        tProgramBase::preferences().SortingPreferences.SecondarySortField[field],
        tProgramBase::preferences().SortingPreferences.TertiarySortField[field],
        ascending);
  }
}




// tSearchViewManager ---------------------------------------------------------
tSearchViewManager::tSearchViewManager(tSongListView &lv, const QString & pref_prefix )
: tSongSetViewManager(lv, pref_prefix), SearchSongSet(NULL),
  DoDisplaySort(true), SortField(FIELD_ARTIST), Ascending(true)
{
  ListView.setSorting(true);
}




void tSearchViewManager::saveListViewAppearance(QSettings &settings)
{
  settings.writeEntry(QString("%1/listview.do_display_sort")
      .arg(PreferencesPrefix), DoDisplaySort ? 1 : 0);
  settings.writeEntry(QString("%1/listview.sort_field")
      .arg(PreferencesPrefix), SortField);
  settings.writeEntry(QString("%1/listview.ascending")
      .arg(PreferencesPrefix), Ascending ? 1 : 0);

  super::saveListViewAppearance(settings);
}




void tSearchViewManager::loadListViewAppearance(QSettings &settings)
{
  DoDisplaySort = settings.readNumEntry(QString("%1/listview.do_display_sort")
      .arg(PreferencesPrefix), 1) != 0;
  SortField = (tSongField) settings.readNumEntry(QString("%1/listview.sort_field")
      .arg(PreferencesPrefix), FIELD_ARTIST);
  Ascending = settings.readNumEntry(QString("%1/listview.ascending")
      .arg(PreferencesPrefix), 1) != 0;

  super::loadListViewAppearance(settings);
}




void tSearchViewManager::setSongSet(tSearchSongSet *ss)
{
  changeSongSet(SearchSongSet, ss);
  SearchSongSet = ss;
  ss->setSort(DoDisplaySort, Ascending,
      SortField, 
      tProgramBase::preferences().SortingPreferences.SecondarySortField[SortField],
      tProgramBase::preferences().SortingPreferences.TertiarySortField[SortField]
      );
}




QPopupMenu *tSearchViewManager::buildPopupMenu()
{
  QPopupMenu *menu = super::buildPopupMenu();

  menu->insertSeparator();
  menu->insertItem(tr("&Add to current playlist (Ins)"), this, SLOT(slotAdd()));

  ListView.addShortCut(new tKeyboardShortCut(Key_Insert, this, SLOT(slotAdd())));

  return menu;
}




void tSearchViewManager::redisplay(bool preserve_scroll_pos, tProgress *progress)
{
  ListView.setSortIndicator(DoDisplaySort, SortField, Ascending);

  super::redisplay(preserve_scroll_pos, progress);
}




void tSearchViewManager::slotAdd()
{
  tSongList list;
  ListView.getSelection(list);
  emit add(list);
}




void tSearchViewManager::sortingChanged(tSongField field, bool ascending)
{
  DoDisplaySort = true;
  SortField = field;
  Ascending = ascending;

  if (SearchSongSet)
    SearchSongSet->setSort(DoDisplaySort, ascending,
        field, 
        tProgramBase::preferences().SortingPreferences.SecondarySortField[field],
        tProgramBase::preferences().SortingPreferences.TertiarySortField[field]
        );
}




void tSearchViewManager::sortBeforePlay(tSongList &list)
{
  if (DoDisplaySort)
  {
    sort(list, SortField, tProgramBase::preferences());
    if (!Ascending)
      std::reverse(list.begin(), list.end());
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
