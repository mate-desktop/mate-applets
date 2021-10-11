/* -*- mode: c; c-basic-offset: 8 -*-
 * trashapplet.c
 *
 * Copyright (c) 2004  Michiel Sikkes <michiel@eyesopened.nl>,
 *               2004  Emmanuele Bassi <ebassi@gmail.com>
 *               2008  Ryan Lortie <desrt@desrt.ca>
 *                     Matthias Clasen <mclasen@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>

#include "trash-empty.h"
#include "xstuff.h"

#define TRASH_TYPE_APPLET (trash_applet_get_type ())

G_DECLARE_FINAL_TYPE (TrashApplet, trash_applet,
                      TRASH, APPLET, MatePanelApplet)

struct _TrashApplet
{
  MatePanelApplet applet;

  GFileMonitor *trash_monitor;
  GFile *trash;

  GtkImage *image;
  GIcon *icon;
  gint items;
};

G_DEFINE_TYPE (TrashApplet, trash_applet, PANEL_TYPE_APPLET);

static void trash_applet_do_empty    (GtkAction   *action,
                                      TrashApplet *applet);
static void trash_applet_show_about  (GtkAction   *action,
                                      TrashApplet *applet);
static void trash_applet_open_folder (GtkAction   *action,
                                      TrashApplet *applet);
static void trash_applet_show_help   (GtkAction   *action,
                                      TrashApplet *applet);

static const GtkActionEntry trash_applet_menu_actions [] = {
	{ "EmptyTrash", "edit-clear", N_("_Empty Trash"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_do_empty) },
	{ "OpenTrash", "document-open", N_("_Open Trash"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_open_folder) },
	{ "HelpTrash", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_show_help) },
	{ "AboutTrash", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_show_about) }
};

static void
trash_applet_monitor_changed (TrashApplet *applet)
{
  GError *error = NULL;
  GFileInfo *info;
  GIcon *icon;
  AtkObject *atk_obj;
  gint items;

  info = g_file_query_info (applet->trash,
                            G_FILE_ATTRIBUTE_STANDARD_ICON","
                            G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT,
                            0, NULL, &error);

  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (applet));

  if (!info)
    {
      g_critical ("could not query trash:/: '%s'", error->message);
      g_error_free (error);

      return;
    }

  icon = g_file_info_get_icon (info);
  items = g_file_info_get_attribute_uint32 (info,
                                            G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);

  if (!g_icon_equal (icon, applet->icon))
    {
      /* note: the size is meaningless here,
       * since we do set_pixel_size() later
       */
      gtk_image_set_from_gicon (GTK_IMAGE (applet->image),
                                icon, GTK_ICON_SIZE_MENU);

      if (applet->icon)
        g_object_unref (applet->icon);

      applet->icon = g_object_ref (icon);
    }

  if (items != applet->items)
    {
      char *text;

      if (items)
        {
          text = g_strdup_printf (ngettext ("%d Item in Trash",
                                            "%d Items in Trash",
                                            items), items);
        }
      else
        text = g_strdup (_("No Items in Trash"));

      gtk_widget_set_tooltip_text (GTK_WIDGET (applet), text);
      atk_object_set_description (atk_obj, text);
      g_free (text);

      applet->items = items;
    }

  g_object_unref (info);
}

static void
trash_applet_set_icon_size (TrashApplet *applet,
                            gint         size)
{
  /* copied from button-widget.c in the panel */
  if (size < 22)
    size = 16;
  else if (size < 24)
    size = 22;
  else if (size < 32)
    size = 24;
  else if (size < 48)
    size = 32;
  else
    size = 48;

  /* GtkImage already contains a check to do nothing if it's the same */
  gtk_image_set_pixel_size (applet->image, size);
}

static void
trash_applet_size_allocate (GtkWidget    *widget,
                            GdkRectangle *allocation)
{
  TrashApplet *applet = TRASH_APPLET (widget);

  GTK_WIDGET_CLASS (trash_applet_parent_class)
    ->size_allocate (widget, allocation);

  switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet)))
  {
    case MATE_PANEL_APPLET_ORIENT_LEFT:
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
      trash_applet_set_icon_size (applet, allocation->width);
      break;

    case MATE_PANEL_APPLET_ORIENT_UP:
    case MATE_PANEL_APPLET_ORIENT_DOWN:
      trash_applet_set_icon_size (applet, allocation->height);
      break;
  }

}

static void
trash_applet_dispose (GObject *object)
{
  TrashApplet *applet = TRASH_APPLET (object);

  if (applet->trash_monitor)
    g_object_unref (applet->trash_monitor);
  applet->trash_monitor = NULL;

  if (applet->trash)
    g_object_unref (applet->trash);
  applet->trash = NULL;

  if (applet->image)
    g_object_unref (applet->image);
  applet->image = NULL;

  if (applet->icon)
    g_object_unref (applet->icon);
  applet->icon = NULL;

  G_OBJECT_CLASS (trash_applet_parent_class)->dispose (object);
}

static void
trash_applet_init (TrashApplet *applet)
{
  const GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

  /* needed to clamp ourselves to the panel size */
  mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet), MATE_PANEL_APPLET_EXPAND_MINOR);

  /* setup the image */
  applet->image = g_object_ref_sink (GTK_IMAGE (gtk_image_new ()));
  gtk_container_add (GTK_CONTAINER (applet),
                     GTK_WIDGET (applet->image));
  gtk_widget_show (GTK_WIDGET (applet->image));

  /* setup the trash backend */
  applet->trash = g_file_new_for_uri ("trash:/");
  applet->trash_monitor = g_file_monitor_file (applet->trash, 0, NULL, NULL);
  g_signal_connect_swapped (applet->trash_monitor, "changed",
                            G_CALLBACK (trash_applet_monitor_changed),
                            applet);

  /* setup drag and drop */
  gtk_drag_dest_set (GTK_WIDGET (applet), GTK_DEST_DEFAULT_ALL,
                     drop_types, G_N_ELEMENTS (drop_types),
                     GDK_ACTION_MOVE);

  /* synthesise the first update */
  applet->items = -1;
  trash_applet_monitor_changed (applet);
}

#define PANEL_SCHEMA "org.mate.panel"
#define PANEL_ENABLE_ANIMATIONS "enable-animations"
static gboolean
trash_applet_button_release (GtkWidget      *widget,
                             GdkEventButton *event)
{
  TrashApplet *applet = TRASH_APPLET (widget);
  static GSettings *settings;

  if (settings == NULL)
    settings = g_settings_new (PANEL_SCHEMA);

  if (event->button == 1)
    {
      if (g_settings_get_boolean (settings, PANEL_ENABLE_ANIMATIONS))
        xstuff_zoom_animate (widget, NULL);

      trash_applet_open_folder (NULL, applet);

      return TRUE;
    }

  if (GTK_WIDGET_CLASS (trash_applet_parent_class)->button_release_event)
    return GTK_WIDGET_CLASS (trash_applet_parent_class)
        ->button_release_event (widget, event);
  else
    return FALSE;
}
static gboolean
trash_applet_key_press (GtkWidget   *widget,
                        GdkEventKey *event)
{
  TrashApplet *applet = TRASH_APPLET (widget);

  switch (event->keyval)
    {
     case GDK_KEY_KP_Enter:
     case GDK_KEY_ISO_Enter:
     case GDK_KEY_3270_Enter:
     case GDK_KEY_Return:
     case GDK_KEY_space:
     case GDK_KEY_KP_Space:
      trash_applet_open_folder (NULL, applet);
      return TRUE;

     default:
      break;
    }

  if (GTK_WIDGET_CLASS (trash_applet_parent_class)->key_press_event)
    return GTK_WIDGET_CLASS (trash_applet_parent_class)
      ->key_press_event (widget, event);
  else
    return FALSE;
}

static gboolean
trash_applet_drag_motion (GtkWidget      *widget,
                          GdkDragContext *context,
                          gint            x,
                          gint            y,
                          guint           time)
{
  GList *target;

  /* refuse drops of panel applets */
  for (target = gdk_drag_context_list_targets (context); target; target = target->next)
    {
      const char *name = gdk_atom_name (target->data);

      if (!strcmp (name, "application/x-panel-icon-internal"))
        break;
    }

  if (target)
    gdk_drag_status (context, 0, time);
  else
    gdk_drag_status (context, GDK_ACTION_MOVE, time);

  return TRUE;
}

/* TODO - Must HIGgify this dialog */
static void
error_dialog (TrashApplet *applet, const gchar *error, ...)
{
  va_list args;
  gchar *error_string;
  GtkWidget *dialog;

  g_return_if_fail (error != NULL);

  va_start (args, error);
  error_string = g_strdup_vprintf (error, args);
  va_end (args);

  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                   "%s", error_string);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_screen (GTK_WINDOW(dialog),
                         gtk_widget_get_screen (GTK_WIDGET (applet)));
  gtk_widget_show (dialog);

  g_free (error_string);
}

static void
trash_applet_do_empty (GtkAction   *action,
                       TrashApplet *applet)
{
  trash_empty (GTK_WIDGET (applet));
}

static void
trash_applet_open_folder (GtkAction   *action,
                          TrashApplet *applet)
{
  GError *err = NULL;

  gtk_show_uri_on_window (NULL,
                          "trash:",
                          gtk_get_current_event_time (),
                          &err);

  if (err)
    {
      error_dialog (applet, _("Error while spawning caja:\n%s"),
      err->message);
      g_error_free (err);
    }
}

static void
trash_applet_show_help (GtkAction   *action,
                        TrashApplet *applet)
{
  GError *err = NULL;

  /* FIXME - Actually, we need a user guide */
  gtk_show_uri_on_window (NULL,
                          "help:mate-trashapplet",
                          gtk_get_current_event_time (),
                          &err);

  if (err)
    {
      error_dialog (applet,
                    _("There was an error displaying help: %s"),
                    err->message);
      g_error_free (err);
    }
}


static void
trash_applet_show_about (GtkAction   *action,
                         TrashApplet *applet)
{
  static const char *authors[] = {
    "Michiel Sikkes <michiel@eyesopened.nl>",
    "Emmanuele Bassi <ebassi@gmail.com>",
    "Sebastian Bacher <seb128@canonical.com>",
    "James Henstridge <james.henstridge@canonical.com>",
    "Ryan Lortie <desrt@desrt.ca>",
    NULL
  };

  static const char *documenters[] = {
    "Michiel Sikkes <michiel@eyesopened.nl>",
    N_("MATE Documentation Team"),
    NULL
  };

#ifdef ENABLE_NLS
  const char **p;
  for (p = documenters; *p; ++p)
    *p = _(*p);
#endif

  gtk_show_about_dialog (NULL,
                         "title", _("About Trash Applet"),
                         "version", VERSION,
                         "copyright", _("Copyright \xc2\xa9 2004 Michiel Sikkes\n"
                                        "Copyright \xc2\xa9 2008 Ryan Lortie\n"
                                        "Copyright \xc2\xa9 2012-2021 MATE developers"),
                         "comments", _("A MATE trash bin that lives in your panel. "
                                       "You can use it to view the trash or drag "
                                       "and drop items into the trash."),
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         "logo_icon_name", "user-trash-full",
                         NULL);
}

static gboolean
confirm_delete_immediately (GtkWidget *parent_view,
                            gint num_files,
                            gboolean all)
{
  GdkScreen *screen;
  GtkWidget *dialog, *hbox, *vbox, *image, *label;
  gchar *str, *prompt, *detail;
  int response;

  screen = gtk_widget_get_screen (parent_view);

  dialog = gtk_dialog_new ();
  gtk_window_set_screen (GTK_WINDOW (dialog), screen);
  atk_object_set_role (gtk_widget_get_accessible (dialog), ATK_ROLE_ALERT);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Delete Immediately?"));
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gtk_widget_realize (dialog);
  gdk_window_set_transient_for (gtk_widget_get_window (GTK_WIDGET (dialog)),
                                gdk_screen_get_root_window (screen));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 14);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), hbox,
                      FALSE, FALSE, 0);

  image = gtk_image_new_from_icon_name ("dialog-question",
                                        GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  if (all)
    {
      prompt = _("Cannot move items to trash, do you want to delete them immediately?");
      detail = g_strdup_printf ("None of the %d selected items can be moved to the Trash", num_files);
    }
  else
    {
      prompt = _("Cannot move some items to trash, do you want to delete these immediately?");
      detail = g_strdup_printf ("%d of the selected items cannot be moved to the Trash", num_files);
    }

  str = g_strconcat ("<span weight=\"bold\" size=\"larger\">",
                     prompt, "</span>", NULL);
  label = gtk_label_new (str);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  g_free (str);

  label = gtk_label_new (detail);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  g_free (detail);

  gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-cancel",
                         GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-delete",
                         GTK_RESPONSE_YES);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_YES);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (GTK_WIDGET (dialog));

  return response == GTK_RESPONSE_YES;
}

static void
trash_applet_drag_data_received (GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *selectiondata,
                                 guint             info,
                                 guint             time_)
{
  gchar **list;
  gint i;
  GList *trashed = NULL;
  GList *untrashable = NULL;
  GList *l;
  GError *error = NULL;

  list = g_uri_list_extract_uris ((gchar *)gtk_selection_data_get_data (selectiondata));

  for (i = 0; list[i]; i++)
    {
      GFile *file;

      file = g_file_new_for_uri (list[i]);

      if (!g_file_trash (file, NULL, NULL))
        {
          untrashable = g_list_prepend (untrashable, file);
        }
      else
        {
          trashed = g_list_prepend (trashed, file);
        }
    }

  if (untrashable)
    {
      if (confirm_delete_immediately (widget,
                                      g_list_length (untrashable),
                                      trashed == NULL))
        {
          for (l = untrashable; l; l = l->next)
            {
              if (!g_file_delete (l->data, NULL, &error))
                {
/*
* FIXME: uncomment me after branched (we're string frozen)
                  error_dialog (applet,
                                _("Unable to delete '%s': %s"),
                                g_file_get_uri (l->data),
                                error->message);
*/
                                g_clear_error (&error);
                }
            }
        }
    }

  g_list_free_full (untrashable, g_object_unref);
  g_list_free_full (trashed, g_object_unref);

  g_strfreev (list);

  gtk_drag_finish (context, TRUE, FALSE, time_);
}

static void
trash_applet_class_init (TrashAppletClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->dispose = trash_applet_dispose;
  widget_class->size_allocate = trash_applet_size_allocate;
  widget_class->button_release_event = trash_applet_button_release;
  widget_class->key_press_event = trash_applet_key_press;
  widget_class->drag_motion = trash_applet_drag_motion;
  widget_class->drag_data_received = trash_applet_drag_data_received;
}

static gboolean
trash_applet_factory (MatePanelApplet *applet,
                      const gchar *iid,
                      gpointer     data)
{
  gboolean retval = FALSE;

  if (!strcmp (iid, "TrashApplet"))
    {
      AtkObject *atk_obj;
      GtkActionGroup *action_group;

      g_set_application_name (_("Trash Applet"));

      gtk_window_set_default_icon_name ("user-trash");

      /* Set up the menu */
      action_group = gtk_action_group_new ("Trash Applet Actions");
      gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
      gtk_action_group_add_actions (action_group,
				    trash_applet_menu_actions,
				    G_N_ELEMENTS (trash_applet_menu_actions),
				    applet);
      mate_panel_applet_setup_menu_from_resource (applet,
                                                  GRESOURCE "trashapplet-menu.xml",
                                                  action_group);
      g_object_unref (action_group);

      atk_obj = gtk_widget_get_accessible (GTK_WIDGET (applet));

      if (GTK_IS_ACCESSIBLE (atk_obj)) {
          atk_object_set_name (atk_obj, _("Trash Applet"));
      }

      gtk_widget_show (GTK_WIDGET (applet));

      retval = TRUE;
  }

  return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("TrashAppletFactory",
				  TRASH_TYPE_APPLET,
				  "TrashApplet",
				  trash_applet_factory,
				  NULL)
