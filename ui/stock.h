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





#ifndef HEADER_SEEN_STOCK_H
#define HEADER_SEEN_STOCK_H




#include <qaction.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include "designer/image_data.h"




inline QPixmap getStockPixmap(const char *name)
{
  if (!images::getFile(name))
    throw tRuntimeError(QString("stock icon not found: %1").arg(name));

  QPixmap pm;
  pm.loadFromData((const unsigned char *) images::getFile(name), images::getFileSize(name));
  return pm;
}




inline QIconSet getStockIconSet(const char *name)
{
  return QIconSet(getStockPixmap(name));
}




inline void setStockIcon(QPushButton *pb, const char *name)
{
  pb->setIconSet(getStockIconSet(name));
}





inline void setStockIcon(QToolButton *pb, const char *name)
{
  pb->setIconSet(getStockIconSet(name));
}




inline void setStockIcon(QAction *act, const char *name)
{
  act->setIconSet(getStockIconSet(name));
}




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
