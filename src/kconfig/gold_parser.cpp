/*
 * gold_parser.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * A simple implementation of the GOLD Parsing Engine in C++.
 *
 */

#include "gold_parser.hpp"

#include "boost/scoped_ptr.hpp"
using namespace boost;

#include <istream>
#include <stack>
#include <vector>
using namespace std;

namespace GoldParser {
	
// Source (Interface to the input file)
class Source {
public:
	Source(istream &s, bool cs) : str(s), eof_flag(false), line_number(1), case_sensitive(cs) { }

	bool isEOF();
	char lookAhead(int offset); // offset from the *current* byte in the stream.
	string read(int length);
	
	int getLineNumber() const { return line_number; }
	
private:
	void fillBufToSize(size_t sz);
	
private:
	istream &str;
	deque<char> buf;  // buffer of chars for "lookAhead"
	bool eof_flag;
	int line_number; 
	bool case_sensitive;
};


void Source::fillBufToSize(size_t sz)
{
	while (buf.size() < sz) {
		int i = str.get();
		if (i == istream::traits_type::eof()) {
			eof_flag = true;
			buf.push_back(0);
		} else {
			buf.push_back(istream::traits_type::to_char_type(i));
		}
	}
}

bool Source::isEOF()
{
	fillBufToSize(1); // this will check whether we really are at EOF.
	return eof_flag;
}

char Source::lookAhead(int offset)
{
	fillBufToSize(offset + 1);
	if (case_sensitive) return buf[offset];
	else return tolower(buf[offset]);
}

string Source::read(int length)
{
	fillBufToSize(length);
	string s;
	s.resize(length);
	for (int i=0; i<length; ++i) {
		s[i] = buf.front();
		if (!case_sensitive) s[i] = tolower(s[i]);
		if (s[i]==10) ++line_number;
		buf.pop_front();
	}
	return s;
}

// DiscardLine helper routine:
void DiscardLine(Source &src)
{
	while (!src.isEOF()) {
		string s = src.read(1);
		if (s[0] == 10) return;  // line-end character
	}
}


//
// Lexer
//

struct DFAEdge;

enum SpecialSymbolCodes {
	S_ERROR = -1,
	S_COMMENT_START = -2,
	S_COMMENT_END = -3,
	S_EOF = -4,
	S_WHITESPACE = -5,
	S_COMMENT_LINE = -6
};

int TranslateSymbolCode(const vector<int> &symbol_table, int sym)
{
	// Translate sym according to the "symbol kind"
	// in the symbol table.
	// (In this implementation we don't distinguish between different
	// symbol kinds. Instead we just assign special symbol codes to
	// tokens like EOF or comments. We don't make any special
	// distinction between terminal and nonterminal symbols.)
	switch (symbol_table.at(sym)) {
	case 2:
		return S_WHITESPACE;
	case 3:
		return S_EOF;
	case 4:
		return S_COMMENT_START;
	case 5:
		return S_COMMENT_END;
	case 6:
		return S_COMMENT_LINE;
	case 7:
		return S_ERROR;
	default:
		// Terminal or nonterminal symbol kinds -- No translation needed.
		return sym;
	}
}

struct DFAState {
	int accept_symbol_code;        // SYMBOL CODE for accepted token, or -1
	vector<DFAEdge> dfa_edges;

	bool acceptsTerminal() const { return accept_symbol_code != -1; }
	const DFAEdge * findEdgeContainingChar(char c) const;
};

struct DFAEdge {
	const string * charset;  // Characters in ascending order.
	const DFAState * target_state;

	DFAEdge(const string *c, const DFAState *t) : charset(c), target_state(t) { }
};

struct UnsignedCompare {
    // some compilers treat char as signed
    // so we have to use this to make sure comparisons are done on unsigned ascii codes.
    bool operator()(char lhs, char rhs) const
    {
        return (unsigned char)lhs < (unsigned char)rhs;
    }
};

const DFAEdge * DFAState::findEdgeContainingChar(char c) const
{
	for (vector<DFAEdge>::const_iterator it = dfa_edges.begin(); it != dfa_edges.end();
	++it) {
		if (it->charset && binary_search(it->charset->begin(), it->charset->end(), c, UnsignedCompare())) {
			return &*it;
		}
	}
	return false;
}

// ReadToken: Reads one token from the source;
// returns symbol-code and string representation.
// symbol-code will be -1 if there was a lexical error. 
pair<int,string> ReadToken(Source &src, const DFAState * initial_dfa_state)
{
	int result_symbol = S_ERROR;	
	string result_data;

	if (initial_dfa_state) {
		if (src.isEOF()) {
			result_symbol = S_EOF;
		} else {
			const DFAState * state = initial_dfa_state;
			bool done = false;
			int length = 0;
			
			const DFAState *accept_state = 0;
			int accept_length = 0;
		
			while (!done) {
				
				if (state->acceptsTerminal()) {
					accept_state = state;
					accept_length = length;
				}

				const DFAEdge *edge = state->findEdgeContainingChar(src.lookAhead(length));
				if (edge) {
					state = edge->target_state;
					++length;
				} else {
					if (accept_state) {
						result_symbol = accept_state->accept_symbol_code;
						result_data = src.read(length);
					} else {
						result_symbol = S_ERROR;
						result_data = src.read(1);
					}
					done = true;
				}
			}
		}
	}
	return make_pair(result_symbol, result_data);
}


//
// Parser
//

struct Rule {
	int index;
	int head_symbol;
	int symbol_count;  // no. of stack entries to pop while reducing this rule.
};

enum ActionType {
	A_SHIFT = 1,
	A_REDUCE,
	A_GOTO,
	A_ACCEPT
};

struct LALRState;

struct LALRAction {
	ActionType action_type; // shift (uses target_state), reduce (uses reduce_rule)
							// or accept
	int symbol_code;
	union {
		const Rule *reduce_rule;
		const LALRState *target_state;
	};
};

struct LALRState {
	vector<LALRAction> actions;

	const LALRAction * getActionFor(int symbol_code) const;
};

const LALRAction * LALRState::getActionFor(int symbol_code) const
{
	for (vector<LALRAction>::const_iterator it = actions.begin(); it != actions.end(); ++it) {
		if (symbol_code == it->symbol_code) {
			return &*it;
		}
	}
	return 0;
}

enum ParseResult {
	P_SHIFT,
	P_REDUCE,
	P_ACCEPT,
	P_SYNTAX_ERROR,
	P_INTERNAL_ERROR
};

// ParseToken: Analyses one token; returns a ParseResult, and also
// updates the parse stack ("states") appropriately. Also (in the case
// of P_REDUCE) returns a rule index into "reduction".
ParseResult ParseToken(int symbol_code, const string &symbol_data,
					   stack<const LALRState *> &states,
					   int &reduction)
{
	if (states.empty() || !states.top()) return P_INTERNAL_ERROR;
	
	const LALRAction *action = states.top()->getActionFor(symbol_code);

	if (action) {
		switch (action->action_type) {
		case A_ACCEPT:
			return P_ACCEPT;
		case A_SHIFT:
			states.push(action->target_state);
			return P_SHIFT;
		case A_REDUCE:
			{
				// Pop all the stored states (as indicated by the reduction rule). 
				for (int i=0; i<action->reduce_rule->symbol_count; ++i) {
					states.pop();
				}

				// Invoke the appropriate "goto" rule (looked up from the state on the top of the stack). 
				reduction = action->reduce_rule->index;
				const LALRAction *new_action = states.top()->getActionFor(action->reduce_rule->head_symbol);

				// Push the new state onto the stack (this makes it the new current state).
				states.push(new_action->target_state);
				
				// Return result
				return P_REDUCE;
			}
		default:
			return P_INTERNAL_ERROR;
		}
	} else {
		return P_SYNTAX_ERROR;
	}
}



//
// GoldParser Class:
//


// pimpl
struct GPImpl {
	// The CGT file contents:
	// Symbol_table is cleared immediately after loading the file. The
	// other tables are kept intact for the lifetime of the
	// GoldParser.
	vector<int> symbol_table; // This is used to store GOLD's "symbol kinds". 
	vector<string> character_set_table;
	vector<Rule> rule_table;
	vector<DFAState> dfa_state_table;
	vector<LALRState> lalr_state_table;
	bool case_sensitive;
	const DFAState *initial_dfa;
	const LALRState *initial_lalr;
	
	// The parser state:
	scoped_ptr<Source> src;
	stack<const LALRState *> states;
	bool token_available;
	pair<int,string> token;
	int reduction;
	int comment_level;
};


// CGT Loading:

string ReadUnicodeString(const char *&p, const char *q)
{
	// Actually this doesn't really read unicode... we just ignore every 2nd byte :)

	// note that p is passed by reference
	
	string result;
	while (p < q) {
		if (*p == 0) {
			p += 2;
			break; // NUL terminator.
		}
		result += *p;
		p += 2;
	}
	return result;
}

char ReadChar(const char *&p, const char *q)
{
	if (p < q) return *p++;
	else return 0;
}

int ReadUshort(const char *&p, const char *q)
{
	if (p < q) {
		int lo = static_cast<unsigned int>(*p++);
		int hi = static_cast<unsigned int>(*p++);
		return static_cast<int>(hi*256+lo);
	} else {
		return 0;
	}
}

char ReadByteEntry(const char *&p, const char *q)
{
	if (p+2 > q) throw BadCGT();
	char b = *p++;
	if (b != 'b') throw BadCGT();
	return *p++;
}

bool ReadBooleanEntry(const char *&p, const char *q)
{
	if (p+2 > q) throw BadCGT();
	char B = *p++;
	if (B != 'B') throw BadCGT();
	B = *p++;
	if (B==0) return false;
	else if (B==1) return true;
	else throw BadCGT();
}

int ReadIntegerEntry(const char *&p, const char *q)
{
	if (p+2 > q) throw BadCGT();
	char I = *p++;
	if (I != 'I') throw BadCGT();
	int res = 0;
	res += static_cast<unsigned char>(*p++);
	res += 256*static_cast<unsigned char>(*p++);
	if (res > 32767) res = 65536 - res; // result should be signed
	return res;
}
	
string ReadStringEntry(const char *&p, const char *q)
{
	if (p+2 > q) throw BadCGT();
	char S = *p++;
	if (S != 'S') throw BadCGT();
	return ReadUnicodeString(p, q);
}

void ReadEmptyEntry(const char *&p, const char *q)
{
	if (p+1 > q) throw BadCGT();
	char E = *p++;
	if (E != 'E') throw BadCGT();
}

GoldParser::GoldParser(const char *p, const char *q)
	 : pimpl(new GPImpl)
{
	pimpl->case_sensitive = false;
	pimpl->initial_dfa = 0;
	pimpl->initial_lalr = 0;
	
	// Header
	string header = ReadUnicodeString(p,q);
	if (header != "GOLD Parser Tables/v1.0") throw BadCGT();

	// Read each record in turn.
	while (p < q) {
		readRecord(p,q,pimpl->initial_dfa,pimpl->initial_lalr);
	}

	// Now we can clear symbol table
	// (Use swap trick)
	vector<int> empty_symbol_table;
	pimpl->symbol_table.swap(empty_symbol_table);
}

void GoldParser::readRecord(const char *&p, const char *q,
							  const DFAState *& initial_dfa_state,
							  const LALRState *& initial_lalr_state)
{
	// Type of record -- should be 'M'
	char rtype = ReadChar(p,q);
	if (rtype != 'M') throw BadCGT();
	
	// Number of entries in this record
	int nentries = ReadUshort(p,q);
	if (nentries < 1) throw BadCGT();

	// First entry tells us what kind of record this is 
	rtype = ReadByteEntry(p,q);

	switch (rtype) {
	case 'P':
		// Parameters
		{
			ReadStringEntry(p,q);  // name
			ReadStringEntry(p,q);  // version
			ReadStringEntry(p,q);  // author
			ReadStringEntry(p,q);  // about
			pimpl->case_sensitive = ReadBooleanEntry(p,q);
			ReadIntegerEntry(p,q);  // start symbol (not needed here)
		}
		break;
	case 'T':
		// Table Counts
		{
			pimpl->symbol_table.resize(ReadIntegerEntry(p,q));
			pimpl->character_set_table.resize(ReadIntegerEntry(p,q));
			pimpl->rule_table.resize(ReadIntegerEntry(p,q));
			pimpl->dfa_state_table.resize(ReadIntegerEntry(p,q));
			pimpl->lalr_state_table.resize(ReadIntegerEntry(p,q));
		}
		break;
	case 'C':
		// Character Set Table Entry
		{
			int i = ReadIntegerEntry(p,q);
			pimpl->character_set_table.at(i) = ReadStringEntry(p,q);
		}
		break;
	case 'S':
		// Symbol Table Entry
		{
			int i = ReadIntegerEntry(p,q);  // index
			ReadStringEntry(p,q); // symbol name (ignored)
			pimpl->symbol_table.at(i) = ReadIntegerEntry(p,q); // "token type" for the symbol.
		}
		break;
	case 'R':
		// Rule Table Entry.
		{
			Rule rule;
			rule.index = ReadIntegerEntry(p,q); // index
			rule.head_symbol = ReadIntegerEntry(p,q); // non-terminal symbol.
			ReadEmptyEntry(p,q);  // reserved
			rule.symbol_count = 0;
			for (int i=0; i<nentries-4; ++i) {
				ReadIntegerEntry(p,q);  // symbol.
				++rule.symbol_count;
			}
			pimpl->rule_table.at(rule.index) = rule;
		}
		break;
	case 'I':
		// Initial State Record
		initial_dfa_state = &pimpl->dfa_state_table.at(ReadIntegerEntry(p,q));
		initial_lalr_state = &pimpl->lalr_state_table.at(ReadIntegerEntry(p,q));
		break;
	case 'D':
		// DFA State
		{
			int idx = ReadIntegerEntry(p,q); // index
			bool accept_state = ReadBooleanEntry(p,q);
			int accept_symbol_code = ReadIntegerEntry(p,q);
			ReadEmptyEntry(p,q); // reserved
			if (accept_state) {
				accept_symbol_code = TranslateSymbolCode(pimpl->symbol_table, accept_symbol_code);
				pimpl->dfa_state_table.at(idx).accept_symbol_code = accept_symbol_code;
			} else {
				pimpl->dfa_state_table.at(idx).accept_symbol_code = S_ERROR;
			}
			for (int i=0; i<(nentries-5)/3; ++i) {
				int charset_idx = ReadIntegerEntry(p,q);
				int target_state = ReadIntegerEntry(p,q);
				ReadEmptyEntry(p,q); // reserved
				pimpl->dfa_state_table.at(idx).dfa_edges
					.push_back(DFAEdge(&pimpl->character_set_table.at(charset_idx),
									   &pimpl->dfa_state_table.at(target_state)));
			}
		}
		break;
	case 'L':
		// LALR State
		{
			int idx = ReadIntegerEntry(p,q); // index
			ReadEmptyEntry(p,q); // reserved
			for (int i=0; i<(nentries-3)/4; ++i) {
				LALRAction ac;
				
				const int symbol_code = ReadIntegerEntry(p,q); // symbol code
				ac.symbol_code = TranslateSymbolCode(pimpl->symbol_table, symbol_code);
				ac.action_type = ActionType(ReadIntegerEntry(p,q)); // action type
				int target = ReadIntegerEntry(p,q);  // "target" (either state or rule)
				ReadEmptyEntry(p,q);
				
				switch (ac.action_type) {
				case A_SHIFT:
				case A_GOTO:
					ac.target_state = &pimpl->lalr_state_table.at(target);
					break;
				case A_REDUCE:
					ac.reduce_rule = &pimpl->rule_table.at(target);
					break;
				case A_ACCEPT:
					break;  // Target field is not used here.
				default:
					throw BadCGT(); // This should not happen
				}
				pimpl->lalr_state_table.at(idx).actions.push_back(ac);
			}
		}
		break;
	default:
		throw BadCGT();  // Unknown hunk type
	}
}

//
// Source setup / Reset:
//
void GoldParser::setup(istream &str)
{
	pimpl->src.reset(new Source(str, pimpl->case_sensitive));

	// Set up initial parser state.
	// (e.g. "states" should contain the initial lalr state to begin with.)
	while (!pimpl->states.empty()) pimpl->states.pop();
	pimpl->states.push(pimpl->initial_lalr);
	pimpl->token_available = false;
	pimpl->reduction = -1;
	pimpl->comment_level = 0;
}

//
// The parse function.
//
ParseMessage GoldParser::parse()
{
	if (!pimpl->src) return INTERNAL_ERROR;
	
	while (1) {
		if (!pimpl->token_available) {
			pimpl->token = ReadToken(*pimpl->src, pimpl->initial_dfa);
			pimpl->token_available = true;
		}
		if (pimpl->comment_level > 0) {
			// The system is in COMMENT MODE. The system checks for
			// the Comment Start and Comment End terminals and ignores
			// the rest.
			switch (pimpl->token.first) {
			case S_COMMENT_START:
				++pimpl->comment_level;
				break;
			case S_COMMENT_END:
				--pimpl->comment_level;
				break;
			case S_EOF:
				// At this point, the end of the input was reached
				// while the system is still in comment mode. There is
				// a run away block comment.
				return COMMENT_ERROR;
			}
		} else {
			// The system is in NORMAL MODE.

			// The system is in normal parse mode and tokens are
			// available in the Token-Queue. At this point, the
			// ParseToken() function is called. Note: the Token is NOT
			// removed from the Token-Queue. Whether it is discarded
			// depends on the logic below.

			switch (pimpl->token.first) {
			case S_ERROR:
				return LEXICAL_ERROR;
			case S_COMMENT_START:
				pimpl->comment_level = 1;
				pimpl->token_available = false;
				break;
			case S_COMMENT_END:
				// Comment End token found outside of a block comment
				return COMMENT_ERROR;
			case S_WHITESPACE:
				// Filter out whitespace tokens.
				pimpl->token_available = false;
				break;
			case S_COMMENT_LINE:
				// Filter out line comments
				DiscardLine(*pimpl->src);
				pimpl->token_available = false;
				break;
			default:
				{
					ParseResult pr = ParseToken(pimpl->token.first, pimpl->token.second, pimpl->states, pimpl->reduction);
					switch (pr) {
					case P_ACCEPT:
						return ACCEPT;
					case P_INTERNAL_ERROR:
						return INTERNAL_ERROR;
					case P_REDUCE:
						return REDUCTION;
					case P_SHIFT:
						// After a shift we must remove the token from the input stream
						pimpl->token_available = false;
						return SHIFT;
					case P_SYNTAX_ERROR:
						return SYNTAX_ERROR;
					}
				}
				break;
			}
		}
	}
}

//
// "Get" functions:
//

int GoldParser::getReduction() const
{
	return pimpl->reduction;
}

int GoldParser::getTokenCode() const
{
	return pimpl->token.first;
}

const string & GoldParser::getTokenData() const
{
	return pimpl->token.second;
}

int GoldParser::getLineNumber() const
{
	return pimpl->src? pimpl->src->getLineNumber() : 0;
}


} // namespace GoldParser
