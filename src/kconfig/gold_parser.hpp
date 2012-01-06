/*
 * gold_parser.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
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

#ifndef GOLD_PARSER_HPP
#define GOLD_PARSER_HPP

#include "boost/shared_ptr.hpp"
#include <exception>
#include <iosfwd>
#include <string>

namespace GoldParser {

	struct GPImpl; // private
	struct DFAState; // private
	struct LALRState; // private
	
	// ParseMessage:
	// These codes are returned by the GoldParser::parse function
	enum ParseMessage {
		REDUCTION,  // A reduction rule was applied ("Parse stack" should be updated appropriately)
		SHIFT,      // A shift was done (A token should now be added to the "parse stack")
		ACCEPT,     // The end of the input was reached
		LEXICAL_ERROR,  // A lexical error was encountered
		SYNTAX_ERROR,   // A syntax error was encountered
		COMMENT_ERROR,  // Unmatched start/end comment tokens
		INTERNAL_ERROR  // This should not happen :-)
	};

	// BadCGT: This exception gets thrown if the CGT could not be loaded correctly.
	class BadCGT : public std::exception {
	public:
		const char * what() const throw() { return "GOLD parser: bad CGT file"; }
	};

	// The main GoldParser class:
	class GoldParser {
	public:
		// Ctor takes a memory buffer containing the CGT file.
		// p is the first byte, q is the one-past-the-end byte.
		// Throws BadCGT if anything goes wrong.
		GoldParser(const char *p, const char *q);
		
		// Initialize for a new file. This must be called before parsing begins.
		// If called after parsing is underway, then it will reset everything and
		// restart at the beginning of the new file.
		void setup(std::istream &str);
		
		// Parse function.
		// This will return one of the ParseMessage codes from above. Further info
		// can be obtained by using the "get" functions below. 
		ParseMessage parse();
		
		// Use this function to see which rule was used in a REDUCTION:
		int getReduction() const;
		
		// Use these functions to see which token was read (after a SHIFT result):
		int getTokenCode() const;
		const std::string & getTokenData() const;

		// This returns the current line number (useful for error messages):
		int getLineNumber() const;
		
	private:
		// Implementation
		void readRecord(const char *&p, const char *q,
						const DFAState *&init_dfa, const LALRState *&init_lalr);		
		boost::shared_ptr<GPImpl> pimpl;
	};

}

#endif
