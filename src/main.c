#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "cmdline.h"
#include "lastread.h"
#include "core.h"
#include "curlutils.h"

char interrupted = 0;

void handleInt(int dummy)
{
	interrupted = 1;
}


int main(int argc, char **argv)
{
	CMDLine cmdline;
	int r = cmdline_read(&cmdline, argc, argv);
	if(r) return r;
	
	FILE *lastread = lastread_open(cmdline.twitter_last_read_file);
	if(lastread == NULL) return 74;

	CURL *curl = curl_easy_init();
	struct curl_slist *headers = NULL;
	char *auth = calloc(strlen("Authorization: Bearer ") + strlen(cmdline.twitter_token) + 1, sizeof(char));
	strcpy(auth, "Authorization: Bearer ");
	strcat(auth, cmdline.twitter_token);
	headers = curl_slist_append(headers, auth);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);

	signal(SIGINT, handleInt);
	char lastread_file[1001];
	char has_lastread = fgets(lastread_file, 1000, lastread) != NULL && lastread_file[0] != '\0';
	if(has_lastread) 
	{
		lastread_file[strcspn(lastread_file, "\r\n")] = 0;
		has_lastread = lastread_file[0] != '\0';
	}
	rewind(lastread);
	char *lastread_out = NULL;
	while(!interrupted)
	{
		char *lastread_final = NULL;
		if(lastread_out != NULL) lastread_final = lastread_out;
		if(has_lastread) lastread_final = lastread_file;
		r = core_run_once(curl, lastread_final, &cmdline, &interrupted, &lastread_out);
		if(r) 
		{
			lastread_out = NULL;
			break;
		}
		if(cmdline.wait != 0)
			sleep(cmdline.wait);
		else 
			break;
	}

	if(lastread_out != NULL)
	{
		fputs(lastread_out, lastread);
	}
	fclose(lastread);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	free(auth);
	return r;
}
