/*  backend.c
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
 *
 *  Mate Netspeed Applet migrated by Stefano Karapetsas <stefano@karapetsas.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(sun) && defined(__SVR4)
#include <sys/sockio.h>
#endif

#include <glibtop/netlist.h>
#include <glibtop/netload.h>

#ifdef HAVE_IW
 #include <iwlib.h>
#endif /* HAVE_IW */

#include "backend.h"

gboolean
is_dummy_device(const char* device)
{
	glibtop_netload netload;
	glibtop_get_netload(&netload, device);

	if (netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK))
		return TRUE;

	/* Skip interfaces without any IPv4/IPv6 address (or
	   those with only a LINK ipv6 addr) However we need to
	   be able to exclude these while still keeping the
	   value so when they get online (with NetworkManager
	   for example) we don't get a suddent peak.  Once we're
	   able to get this, ignoring down interfaces will be
	   possible too.  */
	if (!(netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS6)
	      && netload.scope6 != GLIBTOP_IF_IN6_SCOPE_LINK)
	    && !(netload.flags & (1 << GLIBTOP_NETLOAD_ADDRESS)))
		return TRUE;

	return FALSE;
}


/* Check for all available devices. This really should be
 * portable for at least all plattforms using the gnu c lib
 * TODO: drop it, use glibtop_get_netlist directly / gchar**
 */
GList*
get_available_devices(void)
{
	glibtop_netlist buf;
	char **devices, **dev;
	GList *device_glist = NULL;

	devices = glibtop_get_netlist(&buf);

	for(dev = devices; *dev; ++dev) {
		device_glist = g_list_prepend(device_glist, g_strdup(*dev));
	}

	g_strfreev(devices);

	return device_glist;
}

const gchar*
get_default_route(void)
{
	FILE *fp;
	static char device[50];

	fp = fopen("/proc/net/route", "r");

	if (fp == NULL) return NULL;

	while (!feof(fp)) {
		char buffer[1024];
		unsigned int ip, gw, flags, ref, use, metric, mask, mtu, window, irtt;
		int retval;
		char *rv;

		rv = fgets(buffer, 1024, fp);
		if (!rv) {
			break;
		}

		retval = sscanf(buffer, "%49s %x %x %x %u %u %u %x %u %u %u",
				device, &ip, &gw, &flags, &ref, &use, &metric, &mask, &mtu, &window, &irtt);

		if (retval != 11) continue;

		if (ip == 0 && !is_dummy_device(device)) {
			fclose(fp);
			return device;
		}
	}
	fclose(fp);
	return NULL;
}


void
free_devices_list(GList *list)
{
	g_list_foreach(list, (GFunc)g_free, NULL);
	g_list_free(list);
}


/* Frees a DevInfo struct and all the stuff it contains
 */
void
free_device_info(DevInfo *devinfo)
{
	g_free(devinfo->name);
	g_free(devinfo->ip);
	g_free(devinfo->netmask);
	g_free(devinfo->ptpip);
	g_free(devinfo->hwaddr);
	g_free(devinfo->ipv6);
	g_free(devinfo->essid);
	g_free(devinfo->tx_rate);
	g_free(devinfo->rx_rate);
	g_free(devinfo->sum_rate);
}




static char*
format_ipv4(guint32 ip)
{
	char *str = g_malloc(INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ip, str, INET_ADDRSTRLEN);
	return str;
}


static char*
format_ipv6(const guint8 ip[16])
{
	char *str = g_malloc(INET6_ADDRSTRLEN);
	inet_ntop(AF_INET6, ip, str, INET6_ADDRSTRLEN);
	return str;
}


/* TODO:
   these stuff are not portable because of ioctl
*/
static void
get_ptp_info(DevInfo *devinfo)
{
	int fd = -1;
	struct ifreq request = {};

	g_strlcpy(request.ifr_name, devinfo->name, sizeof request.ifr_name);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	if (ioctl(fd, SIOCGIFDSTADDR, &request) >= 0) {
		struct sockaddr_in* addr;
		addr = (struct sockaddr_in*)&request.ifr_dstaddr;
		devinfo->ptpip = format_ipv4(addr->sin_addr.s_addr);
	}

	close(fd);
}




void
get_device_info(const char *device, DevInfo *devinfo)
{
	glibtop_netload netload;
	guint8 *hw;

	g_assert(device);

	memset(devinfo, 0, sizeof *devinfo);

	devinfo->name = g_strdup(device);
	devinfo->type = DEV_UNKNOWN;

	glibtop_get_netload(&netload, device);
	devinfo->tx = netload.bytes_out;
	devinfo->rx = netload.bytes_in;

	devinfo->up = (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_UP) ? TRUE : FALSE);
	devinfo->running = (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_RUNNING) ? TRUE : FALSE);

	devinfo->ip = format_ipv4(netload.address);
	devinfo->netmask = format_ipv4(netload.subnet);
	devinfo->ipv6 = format_ipv6(netload.address6);
	devinfo->qual = 0;
	devinfo->essid = NULL;

	hw = netload.hwaddress;
	if (hw[6] || hw[7]) {
		devinfo->hwaddr = g_strdup_printf(
			"%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
			hw[0], hw[1], hw[2], hw[3],
			hw[4], hw[5], hw[6], hw[7]);
	} else {
		devinfo->hwaddr = g_strdup_printf(
			"%02X:%02X:%02X:%02X:%02X:%02X",
			hw[0], hw[1], hw[2],
			hw[3], hw[4], hw[5]);
	}
	/* stolen from gnome-applets/multiload/linux-proc.c */

	if(netload.if_flags & (1L << GLIBTOP_IF_FLAGS_LOOPBACK)) {
		devinfo->type = DEV_LO;
	}

#ifdef HAVE_IW

	else if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_WIRELESS)) {
		devinfo->type = DEV_WIRELESS;
		get_wireless_info (devinfo);
	}

#endif /* HAVE_IW */

	else if(netload.if_flags & (1L << GLIBTOP_IF_FLAGS_POINTOPOINT)) {
		if (g_str_has_prefix(device, "plip")) {
			devinfo->type = DEV_PLIP;
		}
		else if (g_str_has_prefix(device, "sl")) {
			devinfo->type = DEV_SLIP;
		}
		else {
			devinfo->type = DEV_PPP;
		}

		get_ptp_info(devinfo);
	}
	else {
		devinfo->type = DEV_ETHERNET;
	}
}

gboolean
compare_device_info(const DevInfo *a, const DevInfo *b)
{
	g_assert(a && b);
	g_assert(a->name && b->name);

	if (!g_str_equal(a->name, b->name)) return TRUE;
	if (a->ip && b->ip) {
		if (!g_str_equal(a->ip, b->ip)) return TRUE;
	} else {
		if (a->ip || b->ip) return TRUE;
	}
	/* Ignore hwaddr, ptpip and netmask... I think this is ok */
	if (a->up != b->up) return TRUE;
	if (a->running != b->running) return TRUE;

	return FALSE;
}
#ifdef HAVE_IW
void
get_wireless_info (DevInfo *devinfo)
{
	int fd;
	int newqual;
	wireless_info info = {0};

	fd = iw_sockets_open ();

	if (fd < 0)
		return;

	if (iw_get_basic_config (fd, devinfo->name, &info.b) < 0)
		goto out;

	if (info.b.has_essid) {
		if ((!devinfo->essid) || (strcmp (devinfo->essid, info.b.essid) != 0)) {
			devinfo->essid = g_strdup (info.b.essid);
		}
	} else {
		devinfo->essid = NULL;
	}

	if (iw_get_stats (fd, devinfo->name, &info.stats, &info.range, info.has_range) >= 0)
		info.has_stats = 1;

	if (info.has_stats) {
		if ((iw_get_range_info(fd, devinfo->name, &info.range) >= 0) && (info.range.max_qual.qual > 0)) {
			newqual = 0.5f + (100.0f * info.stats.qual.qual) / (1.0f * info.range.max_qual.qual);
		} else {
			newqual = info.stats.qual.qual;
		}

		newqual = CLAMP(newqual, 0, 100);
		if (devinfo->qual != newqual)
			devinfo->qual = newqual;

	} else {
		devinfo->qual = 0;
	}

	goto out;
out:
	if (fd != -1)
		close (fd);
}
#endif /* HAVE_IW */
