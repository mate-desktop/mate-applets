/*
 *  Copyright (C) 2010 by Joachim Breitner <mail@joachim-breitner.de>
 *
 * Based on battstat-hal.c:
 * Copyright (C) 2005 by Ryan Lortie <desrt@desrt.ca>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 * $Id$
 */

#include <config.h>

#ifdef HAVE_UPOWER

#include <upower.h>
#include <math.h>

#include "battstat-upower.h"

static UpClient *upc;
static void (*status_updated_callback) (void);


/* status_updated_callback() can not be called directly because at the time of
 * the device-remove signal, the device is not actually removed from the list
 * of devices known to the up_client object (see libupower-glib/up-client.c in
 * upower). Waiting for the next idle timer works around this issue and has has
 * the additionaly benefit of possibly running status_updated_callback only
 * once when several events happen very soon after each other.
 */
static gboolean status_update_scheduled;

static gboolean
update_status_idle (gpointer junk)
{
  if (status_updated_callback)
    status_updated_callback ();

  return status_update_scheduled = FALSE;
}

static void
schedule_status_callback (void)
{
  if (status_update_scheduled)
    return;

  status_update_scheduled = TRUE;
  g_idle_add (update_status_idle, NULL);
}

static void
device_cb (UpClient *client, UpDevice *device, gpointer user_data) {
  schedule_status_callback();
}

#if UP_CHECK_VERSION (0, 99, 0)
static void
device_removed_cb (UpClient *client, const gchar *object_path, gpointer user_data) {
  schedule_status_callback();
}
#endif

/* ---- public functions ---- */

char *
battstat_upower_initialise (void (*callback) (void))
{
  char *error_str;
  int i, num;

  status_updated_callback = callback;
#if UP_CHECK_VERSION (0, 99, 0)
  GPtrArray *devices;
#endif

  if( upc != NULL )
    return g_strdup( "Already initialised!" );

  if( (upc = up_client_new() ) == NULL )
    goto error_out;

  GCancellable *cancellable = g_cancellable_new();
  GError *gerror;

#if UP_CHECK_VERSION(0, 99, 0)
  devices = up_client_get_devices(upc);
  if (!devices) {
    goto error_shutdownclient;
  }
  g_ptr_array_unref(devices);
#else
  if (! up_client_enumerate_devices_sync( upc, cancellable, &gerror ) ) {
    sprintf(error_str, "Unable to enumerate upower devices: %s\n", gerror->message);
    goto error_shutdownclient;
  }
#endif

  g_signal_connect_after( upc, "device-added", G_CALLBACK (device_cb), NULL );
#if UP_CHECK_VERSION(0, 99, 0)
  g_signal_connect_after( upc, "device-removed", G_CALLBACK (device_removed_cb), NULL );
#else
  g_signal_connect_after( upc, "device-changed", G_CALLBACK (device_cb), NULL );
  g_signal_connect_after( upc, "device-removed", G_CALLBACK (device_cb), NULL );
#endif

  return NULL;

error_shutdownclient:
  g_object_unref( upc );
  upc = NULL;

error_out:
  return "Can not initialize upower";
}

void
battstat_upower_cleanup( void )
{
  if( upc == NULL )
    return;
  
  g_object_unref( upc );
  upc = NULL;
}

#include "battstat.h"

/* This function currently exists to allow the multiple batteries supported
 * by the upower backend to appear as a single composite battery device (since
 * at the current time this is all that battstat supports).
 *
 * This entire function is filled with logic to make multiple batteries
 * appear as one "composite" battery.  Comments included as appropriate.
 *
 * For more information about some of the assumptions made in the following
 * code please see the following mailing list post and the resulting thread:
 *
 *   http://lists.freedesktop.org/archives/hal/2005-July/002841.html
 */
void
battstat_upower_get_battery_info( BatteryStatus *status )
{

  GPtrArray *devices = up_client_get_devices( upc );

  /* The calculation to get overall percentage power remaining is as follows:
   *
   *    Sum( Current charges ) / Sum( Full Capacities )
   *
   * We can't just take an average of all of the percentages since this
   * doesn't deal with the case that one battery might have a larger
   * capacity than the other.
   *
   * In order to do this calculation, we need to keep a running total of
   * current charge and full capacities.
   */
  double current_charge_total = 0, full_capacity_total = 0;

  /* Record the time remaining as reported by upower.  This is used in the event
   * that the system has exactly one battery (since, then, upower is capable
   * of providing an accurate time remaining report and we should trust it.)
   */
  gint64 remaining_time = 0;

  /* The total (dis)charge rate of the system is the sum of the rates of
   * the individual batteries.
   */
  double rate_total = 0;

  /* We need to know if we should report the composite battery as present
   * at all.  The logic is that if at least one actual battery is installed
   * then the composite battery will be reported to exist.
   */
  int present = 0;

  /* We need to know if we are on AC power or not.  Eventually, we can look
   * at the AC adaptor upower devices to determine that.  For now, we assume that
   * if any battery is discharging then we must not be on AC power.  Else, by
   * default, we must be on AC.
   */
  int on_ac_power = 1;

  /* Finally, we consider the composite battery to be "charging" if at least
   * one of the actual batteries in the system is charging.
   */
  int charging = 0;

  /* A list iterator. */
  GSList *item;

  /* For each physical battery bay... */
  int i;
  for( i = 0; i < devices->len; i++ )
  {
    UpDevice *upd = g_ptr_array_index( devices, i );

    int type, state;
    double current_charge, full_capacity, rate;
    gint64 time_to_full, time_to_empty;
    
    g_object_get( upd,
      "kind", &type,
      "state", &state,
      "energy", &current_charge,
      "energy-full", &full_capacity,
      "energy-rate", &rate,
      "time-to-full", &time_to_full,
      "time-to-empty", &time_to_empty,
      NULL );

    /* Only count batteries here */

    if (type != UP_DEVICE_KIND_BATTERY)
      continue;

    /* At least one battery present -> composite battery is present. */
    present++;

    /* At least one battery charging -> composite battery is charging. */
    if( state == UP_DEVICE_STATE_CHARGING )
      charging = 1;

    /* At least one battery is discharging -> we're not on AC. */
    if( state == UP_DEVICE_STATE_DISCHARGING )
      on_ac_power = 0;

    /* Sum the totals for current charge, design capacity, (dis)charge rate. */
    current_charge_total += current_charge;
    full_capacity_total += full_capacity;
    rate_total += rate;

    /* Record remaining time too, incase this is the only battery. */
    remaining_time = (state == UP_DEVICE_STATE_DISCHARGING ? time_to_empty : time_to_full);
  }

  if( !present || full_capacity_total <= 0 || (charging && !on_ac_power) )
  {
    /* Either no battery is present or something has gone horribly wrong.
     * In either case we must return that the composite battery is not
     * present.
     */
    status->present = FALSE;
    status->percent = 0;
    status->minutes = -1;
    status->on_ac_power = TRUE;
    status->charging = FALSE;

    g_ptr_array_unref( devices );
    return;
  }

  /* Else, our composite battery is present. */
  status->present = TRUE;

  /* As per above, overall charge is:
   *
   *    Sum( Current charges ) / Sum( Full Capacities )
   */
  status->percent = ( current_charge_total / full_capacity_total ) * 100.0 + 0.5;

  if( present == 1 )
  {
    /* In the case of exactly one battery, report the time remaining figure
     * from upower directly since it might have come from an authorative source
     * (ie: the PMU or APM subsystem).
     *
     * upower gives remaining time in seconds with a 0 to mean that the
     * remaining time is unknown.  Battstat uses minutes and -1 for 
     * unknown time remaining.
     */

    if( remaining_time == 0 )
      status->minutes = -1;
    else
      status->minutes = (remaining_time + 30) / 60;
  }
  /* Rest of cases to deal with multiple battery systems... */
  else if( !on_ac_power && rate_total != 0 )
  {
    /* Then we're discharging.  Calculate time remaining until at zero. */

    double remaining;

    remaining = current_charge_total;
    remaining /= rate_total;
    status->minutes = (int) floor( remaining * 60.0 + 0.5 );
  }
  else if( charging && rate_total != 0 )
  {
    /* Calculate time remaining until charged.  For systems with more than
     * one battery, this code is very approximate.  The assumption is that if
     * one battery reaches full charge before the other that the other will
     * start charging faster due to the increase in available power (similar
     * to how a laptop will charge faster if you're not using it).
     */

    double remaining;

    remaining = full_capacity_total - current_charge_total;
    if( remaining < 0 )
      remaining = 0;
    remaining /= rate_total;

    status->minutes = (int) floor( remaining * 60.0 + 0.5 );
  }
  else
  {
    /* On AC power and not charging -or- rate is unknown. */
    status->minutes = -1;
  }

  /* These are simple and well-explained above. */
  status->charging = charging;
  status->on_ac_power = on_ac_power;
  
  g_ptr_array_unref( devices );
}

void
error_dialog( const char *fmt , ...)
{
  va_list ap;
  va_start(ap, fmt);
  char str[1000];
  vsprintf(str, fmt, ap);
  va_end(ap);
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new( NULL, 0, GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK, "%s", str);

  g_signal_connect_swapped( G_OBJECT (dialog), "response",
                            G_CALLBACK (gtk_widget_destroy),
                            G_OBJECT (dialog) );

  gtk_widget_show_all( dialog );
}

#endif /* HAVE_UPOWER */
