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




#include "history.h"
#include "database/song.h"




void tHistory::played(tUniqueId id, time_t when, double duration)
{
  tHistoryEntry entry;
  entry.UniqueId = id;
  entry.Time = when;
  entry.PlayDuration = duration;
  HistoryList.push_back(entry);
}




void tHistory::clear()
{
  HistoryList.clear();
}




QDomNode tHistory::serialize(QDomDocument &doc)
{
  QDomElement history = doc.createElement("history");
  FOREACH_CONST(first, HistoryList, tHistoryList)
  {
    QDomElement entry = doc.createElement("playedsong");
    entry.setAttribute("uniqueid", first->UniqueId);
    entry.setAttribute("time", (int) first->Time);
    entry.setAttribute("playduration", first->PlayDuration);

    history.appendChild(entry);
  }
  return history;




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
}
