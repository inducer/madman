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




#ifndef HEADER_SEEN_PROGRESS_IMPL
#define HEADER_SEEN_PROGRESS_IMPL




#include <qstatusbar.h>
#include <qprogressdialog.h>
#include "utility/progress.h"




class tCancellableSimpleProgress : public tProgress
{
  bool Cancelled;
  int Counter;

  public:
  tCancellableSimpleProgress()
    : Cancelled(false), Counter(0)
    {
    }
  void processEvents() 
  { 
    if (++Counter == 10)
    {
      Counter = 0;
      qApp->processEvents();
    }
  }
  bool wasCancelled()
  {
    return Cancelled;
  }
  void cancel()
  {
    Cancelled = true;
  }
};





class tCancellableStatusBarProgress : public tCancellableSimpleProgress
{
  QStatusBar *StatusBar;
  public:
  tCancellableStatusBarProgress(QStatusBar *bar)
    : StatusBar(bar)
    {
    }
  ~tCancellableStatusBarProgress()
  {
    StatusBar->clear();
  }
  void setWhat(const QString &what) 
  { 
    StatusBar->message(what);
  }
};




class tProgressDialog : public tProgress
{
    auto_ptr<QProgressDialog> Progress;
    QProgressBar *Bar;

  public:
    tProgressDialog(QWidget *widget, bool cancellable)
      : Progress(new QProgressDialog(widget, "progress", true))
      {
        if (!cancellable)
          Progress->setCancelButtonText(QString::null);
        Bar = new QProgressBar(Progress.get());
        Bar->show();
        Progress->setBar(Bar);
        
        Progress->setAutoReset(false);
        Progress->setAutoClose(false);
        Progress->setMinimumDuration(0);
        Progress->setProgress(0);
      }
    bool wasCancelled()
      { 
        return Progress->wasCancelled();
      }
    void processEvents() 
      { 
        qApp->processEvents(); 
      }
    int progress() 
      { 
        return Progress->progress();
      }
    void setProgress(int p) 
      { 
        Progress->setProgress(p);
      }
    void setTotalSteps(int p)
      { 
        Progress->setTotalSteps(p);
        if (p == 0)
          Bar->setPercentageVisible(false);
        else
          Bar->setPercentageVisible(true);
      }
    void setWhat(const QString &what) 
      { 
        Progress->setLabelText(what);
      }
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
