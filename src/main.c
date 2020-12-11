#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "cmdline.h"
#include "lastread.h"
#include "core.h"
#include "curlutils.h"

char interrupted = 0;
CMDLine cmdline;

void handleInt(int dummy)
{
	if(cmdline.verbose)
		printf("SIGINT received.\n");
	interrupted = 1;
}


int main(int argc, char **argv)
{
	int r = cmdline_read(&cmdline, argc, argv);
	if(r) return r;
	
	FILE *lastread = lastread_open(cmdline.twitter_last_read_file);
	if(lastread == NULL) return 74;
	if(cmdline.verbose)
		printf("Opened lastread file %s.\n", cmdline.twitter_last_read_file);

	CURL *curl = curl_easy_init();
	struct curl_slist *headers = NULL;
	char *auth = calloc(strlen("Authorization: Bearer ") + strlen(cmdline.twitter_token) + 1, sizeof(char));
	strcpy(auth, "Authorization: Bearer ");
	strcat(auth, cmdline.twitter_token);
	if(cmdline.verbose)
		printf("Created Twitter auth header %s.\n", auth);
	headers = curl_slist_append(headers, auth);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);

	signal(SIGINT, handleInt);
	if(cmdline.verbose)
		printf("Trap SIGINT.\n");
	char lastread_file[1001];
	char has_lastread = fgets(lastread_file, 1000, lastread) != NULL && lastread_file[0] != '\0';
	if(has_lastread) 
	{
		lastread_file[strcspn(lastread_file, "\r\n")] = 0;
		has_lastread = lastread_file[0] != '\0';
		if(cmdline.verbose)
			printf("Read from lastread file: %s.\n", lastread_file);
	}
	else
	{
		if(cmdline.verbose)
			printf("The lastread file is empty.\n");
	}
	rewind(lastread);
	char *lastread_out = NULL;
	while(!interrupted)
	{
		if(cmdline.verbose)
			printf("Loop begins. Last time reported lastread is %s.\n", lastread_out);
		char *lastread_final = NULL;
		if(lastread_out != NULL) lastread_final = lastread_out;
		if(has_lastread) lastread_final = lastread_file;
		if(cmdline.verbose)
			printf("Figured final lastread out: %s.\n", lastread_final);
		r = core_run_once(curl, lastread_final, &cmdline, &interrupted, &lastread_out);
		if(r) 
		{
			lastread_out = NULL;
			break;
		}
		if(cmdline.wait != 0)
		{
			if(cmdline.verbose)
				printf("Begin sleep.\n");
			errno = 0;
			sleep(cmdline.wait);
			if(cmdline.verbose)
				printf("End sleep. %u\n", errno);
		}
		else 
			break;
	}

	if(lastread_out != NULL)
	{
		if(cmdline.verbose)
			printf("Writing lastread to file: %s.\n", lastread_out);
		fputs(lastread_out, lastread);
	}
	else
	{
		if(cmdline.verbose)
			printf("The lastread reported is empty.\n");
	}
	if(cmdline.verbose)
		printf("Cleaning up.\n");
	fclose(lastread);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	free(auth);
	return r;
}
