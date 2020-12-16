#ifndef LINUX_PROC_H__
#define LINUX_PROC_H__

#include <load-graph.h>

G_GNUC_INTERNAL void GetLoad     (guint64 Maximum, guint64 data [cpuload_n],  LoadGraph *g);
G_GNUC_INTERNAL void GetDiskLoad (guint64 Maximum, guint64 data [diskload_n], LoadGraph *g);
G_GNUC_INTERNAL void GetMemory   (guint64 Maximum, guint64 data [memload_n],  LoadGraph *g);
G_GNUC_INTERNAL void GetSwap     (guint64 Maximum, guint64 data [swapload_n], LoadGraph *g);
G_GNUC_INTERNAL void GetLoadAvg  (guint64 Maximum, guint64 data [2],          LoadGraph *g);
G_GNUC_INTERNAL void GetNet      (guint64 Maximum, guint64 data [4],          LoadGraph *g);

#endif
