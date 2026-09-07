# Hardware Monitor applet

Hardware Monitor is a native MATE panel applet for CPU, memory and GPU
telemetry. It reads the Linux `/proc` and `/sys` interfaces directly, so it
does not require `lm-sensors`, `nvidia-smi`, a panel overlay or a user service.

The applet supports CPU temperature and usage, available RAM, and temperature,
usage and free VRAM for each detected GPU. AMD/Intel DRM sysfs metrics are
used when available. On systems where NVIDIA metrics are not exposed through
sysfs, the optional `nvidia-smi` command is used as a fallback; if it is not
installed, the applet remains usable and marks those metrics unavailable.

Use the applet menu to select metrics, the refresh interval and the temperature
unit. All settings are stored per applet instance through GSettings.

This applet is GPL-3.0-or-later, like the rest of MATE Applets.
