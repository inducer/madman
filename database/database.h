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




#ifndef DATABASE_H_SEEN
#define DATABASE_H_SEEN




#include "utility/noncopyable.h"
#include "database/song_collection.h"
#include "database/song_set.h"
#include "database/song_set_tree.h"
#include "database/history.h"




class tProgress;




class tDatabase : public QObject, public tNoncopyable
{
    Q_OBJECT

    auto_ptr<tPlaylistNode> PlaylistTree;

  public:
    tDirectoryList DirectoryList;
    tSongCollection SongCollection;
    tHistory History;

    tDatabase();

    tPlaylistNode *playlistTree()
    {
      return PlaylistTree.get();
    }
    void setPlaylistTree(tPlaylistNode *node);

    void clear();
    void startNew();
    void load(const QString &filename, tProgress *progress = NULL);
    void save(const QString &filename, tProgress *progress = NULL);

    void noticePlaylistTreeChanged();

  signals:
    void notifyPlaylistTreeChanged();
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
