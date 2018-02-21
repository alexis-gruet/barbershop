#include <stdlib.h>
#include <stdio.h>
#include "store.h"

#define _TRACE

static JobStore store;

void initializePriorityQueue()
{
}

int peekNext( int withSchedule )
{
//TODO.withSchedule
	return store.peekNext();
}

int getNext()
{
	return store.getNext();
}

int getNextJobText( std::string &jobText )
{
	return store.getNext( jobText );
}

//Kroknet
int cancelID( int itemId )
{
//TODO.
	return -1;
}

int getScore(int itemId)
{
//TODO.
	return -1;
}

int getSchedule( int itemId )
{
//TODO
	return -1;
}

// returns 1 on adding item, 0 on successful update of item, -1 on error
int update(int itemId, int score, int schedule, bool checkIfExists )
{
	return store.add( itemId, score, schedule, NULL );
}

// returns 1 on adding item, 0 on successful update of item, -1 on error
int updateJobText(int itemId, int score, int schedule, const char *job )
{
	return store.add( itemId, score, schedule, job );
}


void outputScores(FILE *fd)
{
//TODO.	outputScoresIterator(fd, score_root);
}


int JobStore::peekNext() const
{
	if( m_jobsByScore.empty() )
	{
#ifdef _TRACE
printf( "Empty store\n" );
#endif
		return -1;
	}

	JobPriority j = m_jobsByScore.top();
#ifdef _TRACE
printf( "job id=%d score=%d schedule=%d time=%d\n", j.itemId(), j.score(), j.schedule(), (int )time(NULL) );
#endif
	if( j.schedule() == 0 || j.schedule() <= time(NULL) )
		return j.itemId();
	return -1;
}

int JobStore::getNext()
{
#ifdef _TRACE
printf( "getNext\n" );
#endif
	if( m_jobsByScore.empty() )
		return -1;

	int itemId = peekNext();
	if( itemId != -1 )
		m_jobsByScore.pop();
//	m_storeByID.erase( itemid );
	return itemId;
}

int JobStore::getNext( std::string &jobText )
{
#ifdef _TRACE
printf( "getNextText\n" );
#endif
	int itemId = peekNext();
	if( itemId != -1 )
	{
		JobPriority j = m_jobsByScore.top();
		m_jobsByScore.pop();
		jobText = j.getJob();
#ifdef _TRACE
printf( "getNextText = %s\n", jobText.c_str() );
#endif
	}
	return itemId;
}

int JobStore::add( int itemID, int score, int schedule, const char *jobText )
{
#ifdef _TRACE
printf( "Adding job %d %s\n", itemID, jobText ? jobText : "<no text>" );
#endif
	if( jobText )
		m_jobsByScore.push( JobPriority( itemID, score, schedule, jobText ) );
	else
		m_jobsByScore.push( JobPriority( itemID, score, schedule ) );
	return 1;
}


