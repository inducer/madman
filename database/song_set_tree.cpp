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




#include <stdexcept>
#include <cstdlib>
#include <qapplication.h>

#include "song_set_tree.h"
#include "database/database.h"




tPlaylistNode::tPlaylistNode(tDatabase *db, tPlaylist *data)
: Parent(NULL),  Data(data), Database(db)
{
}




tPlaylistNode::~tPlaylistNode()
{
  clear();
  delete Data;
}




tPlaylistNode *tPlaylistNode::parent()
{
  return Parent;
}




void tPlaylistNode::makeChildNodeNameUnique(tPlaylistNode *node)
{
  QString origname = node->name();
  unsigned count = 0;
  while (resolve(node->name()))
    node->setName(QString("%1 %2").arg(origname).arg(++count));
}




void tPlaylistNode::addChild(tPlaylistNode *node)
{
  if (resolve(node->name()))
    throw tRuntimeError(
      qApp->translate("ErrorMessages", "Playlist name '%1' is not unique, not added.")
      .arg(node->name()) );

  node->Parent = this;
  ChildrenList.push_back(node);
  Database->noticePlaylistTreeChanged();
}




void tPlaylistNode::insertChild(tPlaylistNode *node, iterator it)
{
  if (resolve(node->name()))
    throw tRuntimeError(
	qApp->translate("ErrorMessages", "Playlist name '%1' is not unique, not added.")
	.arg(node->name()) );

  node->Parent = this;
  ChildrenList.insert(it, node);
  Database->noticePlaylistTreeChanged();
}




void tPlaylistNode::removeChild(tPlaylistNode *node)
{
  node->Parent = NULL;
  FOREACH(first, ChildrenList, tChildrenList)
    if (node == *first)
      ChildrenList.erase(first);
  Database->noticePlaylistTreeChanged();
}




void tPlaylistNode::clear()
{
  FOREACH(first, ChildrenList, tChildrenList)
  delete * first;
  ChildrenList.clear();
}




QString tPlaylistNode::qualifiedName()
{
  if (Parent && Parent->Parent)
    return Parent->qualifiedName() + "/" + name();
  else
    return name();
}




tPlaylistNode *tPlaylistNode::resolve(const QString &path)
{
  if (path == "")
    return this;

  int slash_index = path.find('/');

  QString local_component,remainder;
  if (slash_index == -1)
    local_component = path;
  else
  {
    local_component = path.left(slash_index);
    remainder = path.mid(slash_index + 1);
  }

  FOREACH_CONST(first, ChildrenList, tChildrenList)
  {
    if (local_component == (*first)->name())
    {
      if (slash_index == -1)
	return *first;
      else
	return (*first)->resolve(remainder);
    }
  }
  return NULL;
}




bool tPlaylistNode::hasParent(tPlaylistNode *node)
{
  if (Parent == node)
    return true;

  if (Parent)
    return Parent->hasParent(node);
  else
    return false;
}




QString const &tPlaylistNode::name()
{
  return Name;
}




void tPlaylistNode::setName(QString const &name)
{
  if (Name == name)
    return;
  if (name.find('/') != -1)
    throw tRuntimeError(qApp->translate("ErrorMessages", "Slashes may not occur in playlist names."));
  if (name == "")
    throw tRuntimeError(qApp->translate("ErrorMessages", "Playlist names may not be empty."));
  if (Parent && Parent->resolve(name))
    throw tRuntimeError(
	qApp->translate("ErrorMessages", "Playlist name '%1' is not unique, not set.")
	.arg(name) );
    
  Name = name;
  Database->noticePlaylistTreeChanged();
}




tPlaylist *tPlaylistNode::data()
{
  return Data;
}




void tPlaylistNode::setData(tPlaylist *data)
{
  if (data)
    delete Data;
  Data = data;
  Database->noticePlaylistTreeChanged();
}




QDomNode tPlaylistNode::serialize(QDomDocument &doc)
{
  QDomElement result = doc.createElement("song_set_node");
  result.setAttribute("name", Name);

  if (Data)
    result.appendChild(Data->serialize(doc));

  FOREACH(first, ChildrenList, tChildrenList)
    result.appendChild((*first) ->serialize(doc));
  return result;
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

