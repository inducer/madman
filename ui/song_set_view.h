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




#ifndef SONG_SET_VIEW
#define SONG_SET_VIEW




#include <qaction.h>
#include <qpopupmenu.h>
#include <qlistview.h>
#include <qsettings.h>
#include "database/song_set.h"
#include "ui/accel_list_view.h"




class tPreferences;
class tSongSetViewManager;
class tProgramInterface;

typedef vector<int> tIndexList;




class tSongRetag
{
  public:
    tSongField	Field;
    QString	NewText;

    void executeOn(tSong *song);
};




class tDeferredSongRetag : public QObject
{
    Q_OBJECT;

  public:
    tSong	*Song;
    tSongRetag	Retag;

  public slots:
    void execute();
};




class tSongListView;
class tVisibilityAction : public QAction
{
  Q_OBJECT
    tSongListView 	*ListView;
    tSongField	Field;

  public:
    tVisibilityAction(QObject *parent, tSongListView *view, tSongField field);

  public slots:
    void onToggle(bool on);
};




class tSongListView : public tAcceleratorTable 
{
    typedef tAcceleratorTable super;

    Q_OBJECT;

    tSongList	SongList;
    int ShownColumns;
    vector<int> FieldToColumn;
    vector<int> ColumnToField;
    vector<int> FieldWidths;

    QIntDict<QWidget> WidgetDictionary;
    bool SortIndicatorEnable, IndicatorAscending;
    tSongField SortIndicatorField;

    QPopupMenu *FieldVisibilityMenu;

  public:
    tSongListView(QWidget *parent = 0, const char *name = "");

    void initialize();

    void saveListViewAppearance(QSettings &settings, const QString &prefix);
    void loadListViewAppearance(QSettings &settings, const QString &prefix);

    void setup();

    void setSongList(const tSongList &list, bool preserve_scroll_pos);

    tSong *highlightedSong();
    void highlightSong(tSong *song);

    void getSelection(tSongList &songlist) const;
    void getSelection(tIndexList &indexlist) const;

    void setSongDropEnable(bool enabled);
    void setSortIndicator(bool enable, tSongField field, bool ascending);

    void columnWidthChanged(int col);
    void swapColumns (int col1, int col2, bool swapHeader = false);
    void sortColumn (int col, bool ascending = TRUE, bool wholeRows = FALSE);

    bool isFieldVisible(tSongField field);
    void showField(tSongField field);
    void hideField(tSongField field);

  public slots:
    void slotContextMenuRequested (int row, int col, const QPoint & pos);
    void currentSongChanged();

  public:
    // sparse QTable implementation -------------------------------------------
    void resizeData(int) {}

    QTableItem *item(int r, int c);
    void setItem(int r, int c, QTableItem *i);
    void takeItem(QTableItem *item);
    void clearCell(int r, int c);

    void insertWidget(int r, int c, QWidget *w);
    QWidget *cellWidget(int r, int c) const;
    void clearCellWidget(int r, int c);

    QWidget *createEditor (int row, int col, bool initFromCell) const;
    void setCellContentFromEditor (int row, int col);

    void paintCell(QPainter *p, int row, int col, const QRect & cr, bool selected, const QColorGroup & cg);

  public slots:
    void noticeSongModified(const tSong *song, tSongField field);

  protected:
    bool eventFilter (QObject * watched, QEvent * e);
    void contentsDragEnterEvent(QDragEnterEvent *event);
    void contentsDragMoveEvent(QDragMoveEvent *event);
    void contentsDropEvent(QDropEvent *event);
    void contentsMousePressEvent (QMouseEvent *e);
    void contentsMouseReleaseEvent (QMouseEvent *e);

    QDragObject *dragObject();

    void showSortIndicator();

  signals:
    void songsDropped(bool from_same_widget, tSong *onto, vector<tFilename> &songs);
    void fieldChanged(tSong *song, tSongField field, const QString &new_text);
    void contextMenuRequested(tSong *song, tSongField field, const QPoint &pos);
    void doubleClicked(tSong *);
    void sortingChanged(tSongField field, bool ascending);
};




class tSongSetViewManager : public QObject
{
    Q_OBJECT;

  protected:
    tSongListView &ListView;
    auto_ptr<QPopupMenu> Menu;

    bool LastRetagValid;
    tSongRetag LastRetag;

    QString PreferencesPrefix;

    hash_map<int,QString> MenuIdToPluginFilenameMap;

    enum tPlayWhen {
      PLAY_NOW, PLAY_NEXT, PLAY_EVENTUALLY
    };

  public:
    tSongSetViewManager(tSongListView &lv, const QString & pref_prefix);

    virtual void setup();
    virtual void saveListViewAppearance(QSettings &settings);
    virtual void loadListViewAppearance(QSettings &settings);

    virtual tSongSet *songSet() = 0;
    void changeSongSet(tSongSet *from, tSongSet *to);

    virtual QPopupMenu *buildPopupMenu();

  public slots:
    void noticeSongSetModified(bool substantial_change, tProgress *progress);

    virtual void redisplay(bool preserve_scroll_pos, tProgress *progress = NULL);

    void fieldChanged(tSong *item, tSongField field, const QString &new_text);
    void contextMenuRequested(tSong *song, tSongField field, const QPoint &pos);

    void slotEditThisTag();
    void slotPlayNow();
    void slotPlayNow(tSong *song);
    void slotPlayNext();
    void slotPlayEventually();
    void slotAutoDJ();

    void rateNone() { rate(-1); } 
    void rate0() { rate(0); } 
    void rate1() { rate(1); } 
    void rate2() { rate(2); } 
    void rate3() { rate(3); } 
    void rate4() { rate(4); } 
    void rate5() { rate(5); } 
    void rate(int rating);

    void viewByCriterion(const QString &crit);
    void playByCriterion(const QString &crit, tPlayWhen when);

    void playNowAllByThisArtist() { playByCriterion(getCurrentArtistCriterion(), PLAY_NOW); }
    void playNextAllByThisArtist() { playByCriterion(getCurrentArtistCriterion(), PLAY_NEXT); }
    void playEventuallyAllByThisArtist() { playByCriterion(getCurrentArtistCriterion(), PLAY_EVENTUALLY); }
    void viewAllByThisArtist() { viewByCriterion(getCurrentArtistCriterion()); }
    void playNowAllOnThisAlbum() { playByCriterion(getCurrentAlbumCriterion(), PLAY_NOW); }
    void playNextAllOnThisAlbum() { playByCriterion(getCurrentAlbumCriterion(), PLAY_NEXT); }
    void playEventuallyAllOnThisAlbum() { playByCriterion(getCurrentAlbumCriterion(), PLAY_EVENTUALLY); }
    void viewAllOnThisAlbum() { viewByCriterion(getCurrentAlbumCriterion()); }

    void resetStatistics();

    void deleteFromDisk();
    void highlightCurrentSong();
    void selectAll();

    void autoTag();
    void redoRetag();
    void rereadTag();
    void rewriteTag();
    void stripTag();

    void highlight(tSong *song);

    void pluginActivated(int id);

    void selectionChanged();

    virtual void sortingChanged(tSongField field, bool ascending) { }

  signals:
    void notifySearchChangeRequested(const QString &);
    void notifySongSetChanged();

  protected:
    QString getCurrentArtistCriterion();
    QString getCurrentAlbumCriterion();

    friend class tSongListViewItem;
};




class tPlaylistViewManager : public tSongSetViewManager
{
    Q_OBJECT

    typedef tSongSetViewManager super;

    tPlaylist *Playlist;

  public:
    tPlaylistViewManager(tSongListView &lv, const QString & pref_prefix);

    QPopupMenu *buildPopupMenu();

    tSongSet *songSet()
    {
      return Playlist;
    }
    tPlaylist *playlist()
    {
      return Playlist;
    }
    void setSongSet(tPlaylist *pl);

  public slots:
    void removeSelected();

    void uploadPlaylist();
    void downloadPlaylist();
    void importM3U();

    void sortBy(tSongField field);
    void jumble();
    void reverse();

    void sortByArtist() { sortBy(FIELD_ARTIST); }
    void sortByTitle() { sortBy(FIELD_TITLE); }
    void sortByAlbum() { sortBy(FIELD_ALBUM); }
    void sortByTrackNumber() { sortBy(FIELD_TRACKNUMBER); }
    void sortByYear() { sortBy(FIELD_YEAR); }
    void sortByGenre() { sortBy(FIELD_GENRE); }

    void songsDropped(bool from_same_widget, tSong *onto, vector<tFilename> &songs);

    void sortingChanged(tSongField field, bool ascending);
};




class tSearchViewManager : public tSongSetViewManager
{
    Q_OBJECT

    typedef tSongSetViewManager super;

    tSearchSongSet	*SearchSongSet;

    bool DoDisplaySort;
    tSongField SortField;
    bool Ascending;

  public:
    tSearchViewManager(tSongListView &lv, const QString & pref_prefix);

    void saveListViewAppearance(QSettings &settings);
    void loadListViewAppearance(QSettings &settings);

    QPopupMenu *buildPopupMenu();

    tSongSet *songSet()
    {
      return SearchSongSet;
    }
    tSearchSongSet *searchSongSet()
    {
      return SearchSongSet;
    }
    void setSongSet(tSearchSongSet *ss);

    void redisplay(bool preserve_scroll_pos, tProgress *progress = NULL);

  public slots:
    void slotAdd();
    void sortingChanged(tSongField field, bool ascending);

  signals:
    void add(tSongList const &);
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
