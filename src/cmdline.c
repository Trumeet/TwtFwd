#include "cmdline.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

int cmdline_validate(CMDLine *cmdline);
int cmdline_parse_wait(CMDLine *cmdline, char *supplied_str);
int cmdline_parse_bool(char *supplied_str);

void cmdline_reset(CMDLine *cmdline)
{
	cmdline->twitter_token = NULL;
	cmdline->twitter_user = NULL;
	cmdline->telegram_token = NULL;
	cmdline->telegram_chat = NULL;

	cmdline->twitter_max = "30";
	cmdline->twitter_last_read_file = "/var/lib/twtfwd/last";
	cmdline->twitter_exclude_replies = 1;
	cmdline->twitter_include_rts = 0;

	cmdline->wait = 0;
	cmdline->verbose = 0;
}

int cmdline_read_env(CMDLine *cmdline)
{
	cmdline->twitter_token = getenv("TWITTER_TOKEN");
	cmdline->twitter_user = getenv("TWITTER_USER");
	cmdline->telegram_token = getenv("TELEGRAM_TOKEN");
	cmdline->telegram_chat = getenv("TELEGRAM_CHAT");

	if(getenv("TWITTER_MAX") != NULL)
		cmdline->twitter_max = getenv("TWITTER_MAX");
	if(getenv("TWITTER_LAST_READ_FILE") != NULL)
		cmdline->twitter_last_read_file = getenv("TWITTER_LAST_READ_FILE");
	if(getenv("TWITTER_EXCLUDE_REPLIES") != NULL)
		cmdline->twitter_exclude_replies = cmdline_parse_bool(getenv("TWITTER_EXCLUDE_REPLIES"));
	if(getenv("TWITTER_INCLUDE_RTS") != NULL)
		cmdline->twitter_exclude_replies = cmdline_parse_bool(getenv("TWITTER_INCLUDE_RTS"));
	if(getenv("WAIT") != NULL)
	{
		int r = cmdline_parse_wait(cmdline, getenv("WAIT"));
		if(r) return r;
	}
	if(getenv("VERBOSE") != NULL)
		cmdline->verbose = cmdline_parse_bool(getenv("VERBOSE"));
	return 0;
}

int cmdline_read_arg(CMDLine *cmdline, int argc, char **argv)
{
	for(int i = 1; i < argc; i ++)
	{
		int last = i == (argc - 1);
		if(!strcmp(argv[i], "-twt") || !strcmp(argv[i], "--twitter-token"))
		{
			if(last)
			{
				fprintf(stderr, "-twt or --twitter-token must be followed by your Twitter token.\n");
				return 64;
			}
			cmdline->twitter_token = argv[++i];
			continue;
		}
		if(!strcmp(argv[i], "-twu") || !strcmp(argv[i], "--twitter-user"))
		{
			if(last)
			{
				fprintf(stderr, "-twu or --twitter-user must be followed by your Twitter username.\n");
				return 64;
			}
			cmdline->twitter_user = argv[++i];
			continue;
		}
		if(!strcmp(argv[i], "-tgt") || !strcmp(argv[i], "--telegram-token"))
		{
			if(last)
			{
				fprintf(stderr, "-tgt or --telegram-token must be followed by your Telegram token.\n");
				return 64;
			}
			cmdline->telegram_token = argv[++i];
			continue;
		}
		if(!strcmp(argv[i], "-tgc") || !strcmp(argv[i], "--telegram-chat"))
		{
			if(last)
			{
				fprintf(stderr, "-twc or --telegram-chat must be followed by your Telegram chat.\n");
				return 64;
			}
			cmdline->telegram_chat = argv[++i];
			continue;
		}
		if(!strcmp(argv[i], "--twitter-max"))
		{
			if(last)
			{
				fprintf(stderr, "--twitter-max must be followed by your max twitter forward count.\n");
				return 64;
			}
			cmdline->twitter_max = argv[++i];
			continue;
		}
		if(!strcmp(argv[i], "--twitter-last-read-file"))
		{
			if(last)
			{
				fprintf(stderr, "--twitter-last-read-file must be followed by your file to store the last read tweet ID.\n");
				return 64;
			}
			cmdline->twitter_last_read_file = argv[++i];
			continue;
		}
		if(!strcmp(argv[i], "--twitter-exclude-replies"))
		{
			if(last)
			{
				fprintf(stderr, "--twitter-exclude-replies must be followed by whether to include replies.\n");
				return 64;
			}
			cmdline->twitter_exclude_replies = cmdline_parse_bool(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--twitter-include-rts"))
		{
			if(last)
			{
				fprintf(stderr, "--twitter-include-rts must be followed by whether to include retweets.\n");
				return 64;
			}
			cmdline->twitter_include_rts = cmdline_parse_bool(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--wait"))
		{
			if(last)
			{
				fprintf(stderr, "--wait must be followed by the time to wait.\n");
				return 64;
			}
			cmdline_parse_wait(cmdline, argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--verbose"))
		{
			cmdline->verbose = 1;
			continue;
		}
		if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
		{
			printf("Usage: twtfwd [OPTION]...\n");
			printf("Forward one's Tweets to Telegram.\n");
			printf("\n");
			printf("\t-twt, --twitter-token\t\tset the Twitter token\n");
			printf("\t-twu, --twitter-user\t\tset the Twitter username without @\n");
			printf("\t-tgt, --telegram-token\t\tset the Telegram bot token\n");
			printf("\t-tgc, --telegram-chat\t\tset the Telegram chat\n");
			printf("\t--twitter-max\t\t\tset how many Tweets to retreve each time\n");
			printf("\t--twitter-last-read-file\tset the location to a text file that will store the last processed Tweet ID\n");
			printf("\t--twitter-exclude-replies\twhether to exclude replies. Default is yes.\n");
			printf("\t--twitter-include-rts\t\twhether to include retweets. Default is no.\n");
			printf("\t--wait\t\t\t\tthe amount of time to wait between each two poll in seconds. Default is 0, or it exits immediatedly after sending the messages.\n");
			printf("\t--verbose\t\t\tbe verbose\n");
			return 64;
		}
		fprintf(stderr, "Unknown option %s.\n", argv[i]);
		return 64;
	}
	return 0;
}

int cmdline_read(CMDLine *cmdline, int argc, char **argv)
{
	cmdline_reset(cmdline);
	int r = cmdline_read_env(cmdline);
	if(r) return r;
	r = cmdline_read_arg(cmdline, argc, argv);
	if(r) return r;
	r = cmdline_validate(cmdline);
	if(r) return r;
	return 0;
}

int cmdline_validate(CMDLine *cmdline)
{
	if(cmdline->twitter_token == NULL || !strcmp(cmdline->twitter_token, ""))
	{
		fprintf(stderr, "The Twitter token is empty.\n");
		return 64;
	}
	if(cmdline->twitter_user == NULL || !strcmp(cmdline->twitter_user, ""))
	{
		fprintf(stderr, "The Twitter user is empty.\n");
		return 64;
	}
	if(cmdline->telegram_token == NULL || !strcmp(cmdline->telegram_token, ""))
	{
		fprintf(stderr, "The Telegram token is empty.\n");
		return 64;
	}
	if(cmdline->telegram_chat == NULL || !strcmp(cmdline->telegram_chat, ""))
	{
		fprintf(stderr, "The Telegram chat is empty.\n");
		return 64;
	}
	if(cmdline->twitter_last_read_file == NULL || !strcmp(cmdline->twitter_last_read_file, ""))
	{
		fprintf(stderr, "The Twitter last read file is empty.\n");
		return 64;
	}
	return 0;
}

int cmdline_parse_wait(CMDLine *cmdline, char *supplied_str)
{
	char *endptr = NULL;
	long wait = strtol(supplied_str, &endptr, 10);
	if(endptr != NULL && strcmp(endptr, ""))
	{
		fprintf(stderr, "WAIT or --wait must be a number.\n");
		return 64;
	}
	if(wait > INT_MAX || wait <= 0)
	{
		fprintf(stderr, "WAIT or --wait must be in the range of %u to %u (inclusive). You supplied %lu.\n", 
				1, INT_MAX, wait);
		return 64;
	}
	cmdline->wait = (unsigned int)wait;
	return 0;
}

int cmdline_parse_bool(char *supplied_str)
{
	if(!strcmp(supplied_str, "1")) return 1;
	if(!strcmp(supplied_str, "yes")) return 1;
	if(!strcmp(supplied_str, "true")) return 1;
	return 0;
}
