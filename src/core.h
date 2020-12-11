#ifndef CORE_H
#define CORE_H

#include "cmdline.h"
#include <stdio.h>
#include <curl/curl.h>

int core_run_once(CURL *curl, char *lastread, CMDLine *cmdline, char **lastread_out);

#endif // CORE_H
