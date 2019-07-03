/*
 * Copyright (C) 2018 Alberts Muktupāvels
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "ma-command.h"

#define BUFFER_SIZE 64

struct _MaCommand
{
  GObject   parent;

  gchar    *command;
  gchar   **argv;
};

typedef struct
{
  GPid        pid;

  GIOChannel *channel;

  GString    *input;

  guint       io_watch_id;
  guint       child_watch_id;
} CommandData;

enum
{
  PROP_0,

  PROP_COMMAND,

  LAST_PROP
};

static GParamSpec *command_properties[LAST_PROP] = { NULL };

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MaCommand, ma_command, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                initable_iface_init))

static gboolean
read_cb (GIOChannel   *source,
         GIOCondition  condition,
         gpointer      user_data)
{
  GTask *task;
  CommandData *data;
  gchar buffer[BUFFER_SIZE];
  gsize bytes_read;
  GError *error;
  GIOStatus status;

  task = (GTask *) user_data;
  data = g_task_get_task_data (task);

  if (g_task_return_error_if_cancelled (task))
    {
      g_object_unref (task);

      data->io_watch_id = 0;

      return G_SOURCE_REMOVE;
    }

  error = NULL;
  status = g_io_channel_read_chars (source, buffer, BUFFER_SIZE,
                                    &bytes_read, &error);

  if (status == G_IO_STATUS_AGAIN)
    {
      g_clear_error (&error);

      return G_SOURCE_CONTINUE;
    }
  else if (status != G_IO_STATUS_NORMAL)
    {
      if (error != NULL)
        {
          g_task_return_error (task, error);
          g_object_unref (task);
        }

      data->io_watch_id = 0;

      return G_SOURCE_REMOVE;
    }

  g_string_append_len (data->input, buffer, bytes_read);

  return G_SOURCE_CONTINUE;
}

static void
child_watch_cb (GPid     pid,
                gint     status,
                gpointer user_data)
{
  GTask *task;
  CommandData *data;

  task = (GTask *) user_data;
  data = g_task_get_task_data (task);

  g_task_return_pointer (task, g_strdup (data->input->str), g_free);
  g_object_unref (task);
}

static void
cancelled_cb (GCancellable *cancellable,
              gpointer      user_data)
{
  GTask *task;

  task = G_TASK (user_data);

  g_object_unref (task);
}

static void
command_data_free (gpointer user_data)
{
  CommandData *data;

  data = (CommandData *) user_data;

  if (data->pid != 0)
    {
      g_spawn_close_pid (data->pid);
      data->pid = 0;
    }

  if (data->channel != NULL)
    {
      g_io_channel_unref (data->channel);
      data->channel = NULL;
    }

  if (data->input != NULL)
    {
      g_string_free (data->input, TRUE);
      data->input = NULL;
    }

  if (data->io_watch_id != 0)
    {
      g_source_remove (data->io_watch_id);
      data->io_watch_id = 0;
    }

  if (data->child_watch_id != 0)
    {
      g_source_remove (data->child_watch_id);
      data->child_watch_id = 0;
    }

  g_free (data);
}

static gboolean
ma_command_initable_init (GInitable     *initable,
                          GCancellable  *cancellable,
                          GError       **error)
{
  MaCommand *command;

  command = MA_COMMAND (initable);

  if (command->command == NULL || *command->command == '\0')
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                   "Empty command");

      return FALSE;
    }

  return TRUE;
}

static void
initable_iface_init (GInitableIface *iface)
{
  iface->init = ma_command_initable_init;
}

static void
ma_command_finalize (GObject *object)
{
  MaCommand *command;

  command = MA_COMMAND (object);

  g_clear_pointer (&command->command, g_free);
  g_clear_pointer (&command->argv, g_strfreev);

  G_OBJECT_CLASS (ma_command_parent_class)->finalize (object);
}

static void
ma_command_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  MaCommand *command;

  command = MA_COMMAND (object);

  switch (property_id)
    {
      case PROP_COMMAND:
          //g_assert (command->command == NULL);
          command->command = g_value_dup_string (value);
          if (command->argv && *command->argv != NULL) {
              g_strfreev(command->argv);
          }
          g_shell_parse_argv (command->command, NULL, &command->argv, NULL);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ma_command_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    MaCommand *command;

    command = MA_COMMAND (object);

    switch (prop_id)
    {
        case PROP_COMMAND:
            g_value_set_string (value, command->command);
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
install_properties (GObjectClass *object_class)
{
  command_properties[PROP_COMMAND] =
    g_param_spec_string ("command", "command", "command",
                         NULL,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP,
                                     command_properties);
}

static void
ma_command_class_init (MaCommandClass *command_class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (command_class);

  object_class->finalize = ma_command_finalize;
  object_class->set_property = ma_command_set_property;
  object_class->get_property = ma_command_get_property;

  install_properties (object_class);
}

static void
ma_command_init (MaCommand *command)
{
}

/**
 * ma_command_new:
 * @command: a command
 * @error: (nullable): return location for an error, or %NULL
 *
 * Creates a new #MaCommand.
 *
 * Returns: (nullable): a newly allocated #MaCommand
 */
MaCommand *
ma_command_new (const gchar  *command,
                GError      **error)
{
  return g_initable_new (MA_TYPE_COMMAND, NULL, error,
                         "command", command,
                         NULL);
}

/**
 * ma_command_run_async:
 * @command: a #MaCommand
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to @callback
 *
 * Request an asynchronous read of output from command that was passed
 * to ma_command_new().
 */
void
ma_command_run_async (MaCommand           *command,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  GTask *task;
  CommandData *data;
  GSpawnFlags spawn_flags;
  gint command_stdout;
  GError *error;
  GIOChannel *channel;
  GIOStatus status;
  GIOCondition condition;

  g_return_if_fail (MA_IS_COMMAND (command));
  g_return_if_fail (callback != NULL);

  task = g_task_new (command, cancellable, callback, user_data);
  g_task_set_source_tag (task, ma_command_run_async);

  if (cancellable)
    {
      g_signal_connect_object (cancellable, "cancelled",
                               G_CALLBACK (cancelled_cb), task,
                               G_CONNECT_AFTER);
    }

  data = g_new0 (CommandData, 1);
  g_task_set_task_data (task, data, command_data_free);

  spawn_flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;
  error = NULL;

  if (!g_spawn_async_with_pipes (NULL, command->argv, NULL, spawn_flags,
                                 NULL, NULL, &data->pid, NULL, &command_stdout,
                                 NULL, &error))
    {
      g_task_return_error (task, error);
      g_object_unref (task);

      return;
    }

  channel = data->channel = g_io_channel_unix_new (command_stdout);
  g_io_channel_set_close_on_unref (channel, TRUE);

  g_assert (error == NULL);
  status = g_io_channel_set_encoding (channel, NULL, &error);

  if (status != G_IO_STATUS_NORMAL)
    {
      g_task_return_error (task, error);
      g_object_unref (task);

      return;
    }

  g_assert (error == NULL);
  status = g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &error);

  if (status != G_IO_STATUS_NORMAL)
    {
      g_task_return_error (task, error);
      g_object_unref (task);

      return;
    }

  data->input = g_string_new (NULL);

  condition = G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP;
  data->io_watch_id = g_io_add_watch (channel, condition, read_cb, task);

  data->child_watch_id = g_child_watch_add (data->pid, child_watch_cb, task);
}

/**
 * ma_command_run_finish:
 * @command: a #MaCommand
 * @result: a #GAsyncResult
 * @error: (nullable): return location for an error, or %NULL
 *
 * Finishes an operation started with ma_command_run_async().
 *
 * Returns: %NULL if @error is set, otherwise output from command
 */
gchar *
ma_command_run_finish (MaCommand     *command,
                       GAsyncResult  *result,
                       GError       **error)
{
  g_return_val_if_fail (MA_IS_COMMAND (command), NULL);
  g_return_val_if_fail (g_task_is_valid (result, command), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}
