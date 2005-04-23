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




#include "base.h"
#include "database/song.h"
#include <qregexp.h>
#include <stdexcept>
#include <cstdio>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "utility/progress.h"




// tFileLock ----------------------------------------------------
tFileLock::tFileLock(const string &name, bool break_lock)
{
  LockName = name + "-lock";

  if (access(LockName.c_str(), F_OK) == 0)
  {
    // the file exists
    if (break_lock)
    {
      if (unlink(LockName.c_str()))
        throw tRuntimeError("failed to delete lock file");
    }
    else
      throw tFileLocked(name);
  }

  FILE *lock = fopen(LockName.c_str(), "w");
  if (lock == NULL)
    throw tRuntimeError("failed to create lock file");
  fclose(lock);
}





tFileLock::~tFileLock()
{
  if (unlink(LockName.c_str()))
    throw tRuntimeError("failed to delete lock file");
}





// utility routines ---------------------------------------------
bool hasAttribute(const char *name, const char **attributes)
{
  while (*attributes && strcmp(name, *attributes) != 0)
    attributes += 2;

  if (*attributes == 0)
    return false;
  return true;
}




const char *lookupAttributeUtf8(const char *name, const char **attributes)
{
  while (*attributes && strcmp(name, *attributes) != 0)
    attributes += 2;

  if (*attributes == 0)
    throw runtime_error(QString2string("Attribute not found: " + QString::fromUtf8(name, strlen(name))));
  return attributes[1];
}




QString lookupAttribute(const char *name, const char **attributes)
{
  return QString::fromUtf8(lookupAttributeUtf8(name, attributes));
}




string QString2string(QString const &str)
{
  if (str == QString::null)
    return "<NULL>";
  else
    return (const char *) str.utf8();
}




QString string2QString(string const &str)
{
  return QString::fromUtf8(str.c_str());
}




// tIConv ----------------------------------------------------------------------
/*
class tIConv {
    iconv_t ConversionHandle;

  public:
    tIConv(const string &to, const string &from);
    ~tIConv();
    string convert(const char *data, size_t size);
    string convert(const string &str);
    void reset();
};




tIConv::tIConv(const string &to, const string &from)
{
  ConversionHandle = iconv_open (to.c_str(), from.c_str());
  if (ConversionHandle == (iconv_t) -1)
  {
    if (errno == EINVAL)
      throw runtime_error( "could not instantiate this specific iconv table" );
    else
      throw runtime_error( "iconv_open error" );
  }
}




tIConv::~tIConv()
{
  if (iconv_close(ConversionHandle) != 0)
    perror ("iconv_close");
}




string tIConv::convert(const char *data, size_t size)
{
  char *inptr = data;
  size_t inlength = size;

  string result;

  do
  {
    char outbuffer[ 2048 ];
    size_t outlength = sizeof(outbuffer);
    char *outptr = outbuffer;

    if (iconv(ConversionHandle, &inptr, &inlength, &outptr, &outlength) == (size_t) -1)
      throw runtime_error("Error during iconv conversion");

    result += string(outbuffer, sizeof(outbuffer) - outlength);
  }
  while (inlength);
  return result;
}




string tIConv::convert(const string &str)
{
  return convert(const_cast< char * >(str.c_str()), str.size());
}




void tIConv::reset()
{
  if (iconv(ConversionHandle, NULL, NULL, NULL, NULL) == (size_t) -1)
    throw runtime_error("Error during iconv reset");
}




QString wstring2QString(const wchar_t *str)
{
  char *inptr = const_cast< char * >(reinterpret_cast<const char *>(str));
  size_t inlength = wcslen(str);
  size_t inlength_bytes = inlength * sizeof(wchar_t);

  size_t destsize = inlength_bytes;
  char destbuffer[ destsize ];
  char *destptr = destbuffer;

  if (iconv (WCharT2UCS2Iconv.cd, &inptr, &inlength_bytes, &destptr, &destsize) == (size_t) -1)
  {
    throw runtime_error("Error during wchar_t -> UCS2 conversion.");
  }

  unsigned short *ucs2 = (unsigned short *) destbuffer;
  ucs2[ inlength ] = 0;

  return QString::fromUcs2(ucs2);
}
*/




// string conversion ----------------------------------------------------------
vector<QString> QStringList2QStringvector(const QStringList &strlist)
{
  vector<QString> result;

  for (unsigned i = 0; i < strlist.size(); i++)
    result.push_back(strlist[ i ] );

  return result;
}




QStringList QStringvector2QStringList(const vector<QString> &strlist)
{
  QStringList result;

  for (unsigned i = 0; i < strlist.size(); i++)
    result.push_back(strlist[ i ] );

  return result;
}




vector<string> QStringList2stringvector(const QStringList &strlist)
{
  vector<string> result;

  for (unsigned i = 0; i < strlist.size(); i++)
    result.push_back((const char *) strlist[ i ].utf8());

  return result;
}




QStringList stringvector2QStringList(const vector<string> &strlist)
{
  QStringList result;

  for (unsigned i = 0; i < strlist.size(); i++)
    result.push_back(QString::fromUtf8(strlist[ i ].c_str()));

  return result;
}




ostream &operator <<(ostream &ostr, QString const &str)
{
  return (ostr << QString2string(str));
}




string replace(const string &haystack, 
  const string &needle,
  const string &new_needle)
{
  string result = haystack;
  string::size_type pos = 0;
  while ((pos = result.find(needle, pos)) != string::npos)
  {
    result.replace(pos, needle.size(), new_needle);
    pos += new_needle.size();
  }
  return result;
}




QString quoteString(const QString &victim)
{
  QString my_victim = victim;
  return "\"" + my_victim.replace(QRegExp("\\\""), "\\\"") + "\"";
}




string quoteString(const string &victim)
{
  return "\"" + replace(victim, "\"", "\\\"") + "\"";
}




void split(const QString &sep, const QString &str, vector<QString> &result)
{
  QStringList temp = QStringList::split(sep, str);
  FOREACH_CONST(first, temp, QStringList)
    result.push_back(*first);
}




QString join(const vector<QString> &strlist, const QString &sep)
{
  bool is_first = true;
  QString result;
  FOREACH_CONST(first, strlist, vector<QString>)
  {
    if (is_first)
    {
      result = *first;
      is_first = false;
    }
    else
      result += sep + *first;
  }
  return result;
}




void enumerateFiles(const string &directory, vector<string> &result, tProgress *prog)
{
  DIR *dir;
  dirent *entry;

  if (directory.size() == 0)
    throw runtime_error("passed zero-length directory to enumerateFiles");

  string realdir;
  if (directory[directory.size()-1] == '/')
    realdir = directory.substr(0, directory.size()-1);
  else
    realdir = directory;

  dir = opendir(realdir.c_str());
  if (dir == NULL)
  {
    // maybe add something so that the error is displayed in the GUI??
    cerr
      << "*** Failed to open directory: " << realdir.c_str() << endl;
	 return;
  }

  try
  {
    while ((entry = readdir(dir)))
    {
      if (prog)
        prog->setProgress(prog->progress() + 1);
      if (strcmp(entry->d_name, ".") == 0)
        continue;
      if (strcmp(entry->d_name, "..") == 0)
        continue;

      string full_name = realdir + "/" + entry->d_name;

      struct stat my_statbuf;
      if (stat(full_name.c_str(), &my_statbuf) != 0)
      {
        cerr 
          << "*** Failed to stat:" << endl
          << "*** " << full_name << endl;
        continue;
      }

      if (S_ISDIR(my_statbuf.st_mode))
        enumerateFiles(full_name, result, prog);
      else if (S_ISREG(my_statbuf.st_mode))
        result.push_back(full_name);
    }

    closedir(dir);
  }
  catch (...)
  {
    closedir(dir);
    throw;
  }
}




namespace 
{
  unsigned char const B64_INVALID = 0xff;
  unsigned char const B64_PAD = 0xfe;
  char const B64_PAD_CHAR = '=';
  char Base64EncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char Base64DecodeTable[] = { // based at 0
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,B64_INVALID,B64_INVALID,62,B64_INVALID,B64_INVALID,B64_INVALID,63,52,53,54,
    55,56,57,58,59,60,61,B64_INVALID,B64_INVALID,B64_INVALID,B64_PAD,B64_INVALID,
    B64_INVALID,B64_INVALID,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
    19,20,21,22,23,24,25,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
    B64_INVALID,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,
    44,45,46,47,48,49,50,51,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,B64_INVALID,
  };

}




size_t getMaxBase64DecodedSize(size_t encoded) 
{
  return ((encoded+3)/4)*3;
}




size_t decodeBase64(unsigned char *data, unsigned dest_size, string const &base64) 
{
  string::const_iterator first = base64.begin(),last = base64.end();

  unsigned char *data_start = data;

  unsigned long block;
  unsigned char a,b,c,d;

  while (first != last && dest_size > 0) {
    a = Base64DecodeTable[(int) *(first++)];
    b = Base64DecodeTable[(int) *(first++)];
    c = Base64DecodeTable[(int) *(first++)];
    d = Base64DecodeTable[(int) *(first++)];
    if (c == B64_PAD) {
      block = a << 3*6 | b << 2*6;
      *data++ = (block >> 16) & 0xff;
      --dest_size;
    }
    else if (d == B64_PAD) {
      if (dest_size < 2)
        throw runtime_error("base64 buffer overflow");
      block = a << 3*6 | b << 2*6 | c << 1*6;
      *data++ = (block >> 16) & 0xff;
      *data++ = (block >> 8) & 0xff;
      dest_size -= 2;
    }
    else {
      if (dest_size < 3)
        throw runtime_error("base64 buffer overflow");
      block = a << 3*6 | b << 2*6 | c << 1*6 | d << 0*6;
      *data++ = (block >> 16) & 0xff;
      *data++ = (block >> 8) & 0xff;
      *data++ = (block >> 0) & 0xff;
      dest_size -= 3;
    }
  }
  return data-data_start;
}




void encodeBase64(string &base64, unsigned char const *data, size_t size) 
{
  base64.resize((size+2)/3*4);

  unsigned long block;
  unsigned char a,b,c,d;

  unsigned char const *end = data+size;
  string::iterator first = base64.begin();
  while (data < end)
    if (data+1 == end) {
      block = data[0] << 16;
      a = (block >> 3*6) & 0x3f;
      b = (block >> 2*6) & 0x3f;
      *first++ = Base64EncodeTable[a];
      *first++ = Base64EncodeTable[b];
      *first++ = B64_PAD_CHAR;
      *first++ = B64_PAD_CHAR;
      data++;
    }
    else if (data+2 == end) {
      block = data[0] << 16 | data[1] << 8;
      a = (block >> 3*6) & 0x3f;
      b = (block >> 2*6) & 0x3f;
      c = (block >> 1*6) & 0x3f;
      *first++ = Base64EncodeTable[a];
      *first++ = Base64EncodeTable[b];
      *first++ = Base64EncodeTable[c];
      *first++ = B64_PAD_CHAR;
      data += 2;
    }
    else {
      block = data[0] << 16 | data[1] << 8 | data[2];
      a = (block >> 3*6) & 0x3f;
      b = (block >> 2*6) & 0x3f;
      c = (block >> 1*6) & 0x3f;
      d = (block >> 0*6) & 0x3f;
      *first++ = Base64EncodeTable[a];
      *first++ = Base64EncodeTable[b];
      *first++ = Base64EncodeTable[c];
      *first++ = Base64EncodeTable[d];
      data += 3;
    }
}




string encodeBase64(const string &src)
{
  string base64;
  encodeBase64(base64, reinterpret_cast<const unsigned char *>(src.data()), src.size());
  return base64;
}




string decodeBase64(const string &src)
{
  unsigned decoded_size = getMaxBase64DecodedSize(src.length());
  char buffer[decoded_size];
  unsigned size = decodeBase64(reinterpret_cast<unsigned char *>(buffer), decoded_size, src);
  return string(buffer, size);
}




unsigned long hash_QString::operator() (QString const &str) const
{
  unsigned long h = 0;
   for (unsigned i = 0; i < str.length(); i++)
    h = 5 * h + str[ i ].unicode();
  return h;
}





unsigned long hash_string::operator() (string const &str) const
{
  unsigned long h = 0;
  for (unsigned i = 0; i < str.size(); i++)
    h = 5 * h + str[i];
  return h;
}





string sanitizeUtf8(string const &victim)
{
  string result;
  string::const_iterator first = victim.begin(), last = victim.end();
  while (first != last)
  {
    unsigned char ucf = (unsigned char) *first;
    if (ucf < 0x20)
    {
      result += "?";
      first++;
    }
    else if (0x20 <= ucf && ucf <= 0x7f)
    {
      result += *first++;
    }
    else if (0x80 <= ucf && ucf <= 0xc1)
    {
      result += "?";
      first++;
    }
    else if (0xc2 <= ucf && ucf <= 0xdf)
    {
      // two byte sequences
      if (last - first >= 2)
      {
        unsigned char ucf1 = (unsigned char) first[1];
        if (0x80 <= ucf1 && ucf1 <= 0xbf)
        {
          result += *first++;
          result += *first++;
        }
        else
        {
          first += 2;
          result += "?";
        }
      }
      else
      {
        first++;
        result += "?";
      }
    }
    else if (0xe0 <= ucf  && ucf <= 0xef)
    {
      // three byte sequences
      if (last - first >= 3)
      {
        unsigned char ucf1 = (unsigned char) first[1];
        unsigned char ucf2 = (unsigned char) first[2];

        bool valid = 
          (0x80 <= ucf1 && ucf1 <= 0xbf) &&
          (0x80 <= ucf2 && ucf2 <= 0xbf);

        valid = valid && !(ucf == 0xe0 && ucf1 < 0xa0);
        valid = valid && !(ucf == 0xed && ucf1 > 0x9f);
        if (valid)
        {
          result += *first++;
          result += *first++;
          result += *first++;
        }
        else
        {
          first += 3;
          result += "?";
        }
      }
      else
      {
        first++;
        result += "?";
      }
    }
    else if (0xf0 <= ucf && ucf <= 0xf4)
    {
      // four byte sequences
      if (last - first >= 4)
      {
        unsigned char ucf1 = (unsigned char) first[1];
        unsigned char ucf2 = (unsigned char) first[2];
        unsigned char ucf3 = (unsigned char) first[3];

        bool valid = 
          (0x80 <= ucf1 && ucf1 <= 0xbf) &&
          (0x80 <= ucf2 && ucf2 <= 0xbf) &&
          (0x80 <= ucf3 && ucf3 <= 0xbf);

        valid = valid && !(ucf == 0xf0 && ucf1 < 0x90);
        valid = valid && !(ucf == 0xf4 && ucf1 > 0x8f);
        if (valid)
        {
          result += *first++;
          result += *first++;
          result += *first++;
          result += *first++;
        }
        else
        {
          first += 4;
          result += "?";
        }
      }
      else
      {
        first++;
        result += "?";
      }
    }
    else
    {
      first += 1;
      result += "?";
    }
  }
  return result;
}





bool isValidUtf8(string const &victim)
{
  string::const_iterator first = victim.begin(), last = victim.end();
  while (first != last)
  {
    unsigned char ucf = (unsigned char) *first;
    if (ucf < 0x20)
      return false;
    else if (0x20 <= ucf && ucf <= 0x7f)
      first++;
    else if (0x80 <= ucf && ucf <= 0xc1)
      return false;
    else if (0xc2 <= ucf && ucf <= 0xdf)
    {
      // two byte sequences
      if (last - first >= 2)
      {
        unsigned char ucf1 = (unsigned char) first[1];
        if (0x80 <= ucf1 && ucf1 <= 0xbf)
          first += 2;
        else
          return false;
      }
      else
        return false;
    }
    else if (0xe0 <= ucf && ucf <= 0xef)
    {
      // three byte sequences
      if (last - first >= 3)
      {
        unsigned char ucf1 = (unsigned char) first[1];
        unsigned char ucf2 = (unsigned char) first[2];

        bool valid = 
          (0x80 <= ucf1 && ucf1 <= 0xbf) &&
          (0x80 <= ucf2 && ucf2 <= 0xbf);

        valid = valid && !(ucf == 0xe0 && ucf1 < 0xa0);
        valid = valid && !(ucf == 0xed && ucf1 > 0x9f);
        if (valid)
          first += 3;
        else
          return false;
      }
      else
        return false;
    }
    else if (0xf0 <= ucf && ucf <= 0xf4)
    {
      // four byte sequences
      if (last - first >= 4)
      {
        unsigned char ucf1 = (unsigned char) first[1];
        unsigned char ucf2 = (unsigned char) first[2];
        unsigned char ucf3 = (unsigned char) first[3];

        bool valid = 
          (0x80 <= ucf1 && ucf1 <= 0xbf) &&
          (0x80 <= ucf2 && ucf2 <= 0xbf) &&
          (0x80 <= ucf3 && ucf3 <= 0xbf);

        valid = valid && !(ucf == 0xf0 && ucf1 < 0x90);
        valid = valid && !(ucf == 0xf4 && ucf1 > 0x8f);
        if (valid)
          first += 4;
        else
          return false;
      }
      else
        return false;
    }
    else
      return false;
  }
  return true;
}





// tProfiler ------------------------------------------------------------------
tProfiler::tProfiler(const QString &name)
: Name(name)
{
  Time.start();
}




tProfiler::~tProfiler()
{
  cerr << "[profiler] " << Name << " -> " << Time.elapsed() << " ms." << endl;
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

