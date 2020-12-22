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

#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glibtop/netlist.h>
#include <glibtop/netload.h>

#include "backend.h"

#ifdef HAVE_IW
#include <iwlib.h>
#endif /* HAVE_IW */

#ifdef HAVE_NL

#include <errno.h>
#include <linux/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>
#include <netlink/msg.h>

#include "nl80211.h"
#include "ieee80211.h"

struct nl80211_state {
    struct nl_sock *sock;
    int nl80211_id;
};

#endif /* HAVE_NL */

gboolean
is_dummy_device (const char* device)
{
    glibtop_netload netload;
    glibtop_get_netload (&netload, device);

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
get_available_devices (void)
{
    glibtop_netlist buf;
    char **devices, **dev;
    GList *device_glist = NULL;

    devices = glibtop_get_netlist (&buf);

    for (dev = devices; *dev; ++dev) {
        device_glist = g_list_prepend (device_glist, g_strdup (*dev));
    }

    g_strfreev (devices);

    return device_glist;
}

GSList*
get_ip_address_list (const char *iface_name,
                     gboolean    ipv4)
{
    GSList *list = NULL;
    struct ifaddrs *ifaces;

    if (getifaddrs (&ifaces) != -1) {
        int family;
        char ip[INET6_ADDRSTRLEN];

        family = ipv4 ? AF_INET : AF_INET6;
        for (struct ifaddrs *iface = ifaces; iface != NULL; iface = iface->ifa_next) {
            if (iface->ifa_addr == NULL)
                continue;

            if ((iface->ifa_addr->sa_family == family) &&
                !g_strcmp0 (iface->ifa_name, iface_name))
            {
                unsigned int netmask = 0;
                void *sinx_addr = NULL;

                if (iface->ifa_addr->sa_family == AF_INET6) {
                    struct sockaddr_in6 ip6_addr;
                    struct sockaddr_in6 ip6_network;
                    uint32_t ip6_netmask = 0;
                    const char *scope;
                    int i;

                    memcpy (&ip6_addr, iface->ifa_addr, sizeof (struct sockaddr_in6));
                    memcpy (&ip6_network, iface->ifa_netmask, sizeof (struct sockaddr_in6));

                    /* get network scope */
                    if (IN6_IS_ADDR_LINKLOCAL (&ip6_addr.sin6_addr)) {
                        scope = _("link-local");
                    } else if (IN6_IS_ADDR_SITELOCAL (&ip6_addr.sin6_addr)) {
                        scope = _("site-local");
                    } else if (IN6_IS_ADDR_V4MAPPED (&ip6_addr.sin6_addr)) {
                        scope = _("v4mapped");
                    } else if (IN6_IS_ADDR_V4COMPAT (&ip6_addr.sin6_addr)) {
                        scope = _("v4compat");
                    } else if (IN6_IS_ADDR_LOOPBACK (&ip6_addr.sin6_addr)) {
                        scope = _("host");
                    } else if (IN6_IS_ADDR_UNSPECIFIED (&ip6_addr.sin6_addr)) {
                        scope = _("unspecified");
                    } else {
                        scope = _("global");
                    }

                    /* get network ip */
                    sinx_addr = &ip6_addr.sin6_addr;
                    inet_ntop (iface->ifa_addr->sa_family, sinx_addr, ip, sizeof (ip));

                    /* get network mask length */
                    for (i = 0; i < 4; i++) {
                        ip6_netmask = ntohl (((uint32_t*)(&ip6_network.sin6_addr))[i]);
                        while (ip6_netmask) {
                            netmask++;
                            ip6_netmask <<= 1;
                        }
                    }

                    list = g_slist_prepend (list,
                                            g_strdup_printf ("%s/%u (%s)",
                                                             ip, netmask, scope));
                } else {
                    struct sockaddr_in ip4_addr;
                    struct sockaddr_in ip4_network;
                    in_addr_t ip4_netmask = 0;

                    memcpy (&ip4_addr, iface->ifa_addr, sizeof (struct sockaddr_in));
                    memcpy (&ip4_network, iface->ifa_netmask, sizeof (struct sockaddr_in));

                    /* get network ip */
                    sinx_addr = &ip4_addr.sin_addr;
                    inet_ntop (iface->ifa_addr->sa_family, sinx_addr, ip, sizeof (ip));

                    /* get network mask length */
                    ip4_netmask = ntohl (ip4_network.sin_addr.s_addr);
                    while (ip4_netmask) {
                        netmask++;
                        ip4_netmask <<= 1;
                    }

                    list = g_slist_prepend (list,
                                            g_strdup_printf ("%s/%u", ip, netmask));
                }
            }
        }
        if (list != NULL)
            list = g_slist_sort (list, (GCompareFunc) g_strcmp0);
        freeifaddrs (ifaces);
    }
    return list;
}

const gchar*
get_default_route (void)
{
    FILE *fp;
    static char device[50];

    fp = fopen ("/proc/net/route", "r");

    if (fp == NULL) return NULL;

    while (!feof (fp)) {
        char buffer[1024];
        unsigned int ip, gw, flags, ref, use, metric, mask, mtu, window, irtt;
        int retval;
        char *rv;

        rv = fgets (buffer, 1024, fp);
        if (!rv) {
            break;
        }

        retval = sscanf (buffer, "%49s %x %x %x %u %u %u %x %u %u %u",
                         device, &ip, &gw, &flags, &ref, &use, &metric,
                         &mask, &mtu, &window, &irtt);

        if (retval != 11) continue;

        if (ip == 0 && !is_dummy_device (device)) {
            fclose (fp);
            return device;
        }
    }
    fclose (fp);
    return NULL;
}

void
free_devices_list (GList *list)
{
    g_list_free_full (list, g_free);
}

/* Frees a DevInfo struct and all the stuff it contains
 */
void
free_device_info (DevInfo *devinfo)
{
    g_free (devinfo->name);
    g_free (devinfo->essid);
#ifdef HAVE_NL
    g_free (devinfo->tx_bitrate);
    g_free (devinfo->rx_bitrate);
    g_free (devinfo->channel);
#endif /* HAVE_NL */
    g_free (devinfo);
}

/* TODO:
   these stuff are not portable because of ioctl
*/
static void
get_ptp_info (DevInfo *devinfo)
{
    int fd = -1;
    struct ifreq request = {};

    g_strlcpy (request.ifr_name, devinfo->name, sizeof request.ifr_name);

    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        return;

    if (ioctl (fd, SIOCGIFDSTADDR, &request) >= 0) {
        struct sockaddr_in* addr;
        addr = (struct sockaddr_in*)&request.ifr_dstaddr;
        devinfo->ptpip = addr->sin_addr.s_addr;
    }

    close (fd);
}

void
get_device_info (const char  *device,
                 DevInfo    **info)
{
    DevInfo *devinfo;
    glibtop_netload netload;
    gboolean ptp = FALSE;

    g_assert (device);

    *info = g_new0 (DevInfo, 1);
    devinfo = *info;

    devinfo->name = g_strdup (device);
    devinfo->type = DEV_UNKNOWN;

    glibtop_get_netload (&netload, device);

    devinfo->up = (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_UP) ? TRUE : FALSE);
    devinfo->running = (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_RUNNING) ? TRUE : FALSE);

    if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_LOOPBACK)) {
        devinfo->type = DEV_LO;
    }
    else if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_WIRELESS)) {
        devinfo->type = DEV_WIRELESS;
    }
    else if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_POINTOPOINT)) {
        if (g_str_has_prefix (device, "plip")) {
            devinfo->type = DEV_PLIP;
        }
        else if (g_str_has_prefix (device, "sl")) {
            devinfo->type = DEV_SLIP;
        }
        else {
            devinfo->type = DEV_PPP;
        }
        ptp = TRUE;
    }
    else {
        devinfo->type = DEV_ETHERNET;
    }

    switch (devinfo->type) {
#if defined (HAVE_NL)
        case DEV_WIRELESS:
            get_wireless_info (devinfo);
            break;
#endif /* HAVE_NL */
        case DEV_LO:
            break;
#if defined (HAVE_IW)
        case DEV_WIRELESS:
            get_wireless_info (devinfo);
#endif /* HAVE_IW */
        default:
            memcpy (devinfo->hwaddr, netload.hwaddress, 8);
            break;
    }

    if (devinfo->running) {
        devinfo->ip = netload.address;
        devinfo->netmask = netload.subnet;
        memcpy (devinfo->ipv6, netload.address6, 16);
#if defined (HAVE_NL)
        if (devinfo->type != DEV_WIRELESS) {
            devinfo->tx = netload.bytes_out;
            devinfo->rx = netload.bytes_in;
            if (ptp)
                get_ptp_info (devinfo);
        }
#else
        devinfo->tx = netload.bytes_out;
        devinfo->rx = netload.bytes_in;
        if (ptp)
            get_ptp_info (devinfo);
#endif /* HAVE_NL */
    }
}

gboolean
compare_device_info (const DevInfo *a,
                     const DevInfo *b)
{
    g_assert (a && b);
    g_assert (a->name && b->name);

    if (!g_str_equal (a->name, b->name))
        return TRUE;
    if (a->ip != b->ip)
        return TRUE;
    /* Ignore hwaddr, ptpip and netmask... I think this is ok */
    if (a->up != b->up)
        return TRUE;
    if (a->running != b->running)
        return TRUE;

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
        if ((iw_get_range_info (fd, devinfo->name, &info.range) >= 0) && (info.range.max_qual.qual > 0)) {
            newqual = 0.5f + (100.0f * info.stats.qual.qual) / (1.0f * info.range.max_qual.qual);
        } else {
            newqual = info.stats.qual.qual;
        }

        newqual = CLAMP (newqual, 0, 100);
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

#ifdef HAVE_NL
int iw_debug = 0;

static int
nl80211_init (struct nl80211_state *state)
{
    int err;

    state->sock = nl_socket_alloc ();
    if (!state->sock) {
        g_warning ("Failed to allocate netlink socket");
        return -ENOMEM;
    }

    if (genl_connect (state->sock)) {
        g_warning ("Failed to connect to generic netlink");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    nl_socket_set_buffer_size (state->sock, 8192, 8192);

    /* try to set NETLINK_EXT_ACK to 1, ignoring errors */
    err = 1;
    setsockopt (nl_socket_get_fd (state->sock), SOL_NETLINK,
                NETLINK_EXT_ACK, &err, sizeof (err));

    state->nl80211_id = genl_ctrl_resolve (state->sock, "nl80211");
    if (state->nl80211_id < 0) {
        g_warning ("nl80211 not found");
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

out_handle_destroy:
    nl_socket_free (state->sock);
    return err;
}


static void
nl80211_cleanup (struct nl80211_state *state)
{
    nl_socket_free (state->sock);
}

static int
scan_cb (struct nl_msg  *msg,
         void           *arg)
{
    DevInfo *devinfo = (DevInfo*) arg;
    struct genlmsghdr *gnlh = nlmsg_data (nlmsg_hdr (msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *bss[NL80211_BSS_MAX + 1];
    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_TSF] = { .type = NLA_U64 },
        [NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
        [NL80211_BSS_BSSID] = { },
        [NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
        [NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
        [NL80211_BSS_INFORMATION_ELEMENTS] = { },
        [NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
        [NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
        [NL80211_BSS_STATUS] = { .type = NLA_U32 },
        [NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
        [NL80211_BSS_BEACON_IES] = { },
    };

    /* Parse and error check. */
    nla_parse (tb, NL80211_ATTR_MAX, genlmsg_attrdata (gnlh, 0), genlmsg_attrlen (gnlh, 0), NULL);
    if (!tb[NL80211_ATTR_BSS]) {
        g_warning ("bss info missing!");
        return NL_SKIP;
    }
    if (nla_parse_nested (bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
        g_warning ("failed to parse nested attributes!");
        return NL_SKIP;
    }
    if (!bss[NL80211_BSS_BSSID])
        return NL_SKIP;
    if (!bss[NL80211_BSS_STATUS])
        return NL_SKIP;

    if (nla_get_u32 (bss[NL80211_BSS_STATUS]) != NL80211_BSS_STATUS_ASSOCIATED)
        return NL_SKIP;

    memcpy (devinfo->station_mac_addr, nla_data (bss[NL80211_BSS_BSSID]), ETH_ALEN);

    return NL_SKIP;
}

static void
parse_bitrate (struct nlattr *bitrate_attr,
               char          *buf,
               int            buflen)
{
    int rate = 0;
    char *pos = buf;
    struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
    static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
        [NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
        [NL80211_RATE_INFO_BITRATE32] = { .type = NLA_U32 },
        [NL80211_RATE_INFO_MCS] = { .type = NLA_U8 },
        [NL80211_RATE_INFO_40_MHZ_WIDTH] = { .type = NLA_FLAG },
        [NL80211_RATE_INFO_SHORT_GI] = { .type = NLA_FLAG },
    };

    if (nla_parse_nested (rinfo, NL80211_RATE_INFO_MAX,
                          bitrate_attr, rate_policy)) {
        g_warning ("failed to parse nested rate attributes!");
        return;
    }

    if (rinfo[NL80211_RATE_INFO_BITRATE32])
        rate = nla_get_u32 (rinfo[NL80211_RATE_INFO_BITRATE32]);
    else if (rinfo[NL80211_RATE_INFO_BITRATE])
        rate = nla_get_u16 (rinfo[NL80211_RATE_INFO_BITRATE]);
    if (rate > 0)
        pos += snprintf (pos, buflen - (pos - buf),
                         _("%d.%d MBit/s"), rate / 10, rate % 10);
    else
        pos += snprintf (pos, buflen - (pos - buf), _("(unknown)"));

    if (rinfo[NL80211_RATE_INFO_MCS])
        pos += snprintf (pos, buflen - (pos - buf),
                         _(" MCS %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_MCS]));
    if (rinfo[NL80211_RATE_INFO_VHT_MCS])
        pos += snprintf (pos, buflen - (pos - buf),
                         _(" VHT-MCS %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_VHT_MCS]));
    if (rinfo[NL80211_RATE_INFO_40_MHZ_WIDTH])
        pos += snprintf (pos, buflen - (pos - buf), _(" 40MHz"));
    if (rinfo[NL80211_RATE_INFO_80_MHZ_WIDTH])
        pos += snprintf (pos, buflen - (pos - buf), _(" 80MHz"));
    if (rinfo[NL80211_RATE_INFO_80P80_MHZ_WIDTH])
        pos += snprintf (pos, buflen - (pos - buf), _(" 80P80MHz"));
    if (rinfo[NL80211_RATE_INFO_160_MHZ_WIDTH])
        pos += snprintf (pos, buflen - (pos - buf), _(" 160MHz"));
    if (rinfo[NL80211_RATE_INFO_SHORT_GI])
        pos += snprintf (pos, buflen - (pos - buf), _(" short GI)"));
    if (rinfo[NL80211_RATE_INFO_VHT_NSS])
        pos += snprintf (pos, buflen - (pos - buf),
                         _(" VHT-NSS %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_VHT_NSS]));
    if (rinfo[NL80211_RATE_INFO_HE_MCS])
        pos += snprintf (pos, buflen - (pos - buf),
                         _(" HE-MCS %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_HE_MCS]));
    if (rinfo[NL80211_RATE_INFO_HE_NSS])
        pos += snprintf (pos, buflen - (pos - buf),
                        _(" HE-NSS %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_HE_NSS]));
    if (rinfo[NL80211_RATE_INFO_HE_GI])
        pos += snprintf (pos, buflen - (pos - buf),
                        _(" HE-GI %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_HE_GI]));
    if (rinfo[NL80211_RATE_INFO_HE_DCM])
        snprintf (pos, buflen - (pos - buf),
                  _(" HE-DCM %d"), nla_get_u8 (rinfo[NL80211_RATE_INFO_HE_DCM]));
}

static int
station_cb (struct nl_msg *msg,
            void          *arg)
{
    DevInfo *devinfo = (DevInfo*) arg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data (nlmsg_hdr (msg));
    struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
    static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
        [NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
        [NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
        [NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
        [NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
        [NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
        [NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
        [NL80211_STA_INFO_RX_BITRATE] = { .type = NLA_NESTED },
        [NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
        [NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
        [NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
        [NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
    };

    nla_parse (tb, NL80211_ATTR_MAX, genlmsg_attrdata (gnlh, 0),
               genlmsg_attrlen (gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_STA_INFO]) {
        g_warning ("sta stats missing!");
        return NL_SKIP;
    }
    if (nla_parse_nested (sinfo, NL80211_STA_INFO_MAX,
                          tb[NL80211_ATTR_STA_INFO],
                          stats_policy)) {
        g_warning ("failed to parse nested attributes!\n");
        return NL_SKIP;
    }

    if (sinfo[NL80211_STA_INFO_RX_BYTES] && sinfo[NL80211_STA_INFO_RX_PACKETS]) {
        guint32 rx_bytes = nla_get_u32 (sinfo[NL80211_STA_INFO_RX_BYTES]);
        devinfo->rx = (guint64) rx_bytes;
        g_debug ("RX: %" G_GUINT32_FORMAT " bytes (%" G_GUINT32_FORMAT " packets)",
                 rx_bytes,
                 nla_get_u32 (sinfo[NL80211_STA_INFO_RX_PACKETS]));
    }
    if (sinfo[NL80211_STA_INFO_TX_BYTES] && sinfo[NL80211_STA_INFO_TX_PACKETS]) {
        guint32 tx_bytes = nla_get_u32 (sinfo[NL80211_STA_INFO_TX_BYTES]);
        devinfo->tx = (guint64) tx_bytes;
        g_debug ("TX: %" G_GUINT32_FORMAT " bytes (%" G_GUINT32_FORMAT " packets)",
                 tx_bytes,
                 nla_get_u32 (sinfo[NL80211_STA_INFO_TX_PACKETS]));
    }
    if (sinfo[NL80211_STA_INFO_SIGNAL]) {
        int8_t dBm = (int8_t)nla_get_u8 (sinfo[NL80211_STA_INFO_SIGNAL]);
        g_debug ("signal: %d dBm", dBm);
        devinfo->rssi = dBm;
        devinfo->qual =  CLAMP (2 * ((int)dBm + 100), 1, 100);
    }
    if (sinfo[NL80211_STA_INFO_RX_BITRATE]) {
        char buf[100];
        parse_bitrate (sinfo[NL80211_STA_INFO_RX_BITRATE], buf, sizeof (buf));
        g_debug ("rx bitrate: %s", buf);
        devinfo->rx_bitrate = g_strdup (buf);
    }
    if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
        char buf[100];
        parse_bitrate (sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof (buf));
        g_debug ("tx bitrate: %s", buf);
        devinfo->tx_bitrate = g_strdup (buf);
    }
    if (sinfo[NL80211_STA_INFO_CONNECTED_TIME]) {
        devinfo->connected_time = nla_get_u32 (sinfo[NL80211_STA_INFO_CONNECTED_TIME]);
        g_debug ("connected time: %" G_GUINT32_FORMAT " seconds", devinfo->connected_time);
    }

    return NL_SKIP;
}

static char *
channel_width_name (enum nl80211_chan_width width)
{
    switch (width) {
        case NL80211_CHAN_WIDTH_20_NOHT:
            return _("20 MHz (no HT)");
        case NL80211_CHAN_WIDTH_20:
            return _("20 MHz");
        case NL80211_CHAN_WIDTH_40:
            return _("40 MHz");
        case NL80211_CHAN_WIDTH_80:
            return _("80 MHz");
        case NL80211_CHAN_WIDTH_80P80:
            return _("80+80 MHz");
        case NL80211_CHAN_WIDTH_160:
            return _("160 MHz");
        case NL80211_CHAN_WIDTH_5:
            return _("5 MHz");
        case NL80211_CHAN_WIDTH_10:
            return _("10 MHz");
        default:
            return _("unknown");
    }
}

static int
ieee80211_frequency_to_channel (int freq)
{
    /* see 802.11-2007 17.3.8.3.2 and Annex J */
    if (freq == 2484)
        return 14;
    else if (freq < 2484)
        return (freq - 2407) / 5;
    else if (freq >= 4910 && freq <= 4980)
        return (freq - 4000) / 5;
    else if (freq <= 45000) /* DMG band lower limit */
        return (freq - 5000) / 5;
    else if (freq >= 58320 && freq <= 64800)
        return (freq - 56160) / 2160;
    else
        return 0;
}

static int
iface_cb (struct nl_msg *msg,
          void          *arg)
{
    DevInfo *devinfo = (DevInfo*) arg;
    struct genlmsghdr *gnlh = nlmsg_data (nlmsg_hdr (msg));
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

    nla_parse (tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata (gnlh, 0),
               genlmsg_attrlen (gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_MAC]) {
        memcpy (devinfo->hwaddr, nla_data (tb_msg[NL80211_ATTR_MAC]), ETH_ALEN);
    }

    if (tb_msg[NL80211_ATTR_SSID]) {
        char buf[G_MAXUINT8];
        int len = nla_len (tb_msg[NL80211_ATTR_SSID]);
        memcpy (buf, nla_data (tb_msg[NL80211_ATTR_SSID]), len);
        buf [len] = '\0';
        devinfo->essid = g_strescape (buf, NULL);
        g_debug ("ssid: %s", buf);
    }

    if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
        uint32_t freq = nla_get_u32 (tb_msg[NL80211_ATTR_WIPHY_FREQ]);
        char buf[100];
        char *pos = buf;

        pos += sprintf (pos, _("%d (%d MHz)"),
                        ieee80211_frequency_to_channel (freq), freq);

        if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH])
            sprintf (pos, _(", width: %s"),
                     channel_width_name (nla_get_u32 (tb_msg[NL80211_ATTR_CHANNEL_WIDTH])));

        devinfo->channel = g_strdup (buf);
    }

    return NL_SKIP;
}

void
get_wireless_info (DevInfo *devinfo)
{
    struct nl80211_state nlstate = {.sock = NULL, .nl80211_id = -1};
    int err, ret;
    struct nl_msg *msg;
    signed long long devidx = 0;

    err = nl80211_init (&nlstate);
    if (err || (nlstate.nl80211_id < 0)) {
        g_warning ("failed to init netlink");
        return;
    }

    devidx = if_nametoindex (devinfo->name);

    /* Get MAC, SSID & channel info from interface message */
    msg = nlmsg_alloc ();
    if (!msg) {
        g_warning ("failed to allocate netlink message");
        goto cleanup;
    }
    genlmsg_put (msg, 0, 0, nlstate.nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);
    /* Add message attribute, which interface to use */
    nla_put_u32 (msg, NL80211_ATTR_IFINDEX, devidx);
    /* Add the callback */
    nl_socket_modify_cb (nlstate.sock, NL_CB_VALID, NL_CB_CUSTOM, iface_cb, devinfo);
    /* Send the message */
    ret = nl_send_auto (nlstate.sock, msg);
    g_debug ("NL80211_CMD_GET_INTERFACE sent %d bytes to the kernel", ret);
    /* Retrieve the kernel's answer */
    ret = nl_recvmsgs_default (nlstate.sock);
    nlmsg_free (msg);
    if (ret < 0) {
        g_warning ("failed to recive netlink message");
    }

    if (!devinfo->running)
        goto cleanup;

    /* Get station MAC from scan message */
    msg = nlmsg_alloc ();
    if (!msg) {
        g_warning ("failed to allocate netlink message");
        goto cleanup;
    }
    genlmsg_put (msg, 0, 0, nlstate.nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
    /* Add message attribute, which interface to use */
    nla_put_u32 (msg, NL80211_ATTR_IFINDEX, devidx);
    /* Add the callback */
    nl_socket_modify_cb (nlstate.sock, NL_CB_VALID, NL_CB_CUSTOM, scan_cb, devinfo);
    /* Send the message */
    ret = nl_send_auto (nlstate.sock, msg);
    g_debug ("NL80211_CMD_GET_SCAN sent %d bytes to the kernel", ret);
    /* Retrieve the kernel's answer */
    ret = nl_recvmsgs_default (nlstate.sock);
    nlmsg_free (msg);
    if (ret < 0) {
        g_warning ("failed to recive netlink message");
        goto cleanup;
    }

    if (!devinfo->running)
        goto cleanup;

    /* Get in/out bitrate/rate/total, signal quality from station message */
    msg = nlmsg_alloc ();
    if (!msg) {
        g_warning ("failed to allocate netlink message");
        goto cleanup;
    }
    genlmsg_put (msg, 0, 0, nlstate.nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
    /* Add message attribute, which interface to use */
    nla_put (msg, NL80211_ATTR_MAC, ETH_ALEN, devinfo->station_mac_addr);
    nla_put_u32 (msg, NL80211_ATTR_IFINDEX, devidx);
    /* Add the callback */
    nl_socket_modify_cb (nlstate.sock, NL_CB_VALID, NL_CB_CUSTOM, station_cb, devinfo);
    /* Send the message */
    ret = nl_send_auto (nlstate.sock, msg);
    g_debug ("NL80211_CMD_GET_STATION sent %d bytes to the kernel", ret);
    /* Retrieve the kernel's answer */
    ret = nl_recvmsgs_default (nlstate.sock);
    nlmsg_free (msg);
    if (ret < 0) {
        g_warning ("failed to recive netlink message");
    }

 cleanup:
    nl80211_cleanup (&nlstate);
}
#endif /* HAVE_NL */
