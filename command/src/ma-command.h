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

#ifndef MA_COMMAND_H
#define MA_COMMAND_H

#include <gio/gio.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MA_TYPE_COMMAND (ma_command_get_type ())
G_DECLARE_FINAL_TYPE (MaCommand, ma_command, MA, COMMAND, GObject)

MaCommand *ma_command_new        (const gchar          *command,
                                  GError              **error);

void       ma_command_run_async  (MaCommand            *command,
                                  GCancellable         *cancellable,
                                  GAsyncReadyCallback   callback,
                                  gpointer              user_data);

gchar     *ma_command_run_finish (MaCommand            *command,
                                  GAsyncResult         *result,
                                  GError              **error);

G_END_DECLS

#endif
