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



#ifndef HEADER_SEEN_CLICKABLE_LABEL
#define HEADER_SEEN_CLICKABLE_LABEL




#include <qframe.h>
#include <qsimplerichtext.h>
#include "utility/base.h"




class tClickableLabel : public QFrame
{
    Q_OBJECT;

    typedef QFrame super;
    auto_ptr<QSimpleRichText>        RichText;

    QString HeldAnchor;

  public:
    tClickableLabel(QWidget *parent, const char *name = 0, WFlags f = 0);
    void setText(const QString &text);
    
    QSize minimumSizeHint () const;

  protected:
    void drawContents(QPainter *p);
    void mouseEnterEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void leaveEvent(QEvent *e);

    void updateCursor(QMouseEvent *e);
    
  signals:
    void linkClicked(const QString &href);
    void contextMenuRequested(const QString &href, const QPoint &point);
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
