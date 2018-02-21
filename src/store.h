

#ifndef _STORE_H
#define	_STORE_H

#include <hash_map>
#include <queue>

// item id, score 
typedef __gnu_cxx::hash_map<int,int>	storeByID_t;

class JobPriority
{
	public:
		JobPriority( int itemId, int score, int schedule ) : m_itemId( itemId ), m_score( score ), m_schedule( schedule )	{}
		JobPriority( int itemId, int score, int schedule, const char *jobText ) : m_itemId( itemId ), m_score( score ), m_schedule( schedule ), m_jobText( jobText )	{}
		int score() const						{ return m_score; }
		int itemId() const						{ return m_itemId; }
		int schedule() const					{ return m_schedule; }
		const std::string &getJob() const		{ return m_jobText; }
		bool operator<( const JobPriority &j ) const
		{
			// high priority for low scores
			return score() > j.score();
		}

	private:
		int m_score;
		int m_itemId;
		int m_schedule;
		std::string m_jobText;
};

// by score
typedef std::priority_queue<JobPriority> storeByScore_t;

class JobStore
{
	public:
		JobStore()	{}
		int		peekNext() const;
		int 	getNext();
		int 	getNext( std::string &jobText );
		int		add( int itemid, int score, int schedule, const char *jobText );
		
	private:
//		storeByID_t		m_jobsByID;
		storeByScore_t 	m_jobsByScore;
};

/**
 * These are the functions to access the priority queue
 */
// returns the itemId of the item on the top of the queue and leaves item in queue or -1 if queue is empty
int peekNext( int withSchedule );
// returns the itemId of the item on the top of the queue and removes item from queue or -1 if queue is empty
int getNext();
int getNextJobText( std::string &outJob );

// cancel a job
int cancelID( int itemId );
// returns the score of a specific itemId or -1 if the item is not in the queue
int getScore(int itemId);
int getSchedule( int itemId );
// returns 1 on success, 0 on failure
int update(int itemId, int score, int schedule, bool checkIfExists = true);
int updateJobText(int itemId, int score, int schedule, const char *job );
// iterates through scores and outputs them in the format "itemId score\r\n" to the given file.
// pass NULL for tree to start at the root of the tree
void outputScores(FILE *fd);
//void outputScoresIterator(FILE *fd, ScoreTreeNode tree);
void initializePriorityQueue();

/*
void dumpItems();
void dumpItemsIterator(ItemTreeNode tree);
void dumpScores();
void dumpScoresIterator(ScoreTreeNode tree);
*/




#endif

