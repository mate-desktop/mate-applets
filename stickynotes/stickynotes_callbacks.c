/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>
#include <stickynotes_callbacks.h>

/* Sticky Window Callback : Lock/Unlock the window */
gboolean
stickynote_toggle_lock_cb (GtkWidget  *widget,
                           StickyNote *note)
{
    stickynote_set_locked (note, !note->locked);

    return TRUE;
}

/* Sticky Window Callback : Close the window. */
gboolean
stickynote_close_cb (GtkWidget  *widget,
                     StickyNote *note)
{
    stickynotes_remove (note);

    return TRUE;
}

/* Sticky Window Callback : Resize the window. */
gboolean stickynote_resize_cb (GtkWidget      *widget,
                               GdkEventButton *event,
                               StickyNote     *note)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        if (widget == note->w_resize_se)
            gtk_window_begin_resize_drag (GTK_WINDOW (note->w_window),
                                          GDK_WINDOW_EDGE_SOUTH_EAST,
                                          event->button, event->x_root,
                                          event->y_root, event->time);
        else /* if (widget == note->w_resize_sw) */
            gtk_window_begin_resize_drag (GTK_WINDOW (note->w_window),
                                          GDK_WINDOW_EDGE_SOUTH_WEST,
                                          event->button, event->x_root,
                                          event->y_root, event->time);
    }
    else
        return FALSE;

    return TRUE;
}

/* Sticky Window Callback : Move the window or edit the title. */
gboolean
stickynote_move_cb (GtkWidget      *widget,
                    GdkEventButton *event,
                    StickyNote     *note)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 1)
        gtk_window_begin_move_drag (GTK_WINDOW (note->w_window),
                                    event->button, event->x_root,
                                    event->y_root, event->time);
    else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
        stickynote_change_properties (note);
    else
        return FALSE;

    return TRUE;
}

/* Sticky Window Callback : Store settings when resizing/moving the window */
gboolean
stickynote_configure_cb (GtkWidget         *widget,
                         GdkEventConfigure *event,
                         StickyNote        *note)
{
    note->x = event->x;
    note->y = event->y;
    note->w = event->width;
    note->h = event->height;

    stickynotes_save ();

    return FALSE;
}

/* Sticky Window Callback : Get confirmation when deleting the window. */
gboolean
stickynote_delete_cb (GtkWidget  *widget,
                      GdkEvent   *event,
                      StickyNote *note)
{
    stickynotes_remove (note);

    return TRUE;
}

/* Sticky Window Callback : Popup the right click menu. */
gboolean
stickynote_show_popup_menu (GtkWidget      *widget,
                            GdkEventButton *event,
                            GtkWidget      *popup_menu)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        gtk_menu_popup_at_pointer (GTK_MENU (popup_menu),
                                   (const GdkEvent*) event);
    }

    return FALSE;
}

/* Sticky Window Callback : Exit entry field on key press or popup
 * the right click menu. */
gboolean
stickynote_keypress_cb (GtkWidget    *widget,
                        GdkEventKey  *event,
                        GtkMenu      *popup_menu)
{
    GdkModifierType state = event->state & gtk_accelerator_get_default_mod_mask ();

    switch (event->keyval) {
        case GDK_KEY_F6:
            if (state == 0)
                gtk_widget_child_focus(widget, GTK_DIR_TAB_FORWARD);
            else if (state == GDK_SHIFT_MASK)
                gtk_widget_child_focus(widget, GTK_DIR_TAB_BACKWARD);
            else
                break;
            return TRUE;
        default:
            break;
    }

    if ((event->keyval == GDK_KEY_F10 && (state == 0 || state == GDK_SHIFT_MASK)) ||
        (event->keyval == GDK_KEY_Menu && state == 0)) {
        GtkWidget *focus_widget = NULL;

        if (GTK_IS_WINDOW (widget))
            focus_widget = gtk_window_get_focus (GTK_WINDOW (widget));
        if (! focus_widget)
            focus_widget = widget;

        gtk_menu_popup_at_widget (popup_menu, focus_widget,
                                  GDK_GRAVITY_SOUTH_WEST,
                                  GDK_GRAVITY_NORTH_WEST,
                                  (const GdkEvent*) event);
        return TRUE;
    }

    return FALSE;
}

/* Popup Menu Callback : Create a new sticky note */
void popup_create_cb (GtkWidget  *widget,
                      StickyNote *note)
{
    stickynotes_add (gtk_widget_get_screen (note->w_window));
}

/* Popup Menu Callback : Destroy selected sticky note */
void
popup_destroy_cb (GtkWidget  *widget,
                  StickyNote *note)
{
    stickynotes_remove (note);
}

/* Popup Menu Callback : Lock/Unlock selected sticky note */
void
popup_toggle_lock_cb (GtkCheckMenuItem *item,
                      StickyNote       *note)
{
    stickynote_set_locked (note,
                           gtk_check_menu_item_get_active (item));
}

/* Popup Menu Callback : Change sticky note properties */
void
popup_properties_cb (GtkWidget  *widget,
                     StickyNote *note)
{
    stickynote_change_properties (note);
}

/* Properties Dialog Callback : Apply title */
void
properties_apply_title_cb (StickyNote *note)
{
    stickynote_set_title (note,
                          gtk_entry_get_text (GTK_ENTRY (note->w_entry)));
}

/* Properties Dialog Callback : Apply color */
void
properties_apply_color_cb (StickyNote *note)
{
    char *color_str = NULL;
    char *font_color_str = NULL;

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (note->w_def_color))) {
        GdkRGBA color, font_color;

        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (note->w_color),
                                    &color);
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (note->w_font_color),
                                    &font_color);

        color_str = gdk_rgba_to_string (&color);
        font_color_str = gdk_rgba_to_string (&font_color);
    }

    stickynote_set_color (note, color_str,
                          font_color_str, TRUE);

    g_free (color_str);
    g_free (font_color_str);
}

/* Properties Dialog Callback : Apply font */
void properties_apply_font_cb (StickyNote *note)
{
    const gchar *font_str = NULL;

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (note->w_def_font)))
        font_str = gtk_font_button_get_font_name (GTK_FONT_BUTTON (note->w_font));

    stickynote_set_font (note, font_str, TRUE);
}

/* Properties Dialog Callback : Color */
void
properties_color_cb (GtkWidget  *button,
                     StickyNote *note)
{
    properties_apply_color_cb (note);
}

/* Properties Dialog Callback : Font */
void
properties_font_cb (GtkWidget  *button,
                    StickyNote *note)
{
    const char *font_str = gtk_font_button_get_font_name (GTK_FONT_BUTTON (button));
    stickynote_set_font (note, font_str, TRUE);
}

/* Properties Dialog Callback : Activate */
void
properties_activate_cb (GtkWidget  *widget,
                        StickyNote *note)
{
    gtk_dialog_response (GTK_DIALOG (note->w_properties),
                         GTK_RESPONSE_CLOSE);
}
