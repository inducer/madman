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




#ifndef HEADER_SEEN_PLUGIN_H
#define HEADER_SEEN_PLUGIN_H




#include "base.h"




class QSettings;

class tPlugin
{
  public:
    QString		Filename;
    QString		Name,Description;
    QString		Arguments;
    QString		MenuString;
    QString		KeyboardShortcut;
    bool		Confirm;
    bool		RescanAfter;

    tPlugin();
    bool gatherInfo();
    void configure (QSettings &settings);
    void run(QSettings &settings, const tSongList &songs);
};




typedef vector<tPlugin>		tPluginList;




void enumeratePlugins(const tDirectoryList &dir_list, tPluginList &plugin_list);




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
