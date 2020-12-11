#include "lastread.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

FILE *lastread_open(const char *path)
{
	FILE *file = NULL;
	if(access(path, F_OK))
	{
		errno = 0;
		file = fopen(path, "w+");
	}
	else
	{
		file = fopen(path, "r+");
	}
	if(errno)
	{
		fprintf(stderr, "Cannot read and write to %s: %s.\n", path, strerror(errno));
		return NULL;
	}
	return file;
}
