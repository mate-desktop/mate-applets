#ifndef LINUX_PROC_H__
#define LINUX_PROC_H__

#include <load-graph.h>

G_GNUC_INTERNAL void GetLoad     (int Maximum, int data [cpuload_n],  LoadGraph *g);
G_GNUC_INTERNAL void GetDiskLoad (int Maximum, int data [diskload_n], LoadGraph *g);
G_GNUC_INTERNAL void GetMemory   (int Maximum, int data [memload_n],  LoadGraph *g);
G_GNUC_INTERNAL void GetSwap     (int Maximum, int data [swapload_n], LoadGraph *g);
G_GNUC_INTERNAL void GetLoadAvg  (int Maximum, int data [2],          LoadGraph *g);
G_GNUC_INTERNAL void GetNet      (int Maximum, int data [4],          LoadGraph *g);

#endif
