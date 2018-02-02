/*
 * MATE panel x stuff
 *
 * Copyright (C) 2000, 2001 Eazel, Inc.
 *               2002 Sun Microsystems Inc.
 *
 * Authors: George Lebl <jirka@5z.com>
 *          Mark McLoughlin <mark@skynet.ie>
 *
 *  Contains code from the Window Maker window manager
 *
 *  Copyright (c) 1997-2002 Alfredo K. Kojima

 */
#include <config.h>
#include <unistd.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>

#include "xstuff.h"


/* Zoom animation */
#define MINIATURIZE_ANIMATION_FRAMES_Z   1
#define MINIATURIZE_ANIMATION_STEPS_Z    6
/* the delay per draw */
#define MINIATURIZE_ANIMATION_DELAY_Z    10

static void
draw_zoom_animation (GdkScreen *gscreen,
		     int x, int y, int w, int h,
		     int fx, int fy, int fw, int fh,
		     int steps)
{
#define FRAMES (MINIATURIZE_ANIMATION_FRAMES_Z)
	float cx[FRAMES], cy[FRAMES], cw[FRAMES], ch[FRAMES];
	int cxi[FRAMES], cyi[FRAMES], cwi[FRAMES], chi[FRAMES];
	float xstep, ystep, wstep, hstep;
	int i, j;
	GC frame_gc;
	XGCValues gcv;
	GdkColor color = { 65535, 65535, 65535 };
	Display *dpy;
	Window root_win;
	int screen;
	int depth;

	dpy = gdk_x11_display_get_xdisplay (gdk_screen_get_display (gscreen));
	root_win = GDK_WINDOW_XID (gdk_screen_get_root_window (gscreen));
	screen = gdk_x11_screen_get_screen_number (gscreen);
	depth = DefaultDepth(dpy,screen);

	/* frame GC */
	gcv.function = GXxor;
	/* this will raise the probability of the XORed color being different
	 * of the original color in PseudoColor when not all color cells are
	 * initialized */
	if (DefaultVisual(dpy, screen)->class==PseudoColor)
		gcv.plane_mask = (1<<(depth-1))|1;
	else
		gcv.plane_mask = AllPlanes;
	gcv.foreground = color.pixel;
	if (gcv.foreground == 0)
		gcv.foreground = 1;
	gcv.line_width = 1;
	gcv.subwindow_mode = IncludeInferiors;
	gcv.graphics_exposures = False;

	frame_gc = XCreateGC(dpy, root_win, GCForeground|GCGraphicsExposures
			     |GCFunction|GCSubwindowMode|GCLineWidth
			     |GCPlaneMask, &gcv);

	xstep = (float)(fx-x)/steps;
	ystep = (float)(fy-y)/steps;
	wstep = (float)(fw-w)/steps;
	hstep = (float)(fh-h)/steps;

	for (j=0; j<FRAMES; j++) {
		cx[j] = (float)x;
		cy[j] = (float)y;
		cw[j] = (float)w;
		ch[j] = (float)h;
		cxi[j] = (int)cx[j];
		cyi[j] = (int)cy[j];
		cwi[j] = (int)cw[j];
		chi[j] = (int)ch[j];
	}
	XGrabServer(dpy);
	for (i=0; i<steps; i++) {
		for (j=0; j<FRAMES; j++) {
			XDrawRectangle(dpy, root_win, frame_gc, cxi[j], cyi[j], cwi[j], chi[j]);
		}
		XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY_Z > 0)
		usleep(MINIATURIZE_ANIMATION_DELAY_Z);
#else
		usleep(10);
#endif
		for (j=0; j<FRAMES; j++) {
			XDrawRectangle(dpy, root_win, frame_gc,
				       cxi[j], cyi[j], cwi[j], chi[j]);
			if (j<FRAMES-1) {
				cx[j]=cx[j+1];
				cy[j]=cy[j+1];
				cw[j]=cw[j+1];
				ch[j]=ch[j+1];

				cxi[j]=cxi[j+1];
				cyi[j]=cyi[j+1];
				cwi[j]=cwi[j+1];
				chi[j]=chi[j+1];

			} else {
				cx[j]+=xstep;
				cy[j]+=ystep;
				cw[j]+=wstep;
				ch[j]+=hstep;

				cxi[j] = (int)cx[j];
				cyi[j] = (int)cy[j];
				cwi[j] = (int)cw[j];
				chi[j] = (int)ch[j];
			}
		}
	}

	for (j=0; j<FRAMES; j++) {
		XDrawRectangle(dpy, root_win, frame_gc,
				       cxi[j], cyi[j], cwi[j], chi[j]);
	}
	XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY_Z > 0)
	usleep(MINIATURIZE_ANIMATION_DELAY_Z);
#else
	usleep(10);
#endif
	for (j=0; j<FRAMES; j++) {
		XDrawRectangle(dpy, root_win, frame_gc,
				       cxi[j], cyi[j], cwi[j], chi[j]);
	}

	XUngrabServer(dpy);
	XFreeGC (dpy, frame_gc);
}
#undef FRAMES

void
xstuff_zoom_animate (GtkWidget *widget, GdkRectangle *opt_rect)
{
	GdkScreen    *gscreen;
	GdkRectangle  rect, dest;
	GtkAllocation allocation;
	GdkMonitor   *monitor;

	if (opt_rect)
		rect = *opt_rect;
	else {
		gdk_window_get_origin (gtk_widget_get_window (widget), &rect.x, &rect.y);
		gtk_widget_get_allocation (widget, &allocation);
		if (!gtk_widget_get_has_window (widget)) {
			rect.x += allocation.x;
			rect.y += allocation.y;
		}
		rect.height = allocation.height;
		rect.width = allocation.width;
	}

	gscreen = gtk_widget_get_screen (widget);
	monitor = gdk_display_get_monitor_at_window (gdk_screen_get_display (gscreen),
						     gtk_widget_get_window (widget));
	gdk_monitor_get_geometry (monitor, &dest);

	draw_zoom_animation (gscreen,
			     rect.x, rect.y, rect.width, rect.height,
			     dest.x, dest.y, dest.width, dest.height,
			     MINIATURIZE_ANIMATION_STEPS_Z);
}
