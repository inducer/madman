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




#include <qcursor.h>
#include <qevent.h>
#include "ui/clickable_label.h"




// tClickableLabel ------------------------------------------------------------
tClickableLabel::tClickableLabel(QWidget *parent, const char *name, WFlags f)
: super(parent, name, f)
{
  setMouseTracking(true);
}




void tClickableLabel::setText(const QString &text)
{
  auto_ptr<QSimpleRichText> rt(new QSimpleRichText(text, font()));
  rt->setWidth(100000);
  RichText = rt;
  updateGeometry();
  update();
}




QSize tClickableLabel::minimumSizeHint() const
{
  if (RichText.get())
    return QSize(RichText->widthUsed(), RichText->height());
  else
    return super::minimumSizeHint();
}




void tClickableLabel::drawContents(QPainter *p)
{
  super::drawContents(p);
  if (RichText.get())
  {
    QRect cr = contentsRect();
    int top_offset = (cr.bottom() - cr.top() - RichText->height())/2;
    RichText->draw(p, cr.left(), cr.top() + top_offset, cr, colorGroup());
  }
}




void tClickableLabel::mousePressEvent(QMouseEvent *e)
{
  super::mousePressEvent(e);
  updateCursor(e);

  if (RichText.get())
  {
    QString anchor = RichText->anchorAt(QPoint(e->x(), e->y()));
    if (anchor != "" && !anchor.isNull())
    {
      HeldAnchor = anchor;
      grabMouse();
    }
  }
}




void tClickableLabel::mouseReleaseEvent(QMouseEvent *e)
{
  super::mouseReleaseEvent(e);
  updateCursor(e);

  if (RichText.get())
  {
    if (!HeldAnchor.isNull())
    {
      releaseMouse();
      QString anchor = RichText->anchorAt(QPoint(e->x(), e->y()));
      if (anchor == HeldAnchor)
      {
	if (e->button() != RightButton)
	  emit linkClicked(anchor);
	else
	  emit contextMenuRequested(anchor, e->globalPos());
      }
      else if (e->button() == RightButton)
	emit contextMenuRequested(QString::null, e->globalPos());
    }
    else if (e->button() == RightButton)
      emit contextMenuRequested(QString::null, e->globalPos());

    HeldAnchor = QString::null;
  }
  else if (e->button() == RightButton)
    emit contextMenuRequested(QString::null, e->globalPos());
} 




void tClickableLabel::mouseMoveEvent(QMouseEvent *e)
{
  super::mouseMoveEvent(e);
  updateCursor(e);
}




void tClickableLabel::leaveEvent(QEvent *e)
{
  super::leaveEvent(e);
  unsetCursor();
}




void tClickableLabel::updateCursor(QMouseEvent *e)
{
  if (RichText.get())
  {
    QString anchor = RichText->anchorAt(QPoint(e->x(), e->y()));
    if (anchor == "" || anchor.isNull())
      unsetCursor();
    else
      setCursor(Qt::PointingHandCursor);
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
