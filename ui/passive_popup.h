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



#ifndef _HEADER_SEEN_PASSIVE_POPUP
#define _HEADER_SEEN_PASSIVE_POPUP




#include <qwidget.h>
#include <qtimer.h>
#include <qdatetime.h>
#include "utility/prefs.h"




class tMainWindow;
class QLabel;




class tPassivePopup : public QObject
{
    Q_OBJECT

  public:
  private:
    int				ShownMilliseconds;
    tMainWindow 		*MainWindow;
    
    enum tState
    {
      APPEARING,
      SHOWING,
      DISAPPEARING
    }				State;
    QTimer			MyTimer;
    QTime                       StateTime;
    int				TotalStateMilliseconds;
    tPassivePopupMode           Mode;
    int				XPos, YPos, YEnd;
    bool                        StartAnew;

    QWidget                     *Widget;
    QLabel                      *Label;
        
  public:
    tPassivePopup(const QString &text, unsigned msec, 
                  tPassivePopupMode mode, bool show_right_now = false);
    ~tPassivePopup();

  signals:
    void skipBack();
    void skipForward();

  private slots:
    void timer();
    void emitSkipBack() { emit skipBack(); }
    void emitSkipForward() { emit skipForward(); }
};




tPassivePopup *showPopup(tPassivePopupMode mode, 
                         const QString &text, unsigned msec);




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
