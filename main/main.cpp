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




#include <exception>
#include <qapplication.h>
#include <qtextcodec.h>
#include "utility/player.h"
#include "database/database.h"
#include "ui/mainwin.h"




struct tNoGuiProgram : public tProgramBase
{
    void internal_setStatus(const QString &status)
    {
    }
    void internal_quitApplication()
    {
      qApp->quit();
    }
};




int main(int argc, char **argv)
{
  srand(time(NULL));

  try
  {
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ))
    {
      cout << "madman " << STRINGIFY(MADMAN_VERSION) << endl;
      cout << "usage: " << argv[0] << " [--nogui] [filename_to_open]" << endl << endl;
      cout << "options:" << endl;
      cout << "  --nogui: run without GUI" << endl;
      return 1;
    }
    if (argc >= 2 && strcmp(argv[1], "--nogui") == 0)
    {
      QApplication app(argc, argv, /* GUIEnabled */ false);

      QTranslator translator(0);
      translator.load(QString("madman_") + QTextCodec::locale(), ".");
      app.installTranslator(&translator);

      tNoGuiProgram prog;
      prog.preferences().load(prog.settings());

      tHttpDaemon httpd(prog.preferences().HttpDaemonPort, prog.preferences().HttpRestrictToLocalhost);
      addResponders(&httpd);

      bool filename_valid = false;
      QString filename;
      
      if (!filename_valid && argc >= 3)
      {
	filename = argv[ 2 ];
	filename_valid = true;
      }
      if (!filename_valid) 
	filename = prog.settings().readEntry("/madman/startup_file", QString::null, &filename_valid );
      if (!filename_valid)
	return 1;

      cout << "loading " << filename << "..." << flush;
      auto_ptr<tDatabase> db(new tDatabase);
      db->load(filename, 0);
      prog.setDatabase(db.release());
      cout << "done." <<endl;
      cout << "running..." <<endl;
      return app.exec();
    }
    else
    {
      QApplication app(argc, argv);

      QTranslator translator(0);
      translator.load(QString("madman_") + QTextCodec::locale(), ".");
      app.installTranslator(&translator);

      QString filename_to_open;
      if (argc >= 2)
	filename_to_open = argv[ 1 ];
      tMainWindow mwin;
      mwin.initialize(filename_to_open);
      mwin.show();
      app.setMainWidget(&mwin);
      return app.exec();
    }
  }
  catch (exception & ex)
  {
    cerr << "*** terminated by fatal exception:" << endl;
    cerr << "*** " << ex.what() << endl;
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
