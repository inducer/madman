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




#ifndef _HEADER_SEEN_HTTPD_H
#define _HEADER_SEEN_HTTPD_H



#include <qhttp.h>
#include <qserversocket.h>
#include "utility/base.h"
#include <stdexcept>




class QSocket;
class QUrl;




struct tHttpException : public runtime_error
{
  int HttpStatusCode;

  tHttpException(int status_code, const QString &str)
    : runtime_error(str.latin1()), HttpStatusCode(status_code)
    { }
};




class tHttpResponseHeader : public QHttpHeader
{
  public:
    tHttpResponseHeader();
    tHttpResponseHeader(const tHttpResponseHeader& header);
    tHttpResponseHeader(int code, const QString& text = QString::null, int majorVer = 1, int minorVer = 1);
    tHttpResponseHeader(const QString& str);

    void setStatusLine(int code, const QString& text = QString::null, int majorVer = 1, int minorVer = 1);

    int statusCode() const;
    QString reasonPhrase() const;

    int majorVersion() const;
    int minorVersion() const;

    bool parseLine(const QString& line, int number);
    QString toString() const;

  private:
    int statCode;
    QString reasonPhr;
    int majVer;
    int minVer;
};




class tHttpResponder 
{
  public:
    virtual bool canHandle(const QUrl &url, QHttpRequestHeader &header) = 0;
    /** Handles the request and gives ownership of the socket to
     * a tHttpResponseSender, which will eventually destroy it.
     */
    virtual void handle(const QUrl &url, const QHttpRequestHeader &header, QSocket *&socket) = 0;
};




class tHttpResponseSender : public QObject
{
    Q_OBJECT;

  protected:
    QSocket *Socket;

  private:
    QCString Header;
    unsigned SizeOfSentHeader;
    bool CurrentlySending;
    bool AlreadyClosing;

  public:
    tHttpResponseSender();
    virtual ~tHttpResponseSender();
    virtual void setHeader(tHttpResponseHeader &header);
    virtual void readyToSend() = 0;
    void kickOff(QSocket *&socket);

  public slots:
    void readyToSendInternal(int dummy);
    void connectionClosed();
};




class tHttpStringResponseSender : public tHttpResponseSender
{
    QCString    Document;
    unsigned 	SizeOfSentDocument;

    typedef tHttpResponseSender super;

  public:
    tHttpStringResponseSender(const QCString &document);
    void setHeader(tHttpResponseHeader &header);
    void readyToSend();
};




/** This class is for internal use only.
 */
class tHttpRequestReceiver : public QObject
{
    Q_OBJECT;

    QSocket	*Socket;
    QString	HeadersSoFar;

  public:
    /** This class does not assume ownership of the QSocket.
     */
    tHttpRequestReceiver(QSocket *socket);
  private slots:
    void dataReady();
    void connectionClosed();

  signals:
    void requestReceived(QSocket *socket, const QString &headers);
};




class tHttpDaemon : public QServerSocket
{
    Q_OBJECT;

    typedef vector<tHttpResponder *> 	tResponderList;
    tResponderList			ResponderList;
    tHttpResponder			*DefaultResponder;
    bool				RestrictToLocalhost;

  public:
    tHttpDaemon(int port, bool restrict_to_localhost, QObject* parent=0);
    ~tHttpDaemon();
    void newConnection(int socket);
    void addResponder(tHttpResponder *responder);
    void setDefaultResponder(tHttpResponder *responder);

  private slots:
    void requestReceived(QSocket *socket, const QString &headers);
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
