#ifndef _WIN32
#include <libintl.h>
#define _(STRING) gettext(STRING)
#else
#define _(STRING) STRING
#endif