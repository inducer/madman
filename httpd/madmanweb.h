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




#ifndef _HEADER_SEEN_MADMANWEB_H
#define _HEADER_SEEN_MADMANWEB_H




#include <fstream>
#include <memory>

#include "httpd.h"
#include "madmanweb_external.h"




class tHttpFileResponseSender : public tHttpResponseSender
{
    auto_ptr<ifstream>	File;
    unsigned 		SizeOfSentDocument, TotalSize;

    typedef tHttpResponseSender super;

  public:
    tHttpFileResponseSender(ifstream *file);
    void setHeader(tHttpResponseHeader &header);
    void readyToSend();
};




class tHttpBufferResponseSender : public tHttpResponseSender
{
    const char		*Buffer;
    unsigned		BufferLength;
    unsigned 		SizeOfSentDocument;

    typedef tHttpResponseSender super;

  public:
    tHttpBufferResponseSender(const char *buffer, unsigned length);
    void setHeader(tHttpResponseHeader &header);
    void readyToSend();
};




class tStreamingHttpResponder : public tHttpResponder
{
  public:
    bool canHandle(const QUrl &url, QHttpRequestHeader &header);
    void handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket);
};




class tStaticDataHttpResponder : public tHttpResponder
{
  public:
    bool canHandle(const QUrl &url, QHttpRequestHeader &header);
    void handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket);
};




class tBrowseHttpResponder : public tHttpResponder
{
  public:
    QString getTitleCell(const QUrl &url, const QString &human_readable, const QString &machine_readable);
    QString getTitleRow(const QUrl &url);
    QString getSongRow(tSong *song, bool bright);
    QString generateMenu(const QUrl &url);

    bool canHandle(const QUrl &url, QHttpRequestHeader &header);
    void handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket);
};




class tScriptingHttpResponder : public tHttpResponder
{
  public:
    bool canHandle(const QUrl &url, QHttpRequestHeader &header);
    void handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket);
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
