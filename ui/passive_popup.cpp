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




#include <qsize.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qfont.h>
#include <qapplication.h>
#include "ui/passive_popup.h"

#include "ui/mainwin.h"
#include "ui/stock.h"




static tPassivePopup *CurrentPopup = NULL;




tPassivePopup::tPassivePopup(const QString &text, unsigned msec, 
                             tPassivePopupMode mode, 
                             bool show_instantaneously)
  : ShownMilliseconds(msec), Mode(mode), XPos(0), YRetracted(0), YExtended(0)
{
  QVBox *vbox = new QVBox(NULL, "passive_popup", Qt::WStyle_NoBorder | Qt::WX11BypassWM | Qt::WStyle_StaysOnTop);
  vbox->setSpacing(5);

  Widget = vbox;
  vbox->setFrameShape(QFrame::Panel);
  vbox->setFrameShadow(QFrame::Raised);
  vbox->setMargin(5);
  vbox->setSpacing(10);
    
  QHBox *hbox = new QHBox(vbox);
  hbox->setSpacing(10);
  QPushButton *buttonBack = new QPushButton(
    getStockIconSet("back.png"), "", hbox);  
  connect(buttonBack, SIGNAL(clicked()), this, SLOT(emitSkipBack()));
  
  Label = new QLabel(text, hbox);
  QFont my_font = Label->font();
  if (my_font.pixelSize() < 0)
    my_font.setPointSizeFloat(my_font.pointSizeFloat() * 1.5);
  else
    my_font.setPixelSize(int(my_font.pixelSize() * 1.5));
  my_font.setBold(true);
  Label->setFont(my_font);
  Label->setAlignment(Qt::AlignHCenter);

  QPushButton *buttonNext = new QPushButton(
    getStockIconSet("forward.png"), "", hbox);  
  connect(buttonNext, SIGNAL(clicked()), this, SLOT(emitSkipForward()));
  
  QPushButton *buttonHide = new QPushButton(tr("Hide"), vbox);
  connect(buttonHide, SIGNAL(clicked()), this, SLOT(deleteLater()));

  // normalize mode setting
  if (Mode < POPUP_FIRST || Mode >= POPUP_COUNT)
    Mode = POPUP_TOP_CENTER;

  StartAnew = true;
  if (show_instantaneously)
  {
    State = SHOWING;
    TotalStateMilliseconds = ShownMilliseconds;
  }
  else
  {
    State = APPEARING;
    TotalStateMilliseconds = 500;
  }

  // calculate position
  QDesktopWidget *d = QApplication::desktop();
  QSize sizehint = Widget->minimumSizeHint();

  // calculate start and end positions for y
  if (Mode < POPUP_FIRST_BOTTOM)
  {      
    YRetracted = 1-sizehint.height();
    YExtended = 0;
  }
  else
  {
    YRetracted = d->height() + sizehint.height()-1;
    YExtended = d->height() - sizehint.height();
  }
  
  // calculate x position
  if (Mode == POPUP_TOP_LEFT || Mode == POPUP_BOTTOM_LEFT)
    XPos = 0;
  else if (Mode == POPUP_TOP_RIGHT || Mode == POPUP_BOTTOM_RIGHT)
    XPos = (d->width() - sizehint.width());
  else // center      
    XPos = (d->width() - sizehint.width()) / 2;
      
  if (State == SHOWING)
    Widget->move(XPos, YExtended);
  else
    Widget->move(XPos, YRetracted);
    
  // show
  Widget->show();
      
  connect(&MyTimer, SIGNAL(timeout()), this, SLOT(timer()));
  MyTimer.start(20, false);
}




tPassivePopup::~tPassivePopup()
{
  delete Widget;
  if (CurrentPopup == this)
    CurrentPopup = NULL;
}




static int bound_by(int value, int bound1, int bound2)
{
  int low = std::min(bound1, bound2);
  int high = std::max(bound1, bound2);
  return std::max(low, std::min(high, value));
}




void tPassivePopup::timer()
{
  if (StartAnew)
  {
    StateTime.start();
    StartAnew = false;
  }

  int elapsed = StateTime.elapsed();
  if (elapsed > TotalStateMilliseconds)
  {
    StartAnew = true;
    switch (State)
    {
      case APPEARING:
	State = SHOWING;	
	Widget->move(XPos, YExtended);
	TotalStateMilliseconds = ShownMilliseconds;
        elapsed = 0;
	break;
      case SHOWING:
	State = DISAPPEARING;
	TotalStateMilliseconds = 500;
        elapsed = 0;
	break;
      case DISAPPEARING:
	//Widget->hide();
	deleteLater();
    }
  }

  float time_frac = float(elapsed)/TotalStateMilliseconds;
  bool do_set = false;
  float extend_frac;

  switch (State)
  {       
    case APPEARING:
      do_set = true;
      extend_frac = time_frac;
      break;
    case DISAPPEARING:
      do_set = true;
      extend_frac = 1-time_frac;
      break;
    default:
      break;
  }

  if (do_set)
  {
    int ypos = bound_by(int(YRetracted + extend_frac*(YExtended-YRetracted)),
          YExtended, YRetracted);
    Widget->move(XPos, ypos);
  }

}




tPassivePopup *showPopup(tPassivePopupMode mode, const QString &text, unsigned msec)
{
  if (mode == POPUP_DISABLED)
    return NULL;

  if (CurrentPopup)
  {
    CurrentPopup->deleteLater();
    CurrentPopup = new tPassivePopup(text, msec, mode, true);
  }
  else
  {
    CurrentPopup = new tPassivePopup(text, msec, mode);
  }
  return CurrentPopup;
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

