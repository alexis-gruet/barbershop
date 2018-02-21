/*
 Modified by Dwayn Matthies <dwayn(dot)matthies(at)gmail(dot)com>
 to use pqueue.h to handle the priority queue
Copyright (c) 2010 Nick Gerakines <nick at gerakines dot net>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "commands.h"
#include "store.h"
#include "stats.h"
#include "barbershop.h"
#include "Blacklist.h"

#define _TRACE

using namespace Blacklisting;

/*
	Kroknet - 
    Tweaked update verb to take advantage of the scheduling
    Alexis Gruet
*/
void command_update(int fd, token_t *tokens) {
	int item_id = atoi(tokens[1].value);
	int score = atoi(tokens[2].value);
	int schedule = atoi( tokens[3].value );
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
/*	if (score < 1) {
		reply(fd, "-ERROR INVALID SCORE");
		return;
	}
*/
	pthread_mutex_lock(&scores_mutex);
	int success = update(item_id, score, schedule, 1); //kroknet

	if(success >= 0)
		reply(fd, "+OK");
	else
		reply(fd, "-ERROR UPDATE FAILED");
	pthread_mutex_unlock(&scores_mutex);
}

void command_next(int fd, token_t *tokens) {
	int next;
	pthread_mutex_lock(&scores_mutex);
	next = getNext();
	pthread_mutex_unlock(&scores_mutex);

	char msg[32];
	sprintf(msg, "+%d", next);
	reply(fd, msg);
}

void command_peek(int fd, token_t *tokens) {
	int next;
	pthread_mutex_lock(&scores_mutex);
	next = peekNext( 0 );
	pthread_mutex_unlock(&scores_mutex);
	char msg[32];
	sprintf(msg, "+%d", next);
	reply(fd, msg);
}

void command_peekschedule(int fd, token_t *tokens) {
	int next;
	pthread_mutex_lock(&scores_mutex);
	next = peekNext( 1 );
	pthread_mutex_unlock(&scores_mutex);
	char msg[32];
	sprintf(msg, "+%d", next);
	reply(fd, msg);
}

void command_score(int fd, token_t *tokens) {
	int item_id = atoi(tokens[KEY_TOKEN].value);
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
	int score = getScore(item_id);
	char msg[32];
	sprintf(msg, "+%d", score);
	reply(fd, msg);
}

void command_schedule(int fd, token_t *tokens) {
	int item_id = atoi(tokens[KEY_TOKEN].value);
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
	int schedule = getSchedule(item_id);
	char msg[32];
	sprintf(msg, "+%d", schedule);
	reply(fd, msg);
}

void command_info(int fd, token_t *tokens) {
	char out[128];
	time_t current_time;
	time(&current_time);
	pthread_mutex_lock(&scores_mutex);
	sprintf(out, "uptime:%d\r\n", (int)(current_time - app_stats.started_at)); reply(fd, out);
	sprintf(out, "version:%s\r\n", app_stats.version); reply(fd, out);
	sprintf(out, "updates:%u\r\n", app_stats.updates); reply(fd, out);
	sprintf(out, "items:%u\r\n", app_stats.items); reply(fd, out);
	sprintf(out, "pools:%u\r\n", app_stats.pools); reply(fd, out);
	pthread_mutex_unlock(&scores_mutex);
}

//Kroknet
static int nextID( )
{
	static int id=0;

//FIXME. Is it atomic operation or do we need to have mutex?
	return ++id;
}

void command_id(int fd, token_t *tokens) {
	char out[128];

	pthread_mutex_lock(&scores_mutex);
	sprintf( out, "+%d", nextID() );
	reply( fd, out );
	pthread_mutex_unlock(&scores_mutex);
}

// lock ID and commit it on the fly
void command_updateid(int fd, token_t *tokens) {
	char out[128];

	int item_id = nextID();
	int paramIndex = KEY_TOKEN;
	int score = atoi(tokens[paramIndex++].value);
	int schedule = atoi( tokens[paramIndex++].value );
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
	if (score < 1) {
		reply(fd, "-ERROR INVALID SCORE");
		return;
	}

	pthread_mutex_lock(&scores_mutex);
	int success = update(item_id, score, schedule, 1);

	if(success >= 0)
	{
		sprintf( out, "+%d", item_id );
		reply( fd, out );
	}
	else
		reply(fd, "-ERROR UPDATE FAILED");
	pthread_mutex_unlock(&scores_mutex);
}

// lock ID and commit it on the fly
void command_updateidtext(int fd, token_t *tokens) {
	char out[128];

	int item_id = nextID();
	int paramIndex = KEY_TOKEN;
	int score = atoi(tokens[paramIndex++].value);
	int schedule = atoi( tokens[paramIndex++].value );
	
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
	if (score < 1) {
		reply(fd, "-ERROR INVALID SCORE");
		return;
	}
	if (tokens[paramIndex].value == NULL ) {
		reply(fd, "-ERROR INVALID JOB CONTENT");
		return;
	}

	pthread_mutex_lock(&scores_mutex);
#ifdef _TRACE
	printf( "UPDATEIDTEXT %d %d <%s>", score, schedule, tokens[paramIndex].value );
#endif
	int success = updateJobText(item_id, score, schedule, tokens[paramIndex].value );

	if(success >= 0)
	{
		sprintf( out, "+%d", item_id );
		reply( fd, out );
	}
	else
		reply(fd, "-ERROR UPDATE FAILED");
	pthread_mutex_unlock(&scores_mutex);
}

void command_next_text(int fd, token_t *tokens) {
	int next;
	pthread_mutex_lock(&scores_mutex);
	std::string jobText;
	next = getNextJobText( jobText );
	pthread_mutex_unlock(&scores_mutex);

//FIXME. Max length of job is limited here (2045 chars)
	char msg[2048];
	if( next == -1 )
		strcpy( msg, "+" );
	else
		sprintf(msg, "+%s", jobText.c_str());
	reply(fd, msg);
}

// lock ID so that client can prepare the ID
void command_lockid(int fd, token_t *tokens) {
	char out[128];

	int item_id = nextID();
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}

	sprintf( out, "+%d", item_id );
	reply( fd, out );
}

// commit ID given by lockID
void command_commitid(int fd, token_t *tokens) {
	char out[128];

	int item_id = atoi(tokens[1].value);
	int score = atoi(tokens[2].value);
	int schedule = atoi( tokens[3].value );
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
	if (score < 1) {
		reply(fd, "-ERROR INVALID SCORE");
		return;
	}

	pthread_mutex_lock(&scores_mutex);
	int success = update(item_id, score, schedule, 0);

//printf( "success=%d id=%d\n", success, item_id );
	if(success >= 0)
	{
		sprintf( out, "+%d", item_id );
		reply( fd, out );
	}
	else
		reply(fd, "-ERROR UPDATE FAILED");
	pthread_mutex_unlock(&scores_mutex);
}

void command_cancel(int fd, token_t *tokens) {
	int item_id = atoi(tokens[KEY_TOKEN].value);
	if (item_id == 0) {
		reply(fd, "-ERROR INVALID ITEM ID");
		return;
	}
	int cancel = cancelID( item_id );
	if( cancel == 0 )
		reply( fd, "+OK" );
	reply( fd, "-ERROR INVALID ID" );
}

void command_checkpid( int fd, token_t *tokens )
{
	int pid = atoi( tokens[KEY_TOKEN].value );
	printf( "Checking PID %d\n", pid );
	if( pid <= 0 || pid > 65535 )
	{
		reply(fd, "-ERROR INVALID PID" );
		return;
	}
	char filename[255];
	snprintf( filename, sizeof( filename ), "/proc/%d/cmdline", pid );
	int fpid = open(filename, O_RDONLY);
  	if (fpid < 0){
		reply(fd, "-ERROR CAN'T FIND PID" );
		return;
	}
  /**
   * Try to read contents of the file
   */
	char buf[255];
 	if (read(fpid, buf, sizeof(buf)) <= 0){
    		reply( fd, "-ERROR PID CAN'T BE OPENED");
		return;
	}
     close(fpid);
	reply( fd, "+1" );
}

void split_words( const char *words, std::vector<std::string> &list )
{
	char word[ 1024 ];

	size_t i=0;
#ifdef _TRACE
			printf( "words=<%s>\n", words );
#endif
	for( const char *p = words; *p; p++ )
	{
		if( isalpha( *p ) )
			word[i++] = *p;
		else
		{
			word[ i ] = 0;
#ifdef _TRACE
			printf( "word1=<%s>\n", word );
#endif
			if( i>0 )
				list.push_back( word );
			i = 0;
		}
	}
	word[ i ] = 0;
#ifdef _TRACE
			printf( "word2=<%s> i=%d\n", word, i );
#endif
	if( i>0 )
		list.push_back( word );
}

void command_blacklist(int fd, token_t *tokens) {
static Blacklist bl;
static bool init =false;

#ifdef _TRACE
	printf( "command_blacklist\n" );
#endif
	pthread_mutex_lock(&scores_mutex);
	if( !init )
		bl.load( "/etc/barbershop/config/blacklist.txt" );
//		bl.addText( "nrj", BLS_START );
	pthread_mutex_unlock(&scores_mutex);

	std::string blacklisted;
	std::vector<std::string> words;
	for( size_t i=KEY_TOKEN; tokens[i].value; i++ )
		split_words( tokens[i].value, words );
	std::vector<std::string>::const_iterator iWord = words.begin();
	for( ; iWord != words.end(); ++iWord )
	{
		if( bl.findText( (*iWord).c_str(), blacklisted ) )
		{
			char msg[1024];
			sprintf( msg, "-Le nom <%.900s> ne peut pas être utilisé", blacklisted.c_str() );
			reply( fd, msg );
			return;
		}
	}
	reply( fd, "+OK" );
}


// TODO: Clean the '\r\n' scrub code.
// TODO: Add support for the 'quit' command.
void process_request(int fd, char *input) {

#ifdef _TRACE
	printf( "process_request buffer=<%s>\n", input );
#endif

/*
	char* nl;
	nl = strrchr(input, '\r');
	if (nl) { *nl = '\0'; }
	nl = strrchr(input, '\n');
	if (nl) { *nl = '\0'; }
*/
	token_t tokens[MAX_TOKENS];
	size_t ntokens = tokenize_command(input, tokens, MAX_TOKENS);
#ifdef _TRACE
	printf( "process request=<%s> nbTokens=%d\n", input, ntokens );
#endif

    /*
    	Kroknet - 
    	Tweaked update verb to take advantage of the scheduling
    	Alexis Gruet
     */
	if (ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "UPDATE") == 0) {
		command_update(fd, tokens);
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "PEEK") == 0) {
		command_peek(fd, tokens);
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "NEXT") == 0) {
		command_next(fd, tokens);
	} else if (ntokens == 3 && strcmp(tokens[COMMAND_TOKEN].value, "SCORE") == 0) {
		command_score(fd, tokens);
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "INFO") == 0) {
		command_info(fd, tokens);
//Kroknet
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "ID") == 0) {
		command_id(fd, tokens);
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "LOCKID") == 0) {
		command_lockid(fd, tokens);
	} else if (ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "COMMITID") == 0) {
		command_commitid(fd, tokens);
	} else if (ntokens == 4 && strcmp(tokens[COMMAND_TOKEN].value, "UPDATEID") == 0) {
		command_updateid(fd, tokens);
	} else if (ntokens == 5 && strcmp(tokens[COMMAND_TOKEN].value, "UPDATEIDTEXT") == 0) {
		command_updateidtext(fd, tokens);
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "NEXTTEXT") == 0) {
		command_next_text(fd, tokens);
	} else if (ntokens == 3 && strcmp(tokens[COMMAND_TOKEN].value, "SCHEDULE") == 0) {
		command_score(fd, tokens);
	} else if (ntokens == 2 && strcmp(tokens[COMMAND_TOKEN].value, "PEEKSCHEDULE") == 0) {
		command_peekschedule(fd, tokens);
	} else if (ntokens == 3 && strcmp(tokens[COMMAND_TOKEN].value, "CANCEL") == 0) {
		command_cancel(fd, tokens);
	} else if (ntokens == 3 && strcmp(tokens[COMMAND_TOKEN].value, "PID") == 0) {
		command_checkpid( fd, tokens );
		// BLACKLIST <word>
	} else if (ntokens >= 3 && strcmp(tokens[COMMAND_TOKEN].value, "BLACKLIST") == 0) {
		command_blacklist( fd, tokens );
	} else {
		printf( "Invalid input=<%s>\n", input );
		reply(fd, "-ERROR");
	}
}

size_t tokenize_command(char *command, token_t *tokens, const size_t max_tokens) {
	char *s, *e;
	size_t ntokens = 0;
	bool betweenQuotes = false;
	char *lastQuote = NULL;

		// if any parameter between quotes, then read all chars as part of the token
	for (s = e = command; ntokens < max_tokens - 1; ++e) {
#ifdef _TRACE
//printf( "char=%c %x inquotes=%d\n", *e,*e, betweenQuotes );
#endif
		if (*e == ' ' && !betweenQuotes ) {
			if (s != e) {
				tokens[ntokens].value = s;
				if( lastQuote )
				{
					tokens[ntokens].length = e - s - 1;
					*lastQuote = 0;
				}
				else
					tokens[ntokens].length = e - s;
				*e = 0;
#ifdef _TRACE
				printf( "token %d  len=%d [%s] {%s}\n", ntokens,tokens[ntokens].length,tokens[ntokens].value, s );
#endif
				ntokens++;
				lastQuote = NULL;
			}
			s = e + 1;
		} else if (*e == '\0') {
			if (s != e) {
				tokens[ntokens].value = s;
				if( lastQuote )
				{
					tokens[ntokens].length = e - s - 1;
					*lastQuote = 0;
				}
				else
					tokens[ntokens].length = e - s;
				*e = 0;
#ifdef _TRACE
				printf( "token %d  len=%d [%s] {%s}\n", ntokens,tokens[ntokens].length,tokens[ntokens].value, s );
#endif
				ntokens++;
			}
			break;
		}
		if( *e == '"' )
		{
			if( !betweenQuotes )
				s++;		// skip first quote
			betweenQuotes = !betweenQuotes;
			lastQuote = e;
		}
	}
	tokens[ntokens].value =  *e == '\0' ? NULL : e;
	tokens[ntokens].length = 0;
	ntokens++;
	return ntokens;
}

void reply(int fd, const char *buffer) {
	int n = write(fd, buffer, strlen(buffer));
	if (n < 0 || n < strlen(buffer)) {
		printf("ERROR writing to socket");
	}
}
