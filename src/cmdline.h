#ifndef CMDLINE_H
#define CMDLINE_H

typedef struct cmdline
{
	char *twitter_token;
	char *twitter_user;
	char *telegram_token;
	char *telegram_chat;

	char *twitter_max;
	char *twitter_last_read_file;
	int twitter_exclude_replies;
	int twitter_include_rts;

	unsigned int wait;
	char verbose;
} CMDLine;

int cmdline_read(CMDLine *cmdline, int argc, char **argv);

#endif // CMDLINE_H
