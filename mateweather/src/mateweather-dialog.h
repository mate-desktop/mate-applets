#ifndef __MATEWEATHER_DIALOG_H_
#define __MATEWEATHER_DIALOG_H_

/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main status dialog
 *
 */

#include <gtk/gtk.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "mateweather.h"

G_BEGIN_DECLS

#define MATEWEATHER_TYPE_DIALOG mateweather_dialog_get_type ()
G_DECLARE_FINAL_TYPE (MateWeatherDialog, mateweather_dialog,
                      MATEWEATHER, DIALOG, GtkDialog)

GtkWidget        *mateweather_dialog_new                (MateWeatherApplet *applet);
void              mateweather_dialog_update             (MateWeatherDialog *dialog);

G_END_DECLS

#endif /* __MATEWEATHER_DIALOG_H_ */

