/*
 * trayicon_x11.cpp - X11 trayicon (for use with KDE and GNOME)
 * Copyright (C) 2003  Justin Karneges
 * GNOME2 Notification Area support: Tomasz Sterna
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "trayicon_x11.h"

/* for Gnome2 Notification Area */
static XErrorHandler old_handler = 0;
static int dock_xerror = 0;
static int dock_xerrhandler(Display* dpy, XErrorEvent* err)
{
	dock_xerror = err->error_code;
	return old_handler(dpy, err);
}

static void trap_errors()
{
	dock_xerror = 0;
	old_handler = XSetErrorHandler(dock_xerrhandler);
}

static bool untrap_errors()
{
	XSetErrorHandler(old_handler);
	return (dock_xerror == 0);
}

static bool send_message(
	Display* dpy, /* display */
	Window w,	 /* sender (tray icon window) */
	long message, /* message opcode */
	long data1,   /* message data 1 */
	long data2,   /* message data 2 */
	long data3	/* message data 3 */
){
	XEvent ev;

	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime;
	ev.xclient.data.l[1] = message;
	ev.xclient.data.l[2] = data1;
	ev.xclient.data.l[3] = data2;
	ev.xclient.data.l[4] = data3;

	trap_errors();
	XSendEvent(dpy, w, False, NoEventMask, &ev);
	XSync(dpy, False);
	return untrap_errors();
}

/*static QPixmap buildIcon(const QPixmap &_in)
{
	QImage image(22,22,32);
	image.fill(0);
	image.setAlphaBuffer(TRUE);

	QImage in = _in.convertToImage();
	//QImage in(22,22,32);

	// make sure it is no bigger than 22x22
	if(in.width() > 22 || in.height() > 22)
		in = in.smoothScale(22,22);

	// draw the pixmap onto the 22x22 image
	int xo = (image.width() - in.width()) / 2;
	int yo = (image.height() - in.height()) / 2;
	for(int n2 = 0; n2 < in.height(); ++n2) {
		for(int n = 0; n < in.width(); ++n) {
			if(qAlpha(in.pixel(n,n2))) {
				image.setPixel(n+xo, n2+yo, in.pixel(n,n2));
			}
		}
	}

	// convert to pixmap and return
	QPixmap icon;
	icon.convertFromImage(image);
	return icon;
}*/

//class TrayIcon::TrayIconPrivate : public QLabel

bool TrayIcon::TrayIconPrivate::x11Event(XEvent *e)
{
	if (e->type == ClientMessage){
		if (!inTray){
			Atom xembed_atom = XInternAtom(qt_xdisplay(), "_XEMBED", FALSE);
			if (e->xclient.message_type == xembed_atom){
				inTray = true;
				bInit = true;
			}
		}
	}
	if ((e->type == ReparentNotify) && !bInit && inNetTray){
		Display *dsp = qt_xdisplay();
		if (e->xreparent.parent == XRootWindow(dsp,
						XScreenNumberOfScreen(XDefaultScreenOfDisplay(dsp)))
		  ){
			inNetTray = false;
		}else{
			inTray = true;
			bInit = true;
			move(0, 0);
			resize(22, 22);
			XResizeWindow(dsp, winId(), 22, 22);
		}
	}
	if (((e->type == FocusIn) || (e->type == Expose)) && !bInit){
		if (!inTray){
			bInit = true;
			setFocusPolicy(NoFocus);
		}
	}
	return QWidget::x11Event(e);
}

void TrayIcon::sysInstall()
{
	if (d)
		return;

	d = new TrayIconPrivate(this, pm, v_isWMDock);
	sysUpdateToolTip();
	d->show();
}

void TrayIcon::sysRemove()
{
	if (!d)
		return;

	delete d;
	d = 0;
}

void TrayIcon::sysUpdateIcon()
{
	if (!d)
		return;

	QPixmap pix = pm; //buildIcon(pm);
	d->setPixmap(pix);
	//d->setMask(*pix.mask());
	d->repaint();
}

void TrayIcon::sysUpdateToolTip()
{
	if (!d)
		return;

	if(tip.isEmpty())
		QToolTip::remove(d);
	else
		QToolTip::add(d, tip);




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
