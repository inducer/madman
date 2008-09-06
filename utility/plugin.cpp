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




#include "utility/plugin.h"
#include "database/song.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qapplication.h>
#include <qregexp.h>
#include <qsettings.h>
#include <qinputdialog.h>
#include <qfiledialog.h>

#include <cstdlib>
#include <stdexcept>
#include <csignal>
#include <sys/wait.h>




class tPluginExecutionCancelled : public exception { };




// tPlugin --------------------------------------------------------------------
tPlugin::tPlugin()
  : Confirm(false), RescanAfter(false)
{
}




bool tPlugin::gatherInfo()
{
  Confirm = false;
  RescanAfter = false;
  
  QRegExp this_is_not_re("MADMAN-THIS-IS-NOT-A-PLUGIN");
  QRegExp version_re("MADMAN-PLUGIN-VERSION:(.*)$");
  QRegExp name_re("MADMAN-NAME:(.*)$");
  QRegExp description_re("MADMAN-DESCRIPTION:(.*)$");
  QRegExp arguments_re("MADMAN-ARGUMENTS:(.*)$");
  QRegExp menustring_re("MADMAN-MENUSTRING:(.*)$");
  QRegExp shortcut_re("MADMAN-KEYBOARDSHORTCUT:(.*)$");
  QRegExp confirm_re("MADMAN-CONFIRM:(.*)$");
  QRegExp rescanafter_re("MADMAN-RESCANAFTER:(.*)$");
  QFile pluginfile(Filename);
  if (!pluginfile.open(IO_ReadOnly))
    return false;

  bool isplugin = false;

  QTextStream stream(&pluginfile);
  while (!stream.eof())
  {
    QString line = stream.readLine();

    if (this_is_not_re.search(line) >= 0)
      return false;

    if (version_re.search(line) >= 0)
    { 
      isplugin = version_re.cap(1).stripWhiteSpace() == "2";
      if (!isplugin)
	return false;
    }
    if (name_re.search(line) >= 0)
    { 
      Name = name_re.cap(1).stripWhiteSpace();
    }
    else if (description_re.search(line) >= 0)
    { 
      Description = description_re.cap(1).stripWhiteSpace();
    }
    else if (arguments_re.search(line) >= 0)
    { 
      Arguments = arguments_re.cap(1).stripWhiteSpace();
    }
    else if (menustring_re.search(line) >= 0)
    { 
      MenuString = menustring_re.cap(1).stripWhiteSpace();
    }
    else if (shortcut_re.search(line) >= 0)
    { 
      KeyboardShortcut = shortcut_re.cap(1).stripWhiteSpace();
    }
    else if (confirm_re.search(line) >= 0)
    { 
      bool ok;
      Confirm = confirm_re.cap(1).stripWhiteSpace().toInt(&ok) != 0;
      if (!ok)
	throw runtime_error("found invalid numeral in confirm line of "+QString2string(Filename));
    }
    else if (rescanafter_re.search(line) >= 0)
    { 
      bool ok;
      RescanAfter = rescanafter_re.cap(1).stripWhiteSpace().toInt(&ok) != 0;
      if (!ok)
	throw runtime_error("found invalid numeral in rescanafter line of "+QString2string(Filename));
    }
  }

  if (MenuString.isNull())
    MenuString = Name;

  return isplugin;
}




void tPlugin::configure (QSettings &settings)
{
  QFileInfo info(Filename);

  QRegExp config_re("%config:(.*)%");
  config_re.setMinimal(true);
  int where = 0;
  while ((where = config_re.search(Arguments, where + 1)) >= 0)
  {
    QString prompt = config_re.cap(1);
    QString key = QString("/madman/plugins/%1/%2").arg(info.fileName()).arg(prompt);
    QString value = settings.readEntry(key);

    bool ok;
    QString input = QInputDialog::getText(qApp->translate("tPlugin", "madman Plugin"),
	prompt, QLineEdit::Normal, value, &ok );
    if (!ok)
      return;
    settings.writeEntry(key, input);
  }
}




namespace
{
  QString expand(const QString &pluginname, QSettings &settings, const QString &victim, tSong *context)
  {
    QString result = victim;
    if (context)
      result = substituteSongFields(result, context, false, true);

    // %ask
    QRegExp ask_re("%ask:(.*)%");
    ask_re.setMinimal(true);
    int where = 0;
    while ((where = ask_re.search(result)) >= 0)
    {
      QString prompt = ask_re.cap(1);
      bool ok;
      QString input = QInputDialog::getText(qApp->translate("tPlugin", "madman Plugin"),
	  prompt, QLineEdit::Normal, QString::null, &ok, qApp->mainWidget());
      if (!ok)
	throw tPluginExecutionCancelled();

      result.replace(where, ask_re.matchedLength(), QString("\"%1\"").arg(input));
    }

    QFileInfo info(pluginname);

    // %config
    QRegExp config_re("%config:(.*)%");
    config_re.setMinimal(true);
    while ((where = config_re.search(result)) >= 0)
    {
      bool ok;
      QString key = QString("/madman/plugins/%1/%2").arg(info.fileName()).arg(config_re.cap(1));
      QString value = settings.readEntry(key, QString::null, &ok);
      if (!ok)
	throw tRuntimeError(qApp->translate("tPlugin", "Plugin not yet configured"));

      result.replace(where, config_re.matchedLength(), QString("\"%1\"").arg(value));
    }

    // %askfile_open
    QRegExp askfile_open_re("%askfile_open:(.*)%");
    askfile_open_re.setMinimal(true);
    while ((where = askfile_open_re.search(result)) >= 0)
    {
      QString filename = QFileDialog::getOpenFileName(QString::null, "Any file (*.*)", 
						      qApp->mainWidget(), "plugin_filedlg",
						      askfile_open_re.cap(1));
      if (filename.isNull())
	throw tPluginExecutionCancelled();

      result.replace(where, askfile_open_re.matchedLength(), QString("\"%1\"").arg(filename));
    }

    // %askfile_save
    QRegExp askfile_save_re("%askfile_save:(.*)%");
    askfile_save_re.setMinimal(true);
    while ((where = askfile_save_re.search(result)) >= 0)
    {
      QString filename = QFileDialog::getSaveFileName(QString::null, "Any file (*.*)", 
						      qApp->mainWidget(), "plugin_filedlg",
						      askfile_save_re.cap(1));
      if (filename.isNull())
	throw tPluginExecutionCancelled();

      result.replace(where, askfile_save_re.matchedLength(), QString("\"%1\"").arg(filename));
    }

    return result;
  }




  void mySystem(const QString &command)
  {
    int ret = system(command.latin1());
    /*
     * XXX FIXME portability problems to FreeBSD
    if (WIFSIGNALED(ret) &&
	(WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
      throw tRuntimeError(qApp->translate("tPlugin", "Plugin died because it caught SIGINT or SIGQUIT "));
      */
    if (ret)
      throw tRuntimeError(qApp->translate("tPlugin", "Plugin execution resulted in non-zero return value"));
  }
}




void tPlugin::run(QSettings &settings, const tSongList &songs)
{
  try
  {
    QString myargs = Arguments;
    QRegExp repeat_re("%repeat%(.*)%endrepeat%");
    bool hasrepeat = repeat_re.search(myargs) >= 0;
    if (hasrepeat)
    {
      QString repeat_section = repeat_re.cap(1);
      myargs = myargs.replace(repeat_re, "%REPEAT%");

      myargs = expand(Filename, settings, myargs, NULL);
      if (myargs.isNull())
        return;

      QString repeatargs;
      FOREACH_CONST(first, songs, tSongList)
      {
        QString expansion = expand(Filename, settings, repeat_section, *first);
        if (expansion.isNull())
          return;
        repeatargs += " " + expansion;
      }

      myargs.replace("%REPEAT%", repeatargs);

      QString command = Filename + " " + myargs;
      mySystem(command);
    }
    else
    {
      FOREACH_CONST(first, songs, tSongList)
      {
        QString expansion = expand(Filename, settings, myargs, *first);
        if (expansion.isNull())
          return;
        QString command = Filename + " " + expansion;
        mySystem(command);
      }
    }
  }
  catch (tPluginExecutionCancelled) { }
}




// functions ------------------------------------------------------------------
void enumeratePlugins(const tDirectoryList &dir_list, tPluginList &plugin_list)
{
  FOREACH_CONST(first, dir_list, tDirectoryList)
  {
    QDir directory(QString::fromUtf8(first->c_str()));
    if (!directory.exists())
    {
      cerr << "[ERROR] nonexistent: " << *first << endl;
      continue;
    }

    QStringList strlist = directory.entryList();
    for (unsigned i = 0; i < strlist.size(); i++)
    {
      QString name = strlist[ i ];
      QString fullname = QString::fromUtf8(first->c_str()) + "/" + name;

      QFileInfo file(fullname);
      if (!file.isExecutable())
	continue;

      tPlugin plugin;
      plugin.Filename = fullname;
      if (plugin.gatherInfo())
	plugin_list.push_back(plugin);
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
}
