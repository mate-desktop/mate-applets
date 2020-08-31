/* Keyboard Accessibility Status Applet
 * Copyright 2003 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __ACCESSX_APPLET_H__
#define __ACCESSX_APPLET_H__

#include <gtk/gtk.h>

#include <mate-panel-applet.h>

#define ACCESSX_APPLET            "preferences-desktop-accessibility"

#define ACCESSX_BASE_ICON         "mate-ax-key-none"
#define ACCESSX_BASE_ICON_BASE    "mate-ax-key-base"
#define ACCESSX_BASE_ICON_INVERSE "mate-ax-key-inverse"
#define ACCESSX_ACCEPT_BASE       "mate-ax-key-yes"
#define ACCESSX_REJECT_BASE       "mate-ax-key-no"

#define MOUSEKEYS_BASE_ICON       "mate-mousekeys-base"
#define MOUSEKEYS_BUTTON_LEFT     "mate-mousekeys-pressed-left"
#define MOUSEKEYS_BUTTON_MIDDLE   "mate-mousekeys-pressed-middle"
#define MOUSEKEYS_BUTTON_RIGHT    "mate-mousekeys-pressed-right"
#define MOUSEKEYS_DOT_LEFT        "mate-mousekeys-default-left"
#define MOUSEKEYS_DOT_MIDDLE      "mate-mousekeys-default-middle"
#define MOUSEKEYS_DOT_RIGHT       "mate-mousekeys-default-right"

#define SHIFT_KEY_ICON            "mate-sticky-shift-none"
#define SHIFT_KEY_ICON_LATCHED    "mate-sticky-shift-latched"
#define SHIFT_KEY_ICON_LOCKED     "mate-sticky-shift-locked"

#define CONTROL_KEY_ICON          "mate-sticky-ctrl-none"
#define CONTROL_KEY_ICON_LATCHED  "mate-sticky-ctrl-latched"
#define CONTROL_KEY_ICON_LOCKED   "mate-sticky-ctrl-locked"

#define ALT_KEY_ICON              "mate-sticky-alt-none"
#define ALT_KEY_ICON_LATCHED      "mate-sticky-alt-latched"
#define ALT_KEY_ICON_LOCKED       "mate-sticky-alt-locked"

#define META_KEY_ICON             "mate-sticky-meta-none"
#define META_KEY_ICON_LATCHED     "mate-sticky-meta-latched"
#define META_KEY_ICON_LOCKED      "mate-sticky-meta-locked"

#define HYPER_KEY_ICON            "mate-sticky-hyper-none"
#define HYPER_KEY_ICON_LATCHED    "mate-sticky-hyper-latched"
#define HYPER_KEY_ICON_LOCKED     "mate-sticky-hyper-locked"

#define SUPER_KEY_ICON            "mate-sticky-super-none"
#define SUPER_KEY_ICON_LATCHED    "mate-sticky-super-latched"
#define SUPER_KEY_ICON_LOCKED     "mate-sticky-super-locked"

#define ALTGRAPH_KEY_ICON         "mate-sticky-alt-none"
#define ALTGRAPH_KEY_ICON_LATCHED "mate-sticky-alt-latched"
#define ALTGRAPH_KEY_ICON_LOCKED  "mate-sticky-alt-locked"

#define SLOWKEYS_IDLE_ICON        "mate-ax-slowkeys"
#define SLOWKEYS_PENDING_ICON     "mate-ax-slowkeys-pending"
#define SLOWKEYS_ACCEPT_ICON      "mate-ax-slowkeys-yes"
#define SLOWKEYS_REJECT_ICON      "mate-ax-slowkeys-no"

#define BOUNCEKEYS_ICON           "mate-ax-bouncekeys"

typedef enum {
    ACCESSX_STATUS_ERROR_NONE = 0,
    ACCESSX_STATUS_ERROR_XKB_DISABLED,
    ACCESSX_STATUS_ERROR_UNKNOWN
}AccessxStatusErrorType;

typedef struct {
    MatePanelApplet* applet;
    GtkWidget* box;
    GtkWidget* idlefoo;
    GtkWidget* mousefoo;
    GtkWidget* stickyfoo;
    GtkWidget* slowfoo;
    GtkWidget* bouncefoo;
    GtkWidget* shift_indicator;
    GtkWidget* ctrl_indicator;
    GtkWidget* alt_indicator;
    GtkWidget* meta_indicator;
    GtkWidget* hyper_indicator;
    GtkWidget* super_indicator;
    GtkWidget* alt_graph_indicator;
    MatePanelAppletOrient orient;
    GtkIconFactory* icon_factory;
    gboolean initialized;
    XkbDescRec* xkb;
    Display* xkb_display;
    AccessxStatusErrorType error_type;
} AccessxStatusApplet;

typedef enum {
    ACCESSX_STATUS_MODIFIERS = 1 << 0,
    ACCESSX_STATUS_SLOWKEYS = 1 << 1,
    ACCESSX_STATUS_BOUNCEKEYS = 1 << 2,
    ACCESSX_STATUS_MOUSEKEYS = 1 << 3,
    ACCESSX_STATUS_ENABLED = 1 << 4,
    ACCESSX_STATUS_ALL = 0xFFFF
} AccessxStatusNotifyType;

#endif
