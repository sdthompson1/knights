/*
 * kconfig_loader.hpp
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

#ifndef KCONFIG_LOADER_HPP
#define KCONFIG_LOADER_HPP

#include "gold_parser.hpp"

#include "boost/shared_ptr.hpp"

#include <exception>
#include <iosfwd>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>
using namespace std;

struct lua_State;

namespace KConfig {

class RandomInt;
class Value;

class KConfigError : public std::exception {
public:
    KConfigError(const string &msg, const Value *from_where = 0);
    virtual ~KConfigError() throw() { }
    void addTraceback(const Value &from_where);
    const char * what() const throw() { return error_msg.c_str(); }
private:
    string error_msg;
};

// RandomIntContainer holds all of the randomints generated during
// the config loading process. They will all be released when the
// RandomIntContainer is destroyed.
class RandomIntContainer {
public:
    RandomIntContainer() { }
    ~RandomIntContainer();  // deletes all contained RandomInts
    void add(const RandomInt *ri); // transfers ownership of "ri"
private:
    RandomIntContainer(const RandomIntContainer &);//not defined
    void operator=(const RandomIntContainer &);//not defined
    vector<const RandomInt*> contents; 
};

class Value {
public:
    Value(const string &file_, int line_) : file(file_), line(line_) { }
    virtual ~Value() { }
    
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool isRandomInt() const { return false; }
    virtual bool isTable() const { return false; }
    virtual bool isList() const { return false; }
    virtual bool isDirective() const { return false; }
    virtual bool isLua() const { return false; }
    
    virtual int getInt() const;
    virtual float getFloat() const;
    virtual string getString() const;
    virtual const RandomInt * getRandomInt(RandomIntContainer &c) const;
    virtual const Value & getTable(const string &key) const;
    virtual const Value * getTableOptional(const string &key) const;
    virtual string getTableNext(const string &prev_key) const; // returns next key alphabetically after prev_key (or "" if prev_key was the last key). 
    virtual int getListSize() const;
    virtual const Value & getList(int index) const;
    virtual int getListSizeP() const;
    virtual pair<const Value *, int> getListP(int index) const; // includes repeat count. The value will never be NULL
    virtual pair<string, const Value *> getDirective() const;  // returns dir-name and list-of-args. The value will never be NULL
    virtual void getLua() const;  // Pushes the lua value onto the lua stack. Throws if !isLua().

    virtual string describe() const = 0;
    virtual const Value & getTarget() const { return *this; } // Only defined for "symbols"
    virtual const Value * getTargetOptional() const { return this; }  // Ditto
    
    const string &getFilename() const { return file; }
    int getLineNumber() const { return line; }

private:
    string file;
    int line;
};


class RootTableValue;

// Interface for KConfig
class KConfigSource {
public:
    virtual ~KConfigSource() { }
    virtual boost::shared_ptr<istream> openFile(const string &name) = 0;
};

// KConfigLoader -- loads the config file(s).
// A tree of Value objects is constructed (representing the AST
// of the config file) and these can be accessed through getRootTable().
// The Values are held in memory for the lifetime of the KConfig object.
// The RandomInts generated will be stored in the RandomIntContainer, hence
// these can live for longer than the KConfig if necessary. 
class KConfigLoader {
public:
    KConfigLoader(const string &filename, KConfigSource &file_loader, RandomIntContainer &ric,
                  lua_State * lua_state);
    ~KConfigLoader();
    const Value & getRootTable() const;
    const Value & get(const string &s) const;
    const Value * getOptional(const string &s) const;
    
private:
    KConfigLoader(const KConfigLoader &); // not defined
    void operator=(const KConfigLoader &) const;// not defined

private:
    void shift(int symbol_code, const string &token, RandomIntContainer &);
    void reduce(int reduction, RandomIntContainer &);
    void processFile(istream &, RandomIntContainer &);
    
private:
    set<string> loaded_files;
    set<string> waiting_files;
    stack<Value *> value_stack;
    vector<Value *> stored_values;
    RootTableValue * root_table;
    GoldParser::GoldParser gp;
    string file; // file currently being processed.
};

}

#endif
