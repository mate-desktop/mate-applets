/*  backend.h
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Netspeed Applet was writen by Jörgen Scheibengruber <mfcn@gmx.de>
 */

#ifndef _BACKEND_H
#define _BACKEND_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <glib.h>
#include <glibtop/netload.h>

/* copied from <linux/wireless.h> */
#define SIOCGIWNAME     0x8B01          /* get name == wireless protocol */
#define SIOCGIWENCODE	0x8B2B		    /* get encoding token & mode */

/* Different types of interfaces */
typedef enum
{
	DEV_LO,
	DEV_ETHERNET,
	DEV_WIRELESS,
	DEV_PPP,
	DEV_PLIP,
	DEV_SLIP,
	DEV_UNKNOWN	// this has to be the last one
} DevType;	

/* Some information about the selected network device
 */
typedef struct
{
	DevType type;
	char *name;
	char *ip;
	char *netmask; 
	char *hwaddr;
	char *ptpip;
	char *ipv6;
	char *essid;
	gboolean up, running;
	guint64 tx, rx;
	int qual;
	char *tx_rate;
	char *rx_rate;
	char *sum_rate;
} DevInfo;

GList*
get_available_devices(void);

const gchar*
get_default_route(void);

gboolean
is_dummy_device(const char* device);

void
free_devices_list(GList *list);

void
free_device_info(DevInfo *devinfo);

void
get_device_info(const char *device, DevInfo *info);

gboolean
compare_device_info(const DevInfo *a, const DevInfo *b);

void 
get_wireless_info (DevInfo *devinfo);
#endif /* _BACKEND_H */
