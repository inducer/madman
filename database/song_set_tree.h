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




#ifndef SONG_SET_TREE
#define SONG_SET_TREE




#include "song_set.h"




class tDatabase;

class tPlaylistNode
{
    QString Name;
    tPlaylistNode *Parent;
    typedef vector<tPlaylistNode *> tChildrenList;
    tChildrenList ChildrenList;
    tPlaylist *Data;
    tDatabase *Database;

  public:
    typedef tChildrenList::iterator iterator;

    tPlaylistNode(tDatabase *db, tPlaylist *data);
    ~tPlaylistNode();

    iterator begin()
    {
      return ChildrenList.begin();
    }
    iterator end()
    {
      return ChildrenList.end();
    }

    tPlaylistNode *parent();
    void makeChildNodeNameUnique(tPlaylistNode *node);
    void addChild(tPlaylistNode *node);
    void insertChild(tPlaylistNode *node, iterator it);
    void removeChild(tPlaylistNode *node);
    void clear();

    QString qualifiedName();
    tPlaylistNode *resolve(const QString &name);
    bool hasParent(tPlaylistNode *node);

    QString const &name();
    void setName(QString const &name);
    tPlaylist *data();
    void setData(tPlaylist *data);

    QDomNode serialize(QDomDocument &doc);
    
  private:
    // make this type noncopyable
    tPlaylistNode(const tPlaylistNode &src);
    tPlaylistNode &operator=(const tPlaylistNode &src);
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
