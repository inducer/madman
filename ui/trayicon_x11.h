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




#ifndef _HEADER_SEEN_TRAYICON_X11_H
#define _HEADER_SEEN_TRAYICON_X11_H




#include "trayicon.h"
#include<qapplication.h>
#include<qimage.h>
#include<qpixmap.h>
#include<qbitmap.h>
#include<qcursor.h>
#include<qlabel.h>
#include<qtooltip.h>
#include<qpainter.h>

#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/Xatom.h>




#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2




static bool send_message(
	Display* dpy, /* display */
	Window w,	 /* sender (tray icon window) */
	long message, /* message opcode */
	long data1,   /* message data 1 */
	long data2,   /* message data 2 */
	long data3	/* message data 3 */
);




class TrayIcon::TrayIconPrivate : public QWidget
{
public:
	TrayIconPrivate(TrayIcon *object, const QPixmap &pm, bool _isWMDock)
	: QWidget(NULL, "psidock",  WType_TopLevel | WStyle_Customize | WStyle_NoBorder | WStyle_StaysOnTop | WMouseNoMask), iconObject(object)
//	: QLabel(0, "psidock", WMouseNoMask), iconObject(object)
	{
		isWMDock = _isWMDock;
		inNetTray = false;
		bInit = false;
		inTray = false;

		setMinimumSize(22, 22);
		setPixmap(pm);

		Display *dsp = x11Display(); // get the display
		WId win = winId();		 // get the window
		
		XClassHint classhint;
		classhint.res_name  = (char*)"psidock";
		classhint.res_class = (char*)"Wharf";
		XSetClassHint(dsp, win, &classhint);
		XWMHints *hints;  // hints
		hints = XGetWMHints(dsp, win);  // init hints
		hints->initial_state = WithdrawnState;
		hints->icon_x = 0;
		hints->icon_y = 0;
		hints->icon_window = winId();
		hints->window_group = win;  // set the window hint
		hints->flags = WindowGroupHint | IconWindowHint | IconPositionHint | StateHint; // set the window group hint
		XSetWMHints(dsp, win, hints);  // set the window hints for WM to use.
		XFree(hints);

		// WindowMaker
		if(isWMDock) {
			//QPixmap pix = pm;
			//setPixmap(pix);
			resize(64,64);
			update();

			/*Display *dsp = x11Display();  // get the display
			WId win = winId();     // get the window
			//XClassHint classhint;  // class hints
			//classhint.res_name = "psidock";  // res_name
			//classhint.res_class = "Wharf";  // res_class
			//XSetClassHint(dsp, win, &classhint); // set the class hints
			XWMHints *hints;  // hints
			hints = XGetWMHints(dsp, win);  // init hints
			hints->initial_state = WithdrawnState;
			hints->icon_x = 0;
			hints->icon_y = 0;
			hints->icon_window = winId(); //wharfIcon->winId();
			hints->window_group = win;  // set the window hint
			hints->flags = WindowGroupHint | IconWindowHint | IconPositionHint | StateHint; // set the window group hint
			XSetWMHints(dsp, win, hints);  // set the window hints for WM to use.
			XFree(hints);*/
		}
		// KDE/GNOME
		else {
			setWFlags(WRepaintNoErase);

			//QPixmap pix = pm; //buildIcon(pm);
			//setPixmap(pix);
			resize(22,22);
			update();

			//setMask(*pix.mask());

			/*// dock the widget (adapted from Gabber / gdk)
			Display *dsp = x11Display(); // get the display
			WId win = winId();           // get the window
			int r;
			int data = 1;
			r = XInternAtom(dsp, "KWM_DOCKWINDOW", false);
			XChangeProperty(dsp, win, r, r, 32, 0, (uchar *)&data, 1);
			r = XInternAtom(dsp, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", false);
			XChangeProperty(dsp, win, r, XA_WINDOW, 32, 0, (uchar *)&data, 1);*/

			inNetTray = false;

			// dock the widget (adapted from SIM-ICQ)
			Screen *screen = XDefaultScreenOfDisplay (dsp); // get the screen
			int screen_id = XScreenNumberOfScreen(screen); // and it's number
			char buf[32];
			snprintf(buf, sizeof(buf), "_NET_SYSTEM_TRAY_S%d", screen_id);
			Atom selection_atom = XInternAtom(dsp, buf, false);
			XGrabServer(dsp);
			Window manager_window = XGetSelectionOwner(dsp, selection_atom);
			if (manager_window != None)
				XSelectInput(dsp, manager_window, StructureNotifyMask);
			XUngrabServer(dsp);
			XFlush(dsp);
			if (manager_window != None){
				inNetTray = true;
				if (!send_message(dsp, manager_window, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0)){
					inNetTray = false;
				}
			}

			Atom kwm_dockwindow_atom = XInternAtom(dsp, "KWM_DOCKWINDOW", false);
			Atom kde_net_system_tray_window_for_atom = XInternAtom(dsp, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", false);

			long data[1];
			data[0] = 0;
			XChangeProperty(dsp, win, kwm_dockwindow_atom, kwm_dockwindow_atom,
					32, PropModeReplace,
					(unsigned char*)data, 1);
			XChangeProperty(dsp, win, kde_net_system_tray_window_for_atom, XA_WINDOW,
					32, PropModeReplace,
					(unsigned char*)data, 1);

			resize(22, 22);
			if (!inNetTray) {
				move(-21, -21);
			}
			show();
		}
	}

	~TrayIconPrivate()
	{
	}

	void mouseMoveEvent(QMouseEvent *e)
	{
		QApplication::sendEvent(iconObject, e);
	}

	void mousePressEvent(QMouseEvent *e)
	{
		QApplication::sendEvent(iconObject, e);
	}

	void mouseReleaseEvent(QMouseEvent *e)
	{
		QApplication::sendEvent(iconObject, e);
	}

	void mouseDoubleClickEvent(QMouseEvent *e)
	{
		QApplication::sendEvent(iconObject, e);
	}

	void closeEvent(QCloseEvent *e)
	{
		iconObject->gotCloseEvent();
		e->accept();
	}

	void paintEvent(QPaintEvent*)
	{
		QPainter p(this);
		p.drawPixmap((width() - pix.width())/2, (height() - pix.height())/2, pix);
	}

	inline void setPixmap (const QPixmap & pm)
	{
		pix = pm;
	}

	bool x11Event(XEvent *e);
	
	QPixmap pix;
	TrayIcon *iconObject;
	bool isWMDock;
	bool inNetTray;
	bool inTray;
	bool bInit;
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
