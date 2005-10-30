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




#ifndef _HEADER_SEEN_PROGRAM_BASE_H
#define _HEADER_SEEN_PROGRAM_BASE_H




#include <qsettings.h>

#include "utility/base.h"
#include "utility/singleton.h"
#include "utility/prefs.h"
#include "database/database.h"
#include "database/auto_dj.h"




class tProgramBase : public QObject, public tSingleton<tProgramBase>
{
    Q_OBJECT;

    QSettings Settings;
    tPreferences Preferences;
    auto_ptr<tDatabase> Database;

    tFilename CurrentSongFilename;

    tAutoDJ AutoDJ;

  public:
    tProgramBase();
    virtual ~tProgramBase();
    virtual void setDatabase(tDatabase *db);

    static tSong *currentSong();

  protected slots:
    void noticeStateChanged();
    void noticeSongChanged(tFilename last_song, float play_time);

  signals:
    void songChanged();

  public:
    static void setStatus(const QString &status)
      {
	if (instance())
	  instance()->internal_setStatus(status);
	else
	  throw runtime_error("instance not available in setStatus()");
      }

    static QSettings &settings()
      {
	if (instance())
	  return instance()->Settings;
	else
	  throw runtime_error("instance not available in settings()");
      }

    static tDatabase &database()
      {
	if (instance())
	  return *instance()->Database;
	else
	  throw runtime_error("instance not available in database()");
      }

    static tAutoDJ &autoDJ()
      {
	if (instance())
	  return instance()->AutoDJ;
	else
	  throw runtime_error("instance not available in autoDJ()");
      }

    static tPreferences &preferences()
      {
	if (instance())
	  return instance()->Preferences;
	else
	  throw runtime_error("instance not available in preferences()");
      }

    static void quitApplication()
      {
	if (instance())
	  instance()->internal_quitApplication();
	else
	  throw runtime_error("instance not available in quitApplication()");
      }

  protected:
    virtual void internal_setStatus(const QString &status) = 0;
    virtual void internal_quitApplication() = 0;
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
