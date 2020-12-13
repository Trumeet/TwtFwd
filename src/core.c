#include "core.h"
#include "curlutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>

#define TWITTER_URL_PART_1 "https://api.twitter.com/1.1/statuses/user_timeline.json?trim_user=true&exclude_replies="
#define TWITTER_URL_PART_2 "&include_rts="
#define TWITTER_URL_PART_3 "&count="
#define TWITTER_URL_PART_4 "&screen_name="
#define TWITTER_URL_SINCE "&since_id="

#define TG_URL_PART_1 "https://api.telegram.org/bot"
#define TG_URL_PART_2 "/sendMessage?chat_id="
#define TG_URL_PART_3 "&text="

#define TG_TEXT_PART_1 "https://twitter.com/"
#define TG_TEXT_PART_2 "/status/"

char *core_build_url(CMDLine *cmdline, char *lastread_str)
{
	char has_lastread = lastread_str != NULL;

	// Convert bool to string
	char *exclude_replies = cmdline->twitter_exclude_replies ? "true" : "false";
	char *include_rts = cmdline->twitter_include_rts ? "true" : "false";

	int url_length = strlen(TWITTER_URL_PART_1);
	url_length += strlen(exclude_replies);
	url_length += strlen(TWITTER_URL_PART_2);
	url_length += strlen(include_rts);
	url_length += strlen(TWITTER_URL_PART_3);
	url_length += strlen(cmdline->twitter_max);
	url_length += strlen(TWITTER_URL_PART_4);
	url_length += strlen(cmdline->twitter_user);
	url_length ++;

	if(has_lastread)
	{
		url_length += strlen(TWITTER_URL_SINCE);
		url_length += strlen(lastread_str);
	}

	char *url = calloc(url_length, sizeof(char));
	strcpy(url, TWITTER_URL_PART_1);
	strcat(url, exclude_replies);
	strcat(url, TWITTER_URL_PART_2);
	strcat(url, include_rts);
	strcat(url, TWITTER_URL_PART_3);
	strcat(url, cmdline->twitter_max);
	strcat(url, TWITTER_URL_PART_4);
	strcat(url, cmdline->twitter_user);
	if(has_lastread)
	{
		strcat(url, TWITTER_URL_SINCE);
		strcat(url, lastread_str);
	}
	if(cmdline->verbose)
		printf("Built Twitter URL: %s.\n", url);
	return url;
}

int core_send_to_tg(CURL *curl, char *id, CMDLine *cmdline)
{
	int text_raw_length = strlen(TG_TEXT_PART_1);
	text_raw_length += strlen(cmdline->twitter_user);
	text_raw_length += strlen(TG_TEXT_PART_2);
	text_raw_length += strlen(id);
	char *text_raw = calloc(text_raw_length, sizeof(char));
	strcpy(text_raw, TG_TEXT_PART_1);
	strcat(text_raw, cmdline->twitter_user);
	strcat(text_raw, TG_TEXT_PART_2);
	strcat(text_raw, id);
	char *text_escaped = curl_easy_escape(curl, text_raw, 0);
	free(text_raw);
	text_raw = NULL;

	int url_length = strlen(TG_URL_PART_1);
	url_length += strlen(cmdline->telegram_token);
	url_length += strlen(TG_URL_PART_2);
	url_length += strlen(cmdline->telegram_chat);
	url_length += strlen(TG_URL_PART_3);
	url_length += strlen(text_escaped);
	url_length ++;

	char *url = calloc(url_length, sizeof(char));
	strcpy(url, TG_URL_PART_1);
	strcat(url, cmdline->telegram_token);
	strcat(url, TG_URL_PART_2);
	strcat(url, cmdline->telegram_chat);
	strcat(url, TG_URL_PART_3);
	strcat(url, text_escaped);
	curl_free(text_escaped);

	if(cmdline->verbose)
		printf("Built Telegram URL: %s.\n", url);

	curl_easy_setopt(curl, CURLOPT_URL, url);

	CURLBody body;
	curlbody_setup(&body);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

	int r = curl_easy_perform(curl);
	if(r != CURLE_OK)
	{
		fprintf(stderr, "Cannot request the Telegram API: %u.\n", r);
		goto end;
	}

	long tg_http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &tg_http_code);
	if(tg_http_code != 200)
	{
		fprintf(stderr, "Telegram API returns %lu.\n", tg_http_code);
		fprintf(stderr, "%s\n", body.ptr);
		r = 74;
		goto end;
	}
end:
	free(url);
	free(body.ptr);
	return r;
}

int core_run_once(CURL *curl, char *lastread, CMDLine *cmdline, char **lastread_out)
{
	char *url = core_build_url(cmdline, lastread);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	CURLBody body;
	curlbody_setup(&body);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

	int r = curl_easy_perform(curl);
	if(r != CURLE_OK)
	{
		fprintf(stderr, "Cannot request the Twitter API: %u.\n", r);
		goto end;
	}
	long twitter_http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &twitter_http_code);
	if(twitter_http_code != 200)
	{
		fprintf(stderr, "Twitter API returns %lu.\n", twitter_http_code);
		if(twitter_http_code != 404)
		{
			fprintf(stderr, "%s\n", body.ptr);
		}
		r = 74;
		goto end;
	}

	json_object *json = json_tokener_parse(body.ptr);
	int count = json_object_array_length(json);
	for(int i = count - 1; i >= 0; i --)
	{
		if(interrupted)
		{
			if(cmdline->verbose)
				printf("Core: interrupted.\n");
			break;
		}
		char last = i == 0;
		json_object *current = json_object_array_get_idx(json, i);
		char *id_str = (char*)json_object_get_string(json_object_object_get(current, "id_str"));
		if(cmdline->verbose)
			printf("Got Tweet ID %s.\n", id_str);

		r = core_send_to_tg(curl, id_str, cmdline);
		if(r) 
		{
			json_object_put(json);
			goto end;
		}

		if(cmdline->verbose)
			printf("Mark the last ID.\n");
		*lastread_out = id_str;
	}
	json_object_put(json);

end:
	if(cmdline->verbose)
		printf("Exec done. Cleanup.\n");
	free(url);
	free(body.ptr);
	return r;
}
