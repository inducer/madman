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




#include <qlistbox.h>
#include <qpushbutton.h>
#include <qfiledialog.h>

#include "dirlist.h"




// tDirectoryListManager ------------------------------------------------------
tDirectoryListManager::tDirectoryListManager(QWidget *parent, tDirectoryList &dir_list,
    QListBox *lstbox,
    QPushButton *add_button,
    QPushButton *remove_button)
: Parent(parent), DirectoryList(dir_list), ListBox(lstbox),
AddButton(add_button),RemoveButton(remove_button)
{
  update();
  connect(AddButton, SIGNAL(clicked()), this, SLOT(add()));
  connect(RemoveButton, SIGNAL(clicked()), this, SLOT(remove()));
}




tDirectoryList &tDirectoryListManager::directoryList()
{
  return DirectoryList;
}




void tDirectoryListManager::add()
{
  QString dir = QFileDialog::getExistingDirectory(QString::null, Parent, tr("Add Directory"));
  if (dir == QString::null)
    return;
  DirectoryList.push_back((const char *) dir.utf8());
  update();

  emit changed();
}




void tDirectoryListManager::remove()
{
  if (ListBox->currentItem() < 0 || 
      ListBox->currentItem() >= (int) DirectoryList.size())
    return;

  DirectoryList.erase(DirectoryList.begin() + ListBox->currentItem());
  update();

  emit changed();
}




void tDirectoryListManager::update()
{
  ListBox->clear();
  FOREACH_CONST(first, DirectoryList, tDirectoryList)
    ListBox->insertItem(QString::fromUtf8(first->c_str()));
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

