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




#ifndef BASE
#define BASE




#include <memory>
#include <stdexcept>
#include <iostream>
#include <string>
#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qdatetime.h>
#include <qxml.h>

#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>




using namespace std;




class tSong;




typedef int tUniqueId;
typedef unsigned tIndex;
typedef string tFilename;
typedef vector<tFilename> tDirectoryList;
typedef vector<tSong *> tSongList;
typedef vector<const tSong *> tConstSongList;
typedef vector<tUniqueId> tUniqueIdList;
typedef tr1::unordered_set<tUniqueId> tUniqueIdSet;








#define FOREACH(VAR,LIST,LISTTYPE) \
  for (LISTTYPE::iterator VAR = (LIST).begin(),last = (LIST).end();VAR != last;VAR++)
#define FOREACH_CONST(VAR,LIST,LISTTYPE) \
  for (LISTTYPE::const_iterator VAR = (LIST).begin(),last = (LIST).end();VAR != last;VAR++)
#define XSTRINGIFY(ARG) #ARG
#define STRINGIFY(ARG) XSTRINGIFY(ARG)




class tProgress;




class tRuntimeError : public runtime_error
{
  public:
    tRuntimeError(const QString &err)
      : runtime_error(string(err.utf8()))
    {
    }
};




class tFileLocked : public runtime_error 
{ 
  public:
    tFileLocked(const string &filename)
      : runtime_error(filename + " is locked")
      { }
};




class tFileLock
{
    string LockName;

  public:
    tFileLock(const string &name, bool break_lock = false);
    ~tFileLock();
};




bool hasAttribute(const char *name, const QXmlAttributes &attributes);
//const char *lookupAttributeUtf8(const char *name, QXmlAttributes &attributes);
QString lookupAttribute(const char *name, const QXmlAttributes &attributes);

string QString2string(QString const &str);
QString string2QString(string const &str);
QString wstring2QString(const wchar_t *str);
vector<QString> QStringList2QStringvector(const QStringList &strlist);
QStringList QStringvector2QStringList(const vector<QString> &strlist);
vector<string> QStringList2stringvector(const QStringList &strlist);
QStringList stringvector2QStringList(const vector<string> &strlist);
ostream &operator <<(ostream &ostr, QString const &str);
string replace(const string &haystack, 
               const string &needle,
               const string &new_needle);
QString quoteString(const QString &victim);
string quoteString(const string &victim);
void split(const QString &sep, const QString &str, vector<QString> &result);
QString join(const vector<QString> &strlist, const QString &sep = QString(";"));
void enumerateFiles(const string &directory, vector<string> &result, tProgress *prog = NULL);
size_t getMaxBase64DecodedSize(size_t encoded);
size_t decodeBase64(unsigned char *data, string const &base64);
void encodeBase64(string &base64, unsigned char const *data, size_t size);
string encodeBase64(const string &src);
string decodeBase64(const string &src);
string sanitizeUtf8(string const &victim);
bool isValidUtf8(string const &victim);
tFilename stripPath(const tFilename &fn);
tFilename stripExtension(const tFilename &fn);



class hash_QString
{
  public:
    unsigned long operator() (QString const &str) const;
};

class hash_string
{
  public:
    unsigned long operator() (string const &str) const;
};

class tProfiler
{
    QTime		Time;
    QString	Name;

  public:
    tProfiler(const QString &name = "");
    ~tProfiler();
};

class tObject
{
  public:
    virtual ~tObject() { }
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
