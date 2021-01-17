#ifndef H_MULTILOAD_NETSPEED_
#define H_MULTILOAD_NETSPEED_

#include <glib.h>

typedef struct _NetSpeed NetSpeed;

#include "global.h"

NetSpeed* netspeed_new(LoadGraph *graph);
void netspeed_delete(NetSpeed *ns);
void netspeed_add(NetSpeed *ns, guint64 tx);
char* netspeed_get(NetSpeed *ns);

#endif /* H_MULTILOAD_NETSPEED_ */
