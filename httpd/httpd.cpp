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




#include <csignal>
#include <fcntl.h>

#include <qsocket.h>
#include <qregexp.h>
#include <qtextstream.h>
#include <qurl.h>

#include "httpd.h"




// tool functions -------------------------------------------------------------
QString getHttpStatusCodeText(int code)
{
  switch (code)
  {
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-Authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 306: return "(Unused)";
    case 307: return "Temporary Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Request Entity Too Large";
    case 414: return "Request-URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    default: return QString("<Unknown http status %1>").arg(code);
  }
}
// tHttpResponseHeader --------------------------------------------------------
tHttpResponseHeader::tHttpResponseHeader()
{
    setValid(FALSE);
}

tHttpResponseHeader::tHttpResponseHeader(int code, const QString& text, int majorVer, int minorVer)
    : QHttpHeader(), statCode(code), reasonPhr(text), majVer(majorVer), minVer(minorVer)
{
}

tHttpResponseHeader::tHttpResponseHeader(const tHttpResponseHeader& header)
    : QHttpHeader(header), statCode(header.statCode), reasonPhr(header.reasonPhr), majVer(header.majVer), minVer(header.minVer)
{
}

tHttpResponseHeader::tHttpResponseHeader(const QString& str)
    : QHttpHeader()
{
    parse(str);
}

void tHttpResponseHeader::setStatusLine(int code, const QString& text, int majorVer, int minorVer)
{
    setValid(TRUE);
    statCode = code;
    reasonPhr = text;
    majVer = majorVer;
    minVer = minorVer;
}

int tHttpResponseHeader::statusCode() const
{
    return statCode;
}

QString tHttpResponseHeader::reasonPhrase() const
{
    return reasonPhr;
}

int tHttpResponseHeader::majorVersion() const
{
    return majVer;
}

int tHttpResponseHeader::minorVersion() const
{
    return minVer;
}

bool tHttpResponseHeader::parseLine(const QString& line, int number)
{
    if (number != 0)
	return QHttpHeader::parseLine(line, number);

    QString l = line.simplifyWhiteSpace();
    if (l.length() < 10)
	return FALSE;

    if (l.left(5) == "HTTP/" && l[5].isDigit() && l[6] == '.' &&
	    l[7].isDigit() && l[8] == ' ' && l[9].isDigit()) {
	majVer = l[5].latin1() - '0';
	minVer = l[7].latin1() - '0';

	int pos = l.find(' ', 9);
	if (pos != -1) {
	    reasonPhr = l.mid(pos + 1);
	    statCode = l.mid(9, pos - 9).toInt();
	} else {
	    statCode = l.mid(9).toInt();
	    reasonPhr = QString::null;
	}
    } else {
	return FALSE;
    }

    return TRUE;
}

QString tHttpResponseHeader::toString() const
{
    QString ret("HTTP/%1.%2 %3 %4\r\n%5\r\n");
    return ret.arg(majVer).arg (minVer).arg(statCode).arg(reasonPhr).arg(QHttpHeader::toString());
}




// tHttpResponseSender --------------------------------------------------------
tHttpResponseSender::tHttpResponseSender()
: Socket(NULL), SizeOfSentHeader(0), CurrentlySending(false), AlreadyClosing(false)
{
}




tHttpResponseSender::~tHttpResponseSender()
{
  if (Socket)
    delete Socket;
}




void tHttpResponseSender::setHeader(tHttpResponseHeader &header)
{
  Header = header.toString().utf8();
}




void tHttpResponseSender::kickOff(QSocket *&socket)
{
  if (!socket)
    throw runtime_error("Attempted to kickOff tHttpResponseSender on NULL socket");

  Socket = socket;
  socket = NULL;

  connect(Socket, SIGNAL(delayedCloseFinished()),
      this, SLOT(connectionClosed()));
  connect(Socket, SIGNAL(connectionClosed()),
      this, SLOT(connectionClosed()));
  connect(Socket, SIGNAL(bytesWritten(int)),
      this, SLOT(readyToSendInternal(int)));

  readyToSendInternal(0);
}




void tHttpResponseSender::readyToSendInternal(int dummy)
{
  if (CurrentlySending)
    return;

  CurrentlySending = true;
  while (Socket->bytesToWrite() == 0 && Socket->state() == QSocket::Connected)
  {
    unsigned header_length = Header.length();
    while (SizeOfSentHeader < header_length)
    {
      int written_bytes = Socket->writeBlock(Header.data() + SizeOfSentHeader, 
	  header_length - SizeOfSentHeader);
      if (written_bytes == 0)
	return;
      SizeOfSentHeader += written_bytes;
    }

    readyToSend();
  }
  CurrentlySending = false;
}




void tHttpResponseSender::connectionClosed()
{
  if (!AlreadyClosing)
  {
    AlreadyClosing = true;
    deleteLater();
  }
}




// tHttpStringResponseSender --------------------------------------------------
tHttpStringResponseSender::tHttpStringResponseSender(const QCString &document)
: Document(document), SizeOfSentDocument(0)
{
}




void tHttpStringResponseSender::setHeader(tHttpResponseHeader &header)
{
  header.setValue("Content-Length", QString::number(Document.length()));
  super::setHeader(header);
}




void tHttpStringResponseSender::readyToSend()
{
  unsigned document_length = Document.length();
  while (SizeOfSentDocument < document_length)
  {
    int written_bytes = Socket->writeBlock(
	Document.data() + SizeOfSentDocument, 
	document_length - SizeOfSentDocument);
    if (written_bytes == 0)
      return;
    SizeOfSentDocument += written_bytes;
  }
  Socket->close();
}




// tHttpRequestReceiver -------------------------------------------------------
tHttpRequestReceiver::tHttpRequestReceiver(QSocket *socket)
: Socket(socket)
{
  connect(Socket, SIGNAL(readyRead()), this, SLOT(dataReady()));
}




void tHttpRequestReceiver::dataReady()
{
  char buffer[ 1024 ];
  Q_LONG read_bytes;

  do
  {
    read_bytes = Socket->readBlock(buffer, sizeof(buffer));
    HeadersSoFar += QString::fromAscii(buffer,read_bytes);
  }
  while (read_bytes != 0);

  // check for reception of complete request header
  QTextStream stream(&HeadersSoFar, IO_ReadOnly);
  QString line;
  do
  {
    line = stream.readLine();
    if (line == "")
    {
      emit requestReceived(Socket, HeadersSoFar);
      delete this;
      break;
    }
  }
  while (!line.isNull());
}




void tHttpRequestReceiver::connectionClosed()
{
  delete Socket;
  delete this;
}




// tHttpDaemon ----------------------------------------------------------------
tHttpDaemon::tHttpDaemon(int port, bool restrict_to_localhost, QObject* parent) 
: QServerSocket(port, 1, parent), DefaultResponder(NULL), 
  RestrictToLocalhost(restrict_to_localhost)
{
  // QSocket gives us a SIGPIPE when we write to someone who's already disconnected.
  // FIXME Need to investigage.
  // FIXME It's not good manners for a "library" function to mess with global 
  // resources like signals.
  signal(SIGPIPE, SIG_IGN);

  if (!ok()) 
    throw tRuntimeError(tr("Couldn't bind to port %1").arg(port));

  // close server socket on exec
  fcntl(socket(), F_SETFD, FD_CLOEXEC);
}




tHttpDaemon::~tHttpDaemon()
{
  FOREACH(first, ResponderList, tResponderList)
    delete *first;

  if (DefaultResponder)
    delete DefaultResponder;
}




void tHttpDaemon::newConnection(int socket)
{
  QSocket *s = new QSocket(this);
  s->setSocket(socket);

  tHttpRequestReceiver *rec = new tHttpRequestReceiver(s);
  connect(rec, SIGNAL(requestReceived(QSocket*, const QString&)),
      this, SLOT(requestReceived(QSocket*, const QString&)));
}




void tHttpDaemon::addResponder(tHttpResponder *responder)
{
  ResponderList.push_back(responder);
}




void tHttpDaemon::setDefaultResponder(tHttpResponder *responder)
{
  if (DefaultResponder)
    delete DefaultResponder;
  DefaultResponder = responder;
}




void tHttpDaemon::requestReceived(QSocket *socket, const QString &headers)
{
  try
  {
    if (!socket->address().isIp4Addr())
      throw tHttpException(403, "Non-IP4 connections are currently not allowed.");
    if (socket->address().ip4Addr() != 0x7f000001 && RestrictToLocalhost)
      throw tHttpException(403, "Remote connections have been disallowed.");

    QHttpRequestHeader request(headers);
    QUrl url;
    url.setEncodedPathAndQuery(request.path());

    bool handled = false;
    FOREACH(first, ResponderList, tResponderList)
      if ((*first)->canHandle(url, request))
      {
	(*first)->handle(url, request, socket);
	handled = true;
	break;
      }

    if (!handled)
    {
      if (DefaultResponder)
	DefaultResponder->handle(url, request, socket);
      else
	throw tRuntimeError("No default responder found");
    }
  }
  catch (tHttpException &ex)
  {
    // socket has already been taken by a responder.
    if (!socket)
    {
      cerr 
	<< "*** unhandlable http exception in http server:" << endl
	<< ex.what() << endl;
      return;
    }

    tHttpResponseHeader header(ex.HttpStatusCode, 
	getHttpStatusCodeText(ex.HttpStatusCode), 1, 1);
    header.setContentType("text/html; charset=UTF-8");
    tHttpResponseSender *sender = new tHttpStringResponseSender(
	
	QString("<h2>%1 %2</h2>" 
	"%3" 
	"<hr>madman %4 web services")
	.arg(ex.HttpStatusCode)
	.arg(getHttpStatusCodeText(ex.HttpStatusCode))
	.arg(ex.what())
	.arg(STRINGIFY(MADMAN_VERSION))
	.utf8()
	);
    sender->setHeader(header);
    sender->kickOff(socket);
  }
  catch (exception &ex)
  {
    // socket has already been taken by a responder.
    if (!socket)
    {
      cerr 
	<< "*** unhandlable http exception in http server:" << endl
	<< ex.what() << endl;
      return;
    }

    tHttpResponseHeader header(500, "Internal Server Error", 1, 1);
    header.setContentType("text/html; charset=UTF-8");
    tHttpResponseSender *sender = new tHttpStringResponseSender(
	QString("<h2>500 Internal Server Error</h2>" 
	"Sorry, I caught an exception while I was handling your request:<br>" 
	"<tt>%1</tt>"
	"<hr>madman %2 web services")
	.arg(ex.what()).arg(STRINGIFY(MADMAN_VERSION)).utf8()
	);
    sender->setHeader(header);
    sender->kickOff(socket);
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
