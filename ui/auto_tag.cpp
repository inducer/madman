/*
madman - a music manager
Copyright (C) 2004  Andreas Kloeckner <ak@ixion.net>

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




#include <qapplication.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qheader.h>
#include <qlistview.h>
#include <qprogressbar.h>
#include <qcombobox.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qinputdialog.h>
#include <qregexp.h>
#include <qtimer.h>

#include "database/database.h"
#include "database/song.h"
#include "database/song_set.h"
#include "ui/auto_tag.h"
#include "ui/stock.h"
#include "ui/progress_impl.h"
#include "designer/auto_tag.h"




namespace
{
  struct tSongChange;
  
  struct tTagChange
  {
    tSongField Field;
    QString NewValue;
  };

  typedef vector<tTagChange> tTagChangeList;

  struct tSongChange
  {
    tSong *Song;
    QString Message;
    tTagChangeList ProposedChanges;

    const QString &proposedChange(tSongField field)
    {
      FOREACH(first, ProposedChanges, tTagChangeList)
	if (first->Field == field)
	  return first->NewValue;
      return QString::null;
    }

    void proposeChange(tSongField field, const QString &new_value, bool overwrite = false)
    {
      FOREACH(first, ProposedChanges, tTagChangeList)
      {
	if (first->Field == field)
	{
	  if (overwrite)
	    first->NewValue = new_value;
	  return;
	}
      }

      tTagChange tch;
      tch.Field = field;
      tch.NewValue = new_value;
      ProposedChanges.push_back(tch);
    }
  };

  typedef vector<tSongChange> tSongChangeList;




  class tAutoTagAlgorithm
  {
    public:
      virtual ~tAutoTagAlgorithm() { }
      virtual QString name() = 0;
      virtual void reset() { }
      virtual void familiarize(tSong *song) { }
      virtual void generateChange(tSongChange &ch) = 0;
  };




  class tArtistTitleAutoTagAlgorithm : public tAutoTagAlgorithm
  {
    public:
      QString name()
      {
	return qApp->translate("AutoTag", "Artist - Title");
      }

      void generateChange(tSongChange &ch)
      {
        QRegExp dash_pattern("^([^-]+)\\s*\\-\\s*(.+)\\.[a-z0-9]+$");
        dash_pattern.setCaseSensitive(false);
        dash_pattern.setMinimal(true);

        if (dash_pattern.exactMatch(QString::fromUtf8(ch.Song->filenameOnly().c_str())))
        {
          ch.proposeChange(FIELD_ARTIST, dash_pattern.cap(1).stripWhiteSpace());
          ch.proposeChange(FIELD_TITLE, dash_pattern.cap(2).stripWhiteSpace());
        }
      }
  };

  class tTitleArtistAutoTagAlgorithm : public tAutoTagAlgorithm
  {
    public:
      QString name()
      {
	return qApp->translate("AutoTag", "Title - Artist");
      }

      void generateChange(tSongChange &ch)
      {
        QRegExp dash_pattern("^([^-]+)\\s*\\-\\s*(.+)\\.[a-z0-9]+$");
        dash_pattern.setCaseSensitive(false);
        dash_pattern.setMinimal(true);

        if (dash_pattern.exactMatch(QString::fromUtf8(ch.Song->filenameOnly().c_str())))
        {
          ch.proposeChange(FIELD_TITLE, dash_pattern.cap(1).stripWhiteSpace());
          ch.proposeChange(FIELD_ARTIST, dash_pattern.cap(2).stripWhiteSpace());
        }
      }
  };

  class tIntelligentAutoTagAlgorithm : public tAutoTagAlgorithm
  {
      class tCaseInsensitiveEqual
      {
          QString Operand;
        public:
          tCaseInsensitiveEqual(const QString &str)
            : Operand(str.lower())
            { }
          bool operator()(const QString &op2)
          {
            return op2.lower() == Operand;
          }
      };

      tDatabase &Database;

      typedef hash_map<QString, unsigned, hash_QString> tComponentCounts;
      tComponentCounts	DirComponentCounts, NameComponentCounts;

    public:
      tIntelligentAutoTagAlgorithm(tDatabase &db)
      : Database(db)
      {
      }

      QString name()
      {
	return qApp->translate("AutoTag", "Intelligent");
      }

      static void getComponents(const QString &filename, vector<QString> &dir_comp, vector<QString> &name_comp)
      {
	QFileInfo info(filename);
	QString basename_only = info.baseName(true);

	QDir dir(info.dirPath());

	if (dir.dirName().length() >= 1)
	  dir_comp.push_back(dir.dirName());

	dir.cdUp();

	if (dir.dirName().length() >= 1)
	  dir_comp.push_back(dir.dirName());

	if (basename_only.find(" - ") != -1)
          split(" - ", basename_only, name_comp);
	else if (basename_only.find(" _ ") != -1)
          split(" _ ", basename_only, name_comp);
	else if (basename_only.find("_") != -1)
          split("_", basename_only, name_comp);
	else if (basename_only.find("-") != -1)
          split("-", basename_only, name_comp);
	else
	  name_comp.push_back(basename_only);

        FOREACH(first, name_comp, vector<QString>)
        {
          vector<QString> copy;
          remove_copy_if(dir_comp.begin(), dir_comp.end(), back_inserter(copy), tCaseInsensitiveEqual(*first));
          dir_comp = copy;
        }
      }

      void reset()
      {
        NameComponentCounts.clear();
      }

      void familiarize(tSong *song) 
      { 
	vector<QString> dir_components,name_components;
	getComponents(QString::fromUtf8(song->filename().c_str()), dir_components, name_components);

	FOREACH(first, dir_components, vector<QString>)
	{
	  if (DirComponentCounts.find(*first) != DirComponentCounts.end())
	    ++DirComponentCounts[ *first ];
	  else
	    DirComponentCounts[ *first ] = 1;
	}

	FOREACH(first, name_components, vector<QString>)
	{
	  if (NameComponentCounts.find(*first) != NameComponentCounts.end())
	    ++NameComponentCounts[ *first ];
	  else
	    NameComponentCounts[ *first ] = 1;
	}
      }

      bool isKnownAs(tSongField field, const QString &value)
      {
	tSearchSongSet set;
	set.setSongCollection(&Database.SongCollection);
	set.setCriterion(
	    QString("~%1(complete:%2)")
	    .arg(getFieldIdentifier(field))
	    .arg(quoteString(value)));
	set.reevaluateCriterion();

	tSongList list;
	set.render(list);
	return list.size() != 0;
      }

      void generateChange(tSongChange &ch)
      {
	ch.ProposedChanges.clear();

	vector<QString> dir_components,name_components;
	getComponents(QString::fromUtf8(ch.Song->filename().c_str()), 
            dir_components, name_components);

	{
	  // check for "definite knowledge" first
	  vector<QString>::iterator first = name_components.begin(), last = name_components.end();
	  while (first != last)
	  {
	    bool ok,processed = true;
	    (*first).toUInt(&ok);
	    if (ok)
	      ch.proposeChange(FIELD_TRACKNUMBER, *first);
	    else if (isKnownAs(FIELD_ARTIST, *first))
	      ch.proposeChange(FIELD_ARTIST, *first);
	    else if (isKnownAs(FIELD_ALBUM, *first))
	      ch.proposeChange(FIELD_ALBUM, *first);
	    else
	      processed = false;

	    if (processed)
	    {
	      first = name_components.erase(first);
	      last = name_components.end();
	    }
	    else
	      first++;
	  }
	}

	{
	  // check for "definite knowledge" on directories first
	  vector<QString>::iterator first = dir_components.begin(), last = dir_components.end();
	  while (first != last)
	  {
	    bool processed = true;

	    if (isKnownAs(FIELD_ARTIST, *first))
	      ch.proposeChange(FIELD_ARTIST, *first);
	    else if (isKnownAs(FIELD_GENRE, *first))
	      ch.proposeChange(FIELD_GENRE, *first);
	    else if (isKnownAs(FIELD_ALBUM, *first))
	      ch.proposeChange(FIELD_ALBUM, *first);
	    else
	      processed = false;

	    if (processed)
	    {
	      first = dir_components.erase(first);
	      last = dir_components.end();
	    }
	    else
	      first++;
	  }
	}

	{
	  // a name component that occurs multiple times will likely be an artist,
	  // but could also be an album
	  vector<QString>::iterator first = name_components.begin(), last = name_components.end();
	  while (first != last)
	  {
	    bool processed = false;

	    if (NameComponentCounts.find(*first) != NameComponentCounts.end())
	    {
	      if (NameComponentCounts[ *first ] >= 2)
	      {
		ch.proposeChange(FIELD_ARTIST, name_components[ 0 ]);
		processed = true;
	      }
	    }

	    if (processed)
	    {
	      first = name_components.erase(first);
	      last = name_components.end();
	    }
	    else
	      first++;
	  }
	}

	if (name_components.size() >= 3 )
	  ch.Message = 
            qApp->translate("tAutoTagDialog", 
                "This song can't be meaningfully tagged automatically.");

	// just consume the rest
	while (name_components.size())
	{
	  if (ch.proposedChange(FIELD_ARTIST).isNull() && name_components.size() > 1)
	    ch.proposeChange(FIELD_ARTIST, name_components[ 0 ]);
          else if (ch.proposedChange(FIELD_TITLE).isNull())
	    ch.proposeChange(FIELD_TITLE, name_components[ 0 ]);
	  else if (ch.proposedChange(FIELD_ARTIST).isNull())
	    ch.proposeChange(FIELD_ARTIST, name_components[ 0 ]);
	  else if (ch.proposedChange(FIELD_ALBUM).isNull())
	    ch.proposeChange(FIELD_ALBUM, name_components[ 0 ]);
	  else if (ch.proposedChange(FIELD_PERFORMER).isNull())
	    ch.proposeChange(FIELD_PERFORMER, name_components[ 0 ]);

	  name_components.erase(name_components.begin());
	}

	while (dir_components.size())
	{
	  if (ch.proposedChange(FIELD_ARTIST).isNull())
	    ch.proposeChange(FIELD_ARTIST, dir_components[ 0 ]);
	  else if (ch.proposedChange(FIELD_ALBUM).isNull())
	    ch.proposeChange(FIELD_ALBUM, dir_components[ 0 ]);

	  dir_components.erase(dir_components.begin());
	}
      }
  };




  class tTagChangeListViewItem : public QListViewItem
  {
    public:
      tUniqueId UniqueId;
      tSongField Field;

      tTagChangeListViewItem(QListViewItem *parent, tTagChange tch, tSong *song)
      : QListViewItem(parent, 
	  QString("%1 : %2")
	  .arg(getFieldName(tch.Field))
	  .arg(tch.NewValue)), UniqueId(song->uniqueId()),
          Field(tch.Field)
      {
      }
  };




  class tAutoTagDialog : public tAutoTagDialogBase
  {
      Q_OBJECT;
      const tSongList	&Songs;

      static QString LastTaggingAlgorithm;

      typedef vector<tAutoTagAlgorithm *> tAutoTagAlgorithmList;
      tAutoTagAlgorithmList AutoTagAlgorithmList;
      tSongChangeList SongChanges;

    public:
      tAutoTagDialog(QWidget *parent, const tSongList &songs, tDatabase &database)
      : Songs(songs)
      {
	setStockIcon(btnEditTag, "edit.png");
	setStockIcon(btnRemoveTag, "remove.png");

	AutoTagAlgorithmList.push_back(new tIntelligentAutoTagAlgorithm(database));
	AutoTagAlgorithmList.push_back(new tArtistTitleAutoTagAlgorithm());
	AutoTagAlgorithmList.push_back(new tTitleArtistAutoTagAlgorithm());

	listPreview->header()->hide();
	listPreview->setSorting(-1);

	FOREACH(first, AutoTagAlgorithmList, tAutoTagAlgorithmList)
	  comboAlgorithm->insertItem((*first)->name());
        if (!LastTaggingAlgorithm.isNull())
        comboAlgorithm->setCurrentText(LastTaggingAlgorithm);

	connect(btnEditTag, SIGNAL(clicked()),
	    this, SLOT(editTag()));
	connect(btnRemoveTag, SIGNAL(clicked()),
	    this, SLOT(removeTag()));
        
	connect(btnPreview, SIGNAL(clicked()),
	    this, SLOT(preview()));
	connect(btnApply, SIGNAL(clicked()),
	    this, SLOT(apply()));
	connect(btnClear, SIGNAL(clicked()),
	    this, SLOT(clear()));
	connect(listPreview, SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),
	    this, SLOT(doubleClicked(QListViewItem *,const QPoint &,int)));
      }

      ~tAutoTagDialog()
      {
        LastTaggingAlgorithm = comboAlgorithm->currentText();
	FOREACH(first, AutoTagAlgorithmList, tAutoTagAlgorithmList)
	  delete *first;
      }

      tAutoTagAlgorithm *getAlgorithm(const QString &name)
      {
	FOREACH(first, AutoTagAlgorithmList, tAutoTagAlgorithmList)
	  if ((*first)->name() == name)
	    return *first;
	return NULL;
      }

      tAutoTagAlgorithm *getAlgorithm()
      {
	return getAlgorithm(comboAlgorithm->currentText());
      }

    public slots:
      void clear()
      {
	SongChanges.clear();
	updateSongChanges();
	btnApply->setEnabled(false);
      }

      void preview()
      {
        auto_ptr<tProgress> progress(new tProgressDialog(this, true));
        progress->setWhat(tr("Generating tag changes..."));

        SongChanges.clear();

        tAutoTagAlgorithm *algorithm = getAlgorithm();

        progress->setTotalSteps(Songs.size() * 2);
        unsigned index = 0;
        algorithm->reset();
        FOREACH_CONST(first, Songs, tSongList)
        {
          algorithm->familiarize(*first);

          index++;
          progress->setProgress(index);

          if (progress->wasCancelled())
          {
            clear();
            return;
          }
        }

        FOREACH_CONST(first, Songs, tSongList)
        {
          tSongChange ch;
          ch.Song = *first;
          algorithm->generateChange(ch);
          if (!ch.Message.isNull() || ch.ProposedChanges.size())
            SongChanges.push_back(ch);

          index++;
          progress->setProgress(index);

          if (progress->wasCancelled())
          {
            clear();
            return;
          }
        }

        updateSongChanges();
        btnApply->setEnabled(true);
      }

      void updateSongChangesKeepScroll()
      {
        int contents_x = listPreview->contentsX();
        int contents_y = listPreview->contentsY();

        updateSongChanges();

        listPreview->setContentsPos(contents_x, contents_y);
      }

      void updateSongChanges()
      {
	listPreview->clear();
	FOREACH(first, SongChanges, tSongChangeList)
	{
	  QListViewItem *song_item = new QListViewItem(listPreview,
              QString::fromUtf8(first->Song->filenameOnly().c_str()));
	  if (!first->Message.isNull())
	    new QListViewItem(song_item, first->Message);
	  new QListViewItem(song_item, tr("Path to song: %1").arg(
                QString::fromUtf8(first->Song->pathname().c_str())));

	  FOREACH(first_t, first->ProposedChanges, tTagChangeList)
	    new tTagChangeListViewItem(song_item, *first_t, first->Song);

	  song_item->setOpen(true);
	}
      }

      void apply()
      {
        auto_ptr<tProgress> progress(new tProgressDialog(this, true));
        progress->setWhat(tr("Applying tag changes..."));

        progress->setTotalSteps(SongChanges.size());
        unsigned int index = 0;
        FOREACH(first, SongChanges, tSongChangeList)
        {
          progress->setProgress(index++);
          FOREACH(first_ch, first->ProposedChanges, tTagChangeList)
            first->Song->setFieldText(first_ch->Field, first_ch->NewValue);

          if (progress->wasCancelled())
            return;
        }

        clear();
      }

      tTagChangeList::iterator findCurrentTagChange(tSongChange *&sch)
      {
        QListViewItem *item = listPreview->selectedItem();
        if (item == NULL)
          throw runtime_error("current tag change not found");
        tTagChangeListViewItem *t_item = dynamic_cast<tTagChangeListViewItem *>(item);
        if (t_item == NULL)
          throw runtime_error("current tag change not found");

        FOREACH(first, SongChanges, tSongChangeList)
        {
          if (first->Song->uniqueId() == t_item->UniqueId)
            FOREACH(first_t, first->ProposedChanges, tTagChangeList)
              if (first_t->Field == t_item->Field)
              {
                sch = &*first;
                return first_t;
              }
        }
        throw runtime_error("current tag change not found");
      }

      void doubleClicked(QListViewItem *, const QPoint &, int)
      {
        editTag();
      }

      void editTag()
      {
        tTagChangeList::iterator it;
        tSongChange *sch;

        try
        {
          it = findCurrentTagChange(sch);
        }
        catch (...)
        {
          return;
        }

        bool ok;
        QString result = QInputDialog::getText(
            tr("madman"), 
            tr("New value for the '%1' field:").arg(getFieldName(it->Field)),
            QLineEdit::Normal,
            it->NewValue, &ok, this);
        if (ok)
        {
          it->NewValue = result;
          QTimer::singleShot(0, this, SLOT(updateSongChangesKeepScroll()));
        }
      }
      
      void removeTag()
      {
        tTagChangeList::iterator it;
        tSongChange *sch;
        try
        {
          it = findCurrentTagChange(sch);
        }
        catch (...)
        {
          return;
        }

        sch->ProposedChanges.erase(it);
        QTimer::singleShot(0, this, SLOT(updateSongChangesKeepScroll()));
      }
  };
  
  QString tAutoTagDialog::LastTaggingAlgorithm;
}




void showAutoTagger(QWidget *parent, const tSongList &songs, tDatabase &database)
{
  tAutoTagDialog adlg(parent, songs, database);
  adlg.exec();
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
#include "auto_tag.moc"
