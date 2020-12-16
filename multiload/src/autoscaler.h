#ifndef MATE_APPLETS_MULTILOAD_AUTOSCALER_H
#define MATE_APPLETS_MULTILOAD_AUTOSCALER_H

#include <glib.h>

typedef struct _AutoScaler AutoScaler;

struct _AutoScaler
{
    gint64 update_interval;
    gint64 last_update;
    guint64 floor;
    guint64 max;
    guint64 count;
    guint64 sum;
    float last_average;
};

G_GNUC_INTERNAL void    autoscaler_init    (AutoScaler *that, gint64  interval, guint64 floor);
G_GNUC_INTERNAL guint64 autoscaler_get_max (AutoScaler *that, guint64 current);

#endif /* MATE_APPLETS_MULTILOAD_AUTOSCALER_H */
