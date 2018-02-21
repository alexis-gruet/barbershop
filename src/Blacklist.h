#ifndef _BLACKLIST_H
#define _BLACKLIST_H

#include <map>
#include <string>

namespace Blacklisting
{

	// these methods
enum Search
{
	BLS_NONE	= 0x0000,
	BLS_MATCH	= 0x0001,
	BLS_START	= 0x0011,
	BLS_END		= 0x0100,
	BLS_CONTAINS= 0x1000
};

class BlacklistNode
{
public:

	BlacklistNode( BlacklistNode *parent, char letter, int searchMethods ) : m_parent( parent ),m_letter( letter ), m_searchMethods( searchMethods )	{}

	BlacklistNode	*queryNode( char letter, int searchMethods, bool createIfNotExisting );
	int				getSearchMethods() const				{ return m_searchMethods; }
	BlacklistNode	*getParent() const						{ return m_parent; }
	char			getLetter() const						{ return m_letter; }

private:
	BlacklistNode *m_parent;
	char	m_letter;
	int		m_searchMethods;
		// letter => node
	std::map<char, BlacklistNode *> m_children;
};

class BlacklistTree
{
public:

	BlacklistTree( bool reversed );
	void	addText( const std::string &, int searchMethods );
	int		findText( const std::string &, std::string &blacklisted ) const;

private:
	BlacklistNode 	*queryNode( char letter );
	void 			parseWord( BlacklistNode *n, std::string &word ) const;

	BlacklistNode *m_root;
	bool			m_reversed;
};

class Blacklist
{
public:
	Blacklist();

	void					load( const char *pathname );
	void					addText( const std::string &, int searchMethods );
	int						findText( const std::string &, std::string &blacklisted ) const;

private:
	BlacklistTree m_normal;
	BlacklistTree m_reverse;
};
}

#endif
