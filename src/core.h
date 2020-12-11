#ifndef CORE_H
#define CORE_H

#include "cmdline.h"
#include <stdio.h>
#include <curl/curl.h>

int core_run_once(CURL *curl, FILE *lastread, CMDLine *cmdline, char *interrupted);

#endif // CORE_H
