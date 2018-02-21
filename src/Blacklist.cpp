#include "Blacklist.h"
#include <stdio.h>
#include <string.h>

//#define _TRACE

#define WORD_END_MARKER '$'

using namespace std;
using namespace Blacklisting;

string Reverse ( const string &normal)
{
	int len = normal.length ();
	int n;
	string reverse = normal;
	for (n = 0;n < len; n++)
		reverse[len - n - 1] = normal[n];
	return reverse;
}
BlacklistNode *BlacklistNode::queryNode( char letter, int searchMethods, bool createIfNotExisting )
{
	map<char, BlacklistNode *>::const_iterator iFound = m_children.find( letter );
	if( iFound == m_children.end() )
	{
		if( !createIfNotExisting )
			return NULL;
		BlacklistNode *n = new BlacklistNode( this, letter, searchMethods );
		m_children[ letter ] = n;
		return n;
	}
	return (*iFound).second;
}


BlacklistTree::BlacklistTree( bool reversed ) :
	m_reversed( reversed )
{
	m_root = new BlacklistNode( NULL, 0, 0 );
}

void BlacklistTree::addText( const std::string &text, int searchMethods )
{
	BlacklistNode *n = m_root;
	size_t l = text.length();
	for( size_t i=0; i<l; i++ )
		n = n->queryNode( tolower( text[ i ] ), searchMethods, true );
		// end marker
	n = n->queryNode( WORD_END_MARKER, searchMethods, true );
}

void BlacklistTree::parseWord( BlacklistNode *n, std::string &word ) const
{
	std::string reverse;
	while( n->getParent() && n->getLetter() )
	{
		if( n->getLetter() != WORD_END_MARKER )
			reverse += n->getLetter();
		n = n->getParent();
	}
	word = Reverse( reverse );
}

int BlacklistTree::findText(const std::string &text, std::string &blacklisted ) const
{
#ifdef _TRACE
	printf( "%s with <%s>\n", __FUNCTION__, text.c_str() );
#endif
	BlacklistNode *n = m_root;
	size_t l = text.length();
	size_t i;
	for( i=0; i<l; i++ )
	{
		BlacklistNode *prev = n;
		n = n->queryNode( tolower( text[ i ] ), 0, false );
		if( !n )
		{
				// end of suffix tree => text is starting with a word in the suffix tree
			n = prev->queryNode( WORD_END_MARKER, 0, false );
			if( n )
			{
				parseWord( n, blacklisted );
#ifdef _TRACE
				printf( "Case 1: %s blacklisted because of <%s>\n", text.c_str(), blacklisted.c_str() );
#endif
				if( i == l-1 )
					return BLS_MATCH & n->getSearchMethods();
				if( m_reversed )
					return BLS_END & n->getSearchMethods();
				return BLS_START & n->getSearchMethods();
			}
			return BLS_NONE;
		}
	}
	if( n )
	{
		if( n->queryNode( WORD_END_MARKER, 0, false ) )
		{
			parseWord( n, blacklisted );
	#ifdef _TRACE
			printf( "Case 2: %s blacklisted because of <%s>\n", text.c_str(), blacklisted.c_str() );
	#endif
			return BLS_MATCH & n->getSearchMethods();
		}
	}
	return BLS_NONE;
//	if( n->queryNode( '$', false ) )
//		return true;
//	return false;
}

Blacklist::Blacklist( ) :
	m_normal( false ),
	m_reverse( true )
{
}

void Blacklist::load( const char *pathname )
{
	size_t loaded = 0;
	FILE *f = fopen( pathname, "r" );
	if( f )
	{
		while( !feof( f ) )
		{
			char word[ 1024 ];
			char searchMethods[ 32 ];
			fscanf( f, "%s %s", word, searchMethods );		
			if( strlen( word ) > 0 )
			{
				int search = BLS_NONE;
				for( char *p = searchMethods; *p; p++ )
				{
					switch( *p )
					{
						case 'm':
							search |= BLS_MATCH;
							break;
						case 's':
							search |= BLS_START;
							break;
						case 'e':
							search |= BLS_END;
							break;
						default:
							fprintf( stderr, "Invalid search method '%c'\n", *p );
					}
				}
				addText( word, search );
				loaded++;
			}
		}
		fclose( f );
	}
	printf( "Blacklist dictionary: %d loaded\n", loaded );
}

void Blacklist::addText( const std::string &text, int searchMethods )
{
	m_normal.addText( text, searchMethods );
	m_reverse.addText( Reverse( text ), searchMethods );
}

int Blacklist::findText( const std::string &text ,std::string &blacklisted ) const
{
	int s = m_normal.findText( text, blacklisted );
	if( s != BLS_NONE )
		return s;

	std::string rText( Reverse( text ) );
	std::string rBlacklisted;
	s = m_reverse.findText( rText, rBlacklisted );
	if( s != BLS_NONE )
	{
		blacklisted = Reverse( rBlacklisted );
		return s;
	}
	size_t l = text.length();

#if 0
	for( size_t i=0; i<l; i++ )
	{
		s = m_normal.findText( text.c_str() + i );
		if( s != BlacklistTree::BLS_NONE )
			return BlacklistTree::BLS_CONTAINS;
/*
		s = blReverse.findText( rText.c_str() + i );
		if( s == Blacklist::BLS_START )
			s = Blacklist::BLS_END;
		if( s != Blacklist::BLS_NONE )
			return s;
*/
	}
#endif
	return BLS_NONE;
}


