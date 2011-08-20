/*
 * kconfig_loader.cpp
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

#include "misc.hpp"

#include "kconfig_loader.hpp"
#include "random_int.hpp"

#include "compiled_grammar_table.inc"

#include "lua.hpp"

#include "boost/scoped_ptr.hpp"
using namespace boost;

#include <sstream>
using namespace std;

namespace KConfig {

//
// KConfigError.
//

KConfigError::KConfigError(const string &m, const Value *fw)
{
    error_msg = m;
    if (fw) {
        error_msg += "\n";
        error_msg += "Traceback:\n";
        addTraceback(*fw);
    }
}

void KConfigError::addTraceback(const Value &fw)
{
    const Value *tgt = fw.getTargetOptional();
    if (tgt && tgt != &fw) addTraceback(*tgt);
    ostringstream str;
    str << "  file: ";
    str.width(20);
    str << fw.getFilename();
    str << "  line: ";
    str.width(5);
    str << fw.getLineNumber();
    str << "   ";
    str << fw.describe();
    str << "\n";
    error_msg += str.str();
}

//
// RandomIntContainer.
//

RandomIntContainer::~RandomIntContainer()
{
    for (size_t i=0; i<contents.size(); ++i) delete contents[i];
}

void RandomIntContainer::add(const RandomInt *ri)
{
    contents.push_back(ri);
}


//
// Value virtual function defaults
//

int Value::getInt() const { throw KConfigError("integer expected", this); }
float Value::getFloat() const { throw KConfigError("floating point value expected", this); }
string Value::getString() const { throw KConfigError("string expected", this); }
const RandomInt * Value::getRandomInt(RandomIntContainer &) const { throw KConfigError("random integer expected", this); }
const Value & Value::getTable(const string &) const { throw KConfigError("table expected", this); }
const Value * Value::getTableOptional(const string &) const { throw KConfigError("table expected", this); }
string Value::getTableNext(const string &) const { throw KConfigError("table expected", this); }
int Value::getListSize() const { throw KConfigError("list expected", this); }
const Value & Value::getList(int index) const { throw KConfigError("list expected", this); }
int Value::getListSizeP() const { throw KConfigError("list expected", this); }
pair<const Value*, int> Value::getListP(int) const { throw KConfigError("list expected", this); }
pair<string, const Value*> Value::getDirective() const { throw KConfigError("directive expected", this); }
void Value::getLua() const { throw KConfigError("lua value expected", this); }


//
// The concrete Value classes.
//

// SymbolValue -- looks up a symbol in a symbol table. 
class SymbolValue : public Value {
private:
    template<class T>
    T get0( T (Value::*pfn)()const ) const
    {
        // If getVal() throws then we are alright. The exception will refer to this Value, as desired. 
        const Value &val(getVal());
        // If the following get instruction fails then the exception will refer to "val". We
        // have to add on information corresponding to this object.
        try {
            return (val.*pfn)();
        } catch (KConfigError &kc) {
            kc.addTraceback(*this);
            throw kc;
        }
    }

    template<class T, class S>
    T get1( T (Value::*pfn)(S)const, S param) const
    {
        const Value &val(getVal());
        try {
            return (val.*pfn)(param);
        } catch (KConfigError &kc) {
            kc.addTraceback(*this);
            throw kc;
        }
    }
    
    template <class T, class S>
    T get1R( T (Value::*pfn)(S&)const, S& param) const
    {
        const Value &val(getVal());
        try {
            return (val.*pfn)(param);
        } catch (KConfigError &kc) {
            kc.addTraceback(*this);
            throw kc;
        }
    }

public:
    SymbolValue(const string &file, int line, const Value &st, const string &n)
        : Value(file, line), symbol_table(st), name(n) { }
    
public:
    virtual bool isInt() const { return get0(&Value::isInt); }
    virtual bool isFloat() const { return get0(&Value::isFloat); }
    virtual bool isString() const { return get0(&Value::isString); }
    virtual bool isRandomInt() const { return get0(&Value::isRandomInt); }
    virtual bool isTable() const { return get0(&Value::isTable); }
    virtual bool isList() const { return get0(&Value::isList); }
    virtual bool isDirective() const { return get0(&Value::isDirective); }

    virtual int getInt() const { return get0(&Value::getInt); }
    virtual float getFloat() const { return get0(&Value::getFloat); }
    virtual string getString() const { return get0(&Value::getString); }
    virtual const RandomInt * getRandomInt(RandomIntContainer &m) const { return get1R(&Value::getRandomInt, m); }
    virtual const Value & getTable(const string &key) const { return get1R(&Value::getTable, key); }
    virtual const Value * getTableOptional(const string &key) const { return get1R(&Value::getTableOptional, key); }
    virtual string getTableNext(const string &prev_key) const { return get1R(&Value::getTableNext, prev_key); }
    virtual int getListSize() const { return get0(&Value::getListSize); }
    virtual const Value & getList(int index) const { return get1(&Value::getList, index); }
    virtual int getListSizeP() const { return get0(&Value::getListSizeP); }
    virtual pair<const Value *, int> getListP(int index) const { return get1(&Value::getListP, index); }
    virtual pair<string, const Value *> getDirective() const { return get0(&Value::getDirective); }

    virtual string describe() const { return name; }
    
    const string &getName() const { return name; }
    const Value & getTarget() const { return symbol_table.getTable(name); }
    const Value * getTargetOptional() const { return symbol_table.getTableOptional(name); } // returns 0 if the symbol is undefined
    
private:

    const Value &getVal() const
    {
        try {
            return symbol_table.getTable(name);
        } catch (KConfigError &kc) {
            kc.addTraceback(*this);
            throw kc;
        }
    }

private:
    const Value &symbol_table;
    string name;
};


// IntValue, FloatValue etc

class IntValue : public Value {
public:
    explicit IntValue(const string &file, int line, int ii) : Value(file, line), i(ii), ri(0) { }
    virtual bool isInt() const { return true; }
    virtual bool isFloat() const { return true; }
    virtual bool isRandomInt() const { return true; }
    virtual int getInt() const { return i; }
    virtual float getFloat() const { return float(i); }
    virtual const RandomInt * getRandomInt(RandomIntContainer &c) const {
        if (!ri) {
            ri = new RIConstant(i);
            c.add(ri);
        }
        return ri;
    }
    virtual string describe() const { ostringstream s; s<<i; return s.str(); }
private:
    int i;
    mutable const RandomInt *ri;
};

const IntValue zero_value = IntValue("<internal>", 0, 0);

class FloatValue : public Value {
public:
    explicit FloatValue(const string &file, int line, float ff) : Value(file,line), f(ff) { }
    virtual bool isFloat() const { return true; }
    virtual float getFloat() const { return f; }
    virtual string describe() const { ostringstream s; s<<f; return s.str(); }
private:
    float f;
};

class StringValue : public Value {
public:
    explicit StringValue(const string &file, int line, const string &ss) : Value(file,line), s(ss) { }
    virtual bool isString() const { return true; }
    virtual string getString() const { return s; }
    virtual string describe() const { return '"' + s + '"'; }
private:
    string s;
};

// RandomIntValue

class RandomIntValue : public Value {
public:
    // Note: it is assumed that the randomint has already been added to the RI container
    explicit RandomIntValue(const string &file, int line, const RandomInt *ri_) : Value(file,line), ri(ri_) { }
    virtual bool isRandomInt() const { return true; }
    virtual const RandomInt * getRandomInt(RandomIntContainer&) const { return ri; }
    virtual string describe() const { return "<random int>"; }
private:
    const RandomInt *ri;
};

// TableValue

class TableValue : public Value {
public:
    TableValue(const string &file, int line) : Value(file, line) { }
    void add(const string &key, const Value *val) {
        if (val) tbl[key] = val;
    }
    virtual bool isTable() const { return true; }
    virtual const Value & getTable(const string &key) const; 
    virtual const Value * getTableOptional(const string &key) const;
    virtual string getTableNext(const string &prev_key) const;
    virtual string describe() const { return "<table>"; }
private:
    typedef map<string, const Value *> TableType;
    TableType tbl;
};

const Value & TableValue::getTable(const string &key) const
{
    TableType::const_iterator it = tbl.find(key);
    if (it == tbl.end()) throw KConfigError("Lookup of '" + key + "' failed", this);
    else return *it->second;
}

const Value * TableValue::getTableOptional(const string &key) const
{
    TableType::const_iterator it = tbl.find(key);
    if (it == tbl.end()) return 0;
    else return it->second;
}

string TableValue::getTableNext(const string &prev_key) const
{
    if (tbl.empty()) return "";
    if (prev_key=="") return tbl.begin()->first; // first key
    TableType::const_iterator it = tbl.find(prev_key); // locate the prev key
    if (it == tbl.end()) throw KConfigError("bad key in getTableNext", this);
    ++it; // go one forward
    if (it == tbl.end()) return "";
    else return it->first;
}


// Lua Value (refers to some global lua variable)

class LuaValue : public Value {
public:
    LuaValue(const string &file, int line, lua_State *lua, const string &var)
        : Value(file, line), lua_state(lua), varname(var) { }
    virtual bool isLua() const { return true; }
    void getLua() const { lua_getglobal(lua_state, varname.c_str()); }
    virtual string describe() const { return "<lua value>"; }
private:
    lua_State *lua_state;
    string varname;
};


// RootTableValue

class RootTableValue : public TableValue {
public:
    RootTableValue(const string &file, int line, lua_State * lua)
        : TableValue(file, line), lua_state(lua) { }
    virtual ~RootTableValue();
    virtual const Value & getTable(const string &key) const;
    virtual const Value * getTableOptional(const string &key) const;
    virtual string getTableNext(const string &prev_key) const { throw KConfigError("RootTableValue::getTableNext: Not implemented"); }
    virtual string describe() const { return "<root table>"; }
private:
    void operator=(const RootTableValue &);  // not implemented
    RootTableValue(const RootTableValue &);  // not implemented
private:
    lua_State * lua_state;
    typedef std::map<std::string, const LuaValue *> Key2LuaValue;
    mutable Key2LuaValue lua_value_cache;
};

RootTableValue::~RootTableValue()
{
    for (Key2LuaValue::iterator it = lua_value_cache.begin(); it != lua_value_cache.end(); ++it) {
        delete it->second;
    }
}

const Value * RootTableValue::getTableOptional(const string &key) const
{
    const Value * parent_val = TableValue::getTableOptional(key);
    if (parent_val) return parent_val;

    if (lua_state) {
        // see if it is cached
        Key2LuaValue::const_iterator it = lua_value_cache.find(key);
        if (it != lua_value_cache.end()) {
            return it->second;
        }
        
        // see if it exists in the lua global environment
        lua_getglobal(lua_state, key.c_str());
        const bool found = !lua_isnil(lua_state, -1);
        lua_pop(lua_state, 1);

        if (found) {
            // store it in the cache and return it
            LuaValue * new_lua_value = new LuaValue("<lua code>", 0, lua_state, key);
            lua_value_cache.insert(std::make_pair(key, new_lua_value));
            return new_lua_value;
        }
    }

    return 0;
}

const Value & RootTableValue::getTable(const string &key) const
{
    const Value * val = getTableOptional(key);
    if (val) return *val;
    else throw KConfigError("Symbol '" + key + "' is not defined");
}
    

// ListValue

class ListValue : public Value {
public:
    ListValue(const string &file, int line) : Value(file,line), all_one(true) { }
    void add(const Value *val, int count) {
        if (val) {
            lst.push_back(make_pair(val,count));
            if (count != 1) all_one = false;
        }
    }
    virtual bool isList() const { return true; }
    virtual int getListSize() const {
        if (all_one) return lst.size();
        int sz = 0;
        for (size_t i=0; i<lst.size(); ++i) {
            sz += lst[i].second;
        }
        return sz;
    }
    virtual const Value & getList(int index) const {
        if (index < 0) throw KConfigError("list index out of range", this);
        if (all_one) {
            if (index >= lst.size()) throw KConfigError("list index out of range", this);
            else return *(lst[index].first);
        } else {
            for (size_t i=0; i<lst.size(); ++i) {
                index -= lst[i].second;
                if (index < 0) return *(lst[i].first);
            }
            throw KConfigError("list index out of range", this);
        }
    }
    virtual int getListSizeP() const { return lst.size(); }
    virtual pair<const Value*, int> getListP(int index) const {
        if (index < 0 || index >= lst.size()) {
            throw KConfigError("list index out of range", this);
        } else {
            return lst[index];
        }
    }
    virtual string describe() const { return "<list>"; }
private:
    bool all_one;
    vector<pair<const Value*, int> > lst;
};

// DirectiveValue

class DirectiveValue : public Value {
public:
    DirectiveValue(const string &file, int line, const string &n, const Value *plist)
        : Value(file,line), val(n, plist), ri(0) { }
    virtual bool isRandomInt() const { if (ri || val.first=="Random") return true; else return false; }
    virtual bool isDirective() const { return true; }
    pair<string, const Value *> getDirective() const { return val; }
    const RandomInt *getRandomInt(RandomIntContainer &ric) const {
        // "Random" directives can be converted to RandomInts (RILists)
        if (ri) {
            return ri;
        } else if (val.first!="Random") {
            return Value::getRandomInt(ric);
        } else {
            RIList ri2(val.second->getListSizeP());
            for (int i=0; i<val.second->getListSizeP(); ++i) {
                pair<const Value*, int> p = val.second->getListP(i);
                ri2.add(p.first->getRandomInt(ric), p.second); // might throw an exception
            }
            ri = new RIList(ri2);
            ric.add(ri);
            return ri;
        }
    }
    virtual string describe() const { return "<directive>"; }
private:
    pair<string, const Value*> val; // Name and parameter list.
    mutable const RandomInt *ri;
};

// Binary Arithmetic Expressions:
// Note these are not reduced at construction time; reduction is delayed until
// we actually try to read the value. Otherwise, we might try to look up symbols
// that have not been loaded yet :)

enum Op {
    // These are just rule numbers from the grammar.
    OP_ADD = 1,
    OP_SUB = 2,
    OP_MUL = 5,
    OP_DIV = 6,
    OP_AMP = 3
};

class BinExpValue : public Value {
public:
    explicit BinExpValue(const string &file, int line,
                         const Value &lhs_, const Value &rhs_,
                         int op_)
        : Value(file, line), lhs(lhs_), rhs(rhs_), op(op_), ri(0) { }
    virtual bool isInt() const { return lhs.isInt() && rhs.isInt() && op!=OP_AMP; }
    virtual bool isFloat() const { return lhs.isFloat() && rhs.isFloat() && op!=OP_AMP; }
    virtual bool isRandomInt() const { return lhs.isRandomInt() && rhs.isRandomInt() && op!=OP_AMP; }
    virtual bool isTable() const { return lhs.isTable() && rhs.isTable() && op==OP_AMP; }
    virtual int getInt() const {
        int x = lhs.getInt(), y = rhs.getInt();
        switch (op) {
        case OP_ADD: return x+y; break;
        case OP_SUB: return x-y; break;
        case OP_MUL: return x*y; break;
        case OP_DIV: return x/y; break;
        default: throw KConfigError("'&' applied to non-table arguments", this); break;
        }
    }
    virtual float getFloat() const {
        float x = lhs.getFloat(), y = rhs.getFloat();
        switch (op) {
        case OP_ADD: return x+y; break;
        case OP_SUB: return x-y; break;
        case OP_MUL: return x*y; break;
        case OP_DIV: return x/y; break;
        default: throw KConfigError("'&' applied to non-table arguments", this); break;
        }
    }
    virtual const RandomInt * getRandomInt(RandomIntContainer &c) const {
        if (!ri) {
            const RandomInt *x = lhs.getRandomInt(c);
            const RandomInt *y = rhs.getRandomInt(c);
            switch (op) {
            case OP_ADD: ri = new RIAdd(x,y); break;
            case OP_SUB: ri = new RISub(x,y); break;
            case OP_MUL: ri = new RIMul(x,y); break;
            case OP_DIV: ri = new RIDiv(x,y); break;
            default: throw KConfigError("'&' applied to non-table arguments", this); break;
            }
            c.add(ri);
        }
        return ri;
    }
    virtual const Value & getTable(const string &key) const {
        createNewTable();
        return new_table->getTable(key);
    }
    virtual const Value * getTableOptional(const string &key) const {
        createNewTable();
        return new_table->getTableOptional(key);
    }
    string getTableNext(const string &prev_key) const {
        createNewTable();
        return new_table->getTableNext(prev_key);
    }
    virtual string describe() const { return "<expression>"; }
private:
    void createNewTable() const {
        if (new_table) return;
        if (!lhs.isTable() || !rhs.isTable()) {
            throw KConfigError("'&' applied to non-table arguments", this);
        }
        new_table.reset(new TableValue("<internal>",0));
        string k = "";
        while (1) {
            k = lhs.getTableNext(k);
            if (k=="") break;
            new_table->add(k, &lhs.getTable(k));
        }
        k = "";
        while (1) {
            k = rhs.getTableNext(k);
            if (k=="") break;
            new_table->add(k, &rhs.getTable(k));
        }
    }
private:
    const Value &lhs;
    const Value &rhs;
    int op;
    mutable const RandomInt *ri;
    mutable scoped_ptr<TableValue> new_table;
};    


//
// KConfigLoader
//

const Value & KConfigLoader::getRootTable() const
{
    return *root_table;
}

const Value & KConfigLoader::get(const string &s) const
{
    return root_table->getTable(s);
}

const Value * KConfigLoader::getOptional(const string &s) const
{
    return root_table->getTableOptional(s);
}
    
void KConfigLoader::shift(int symbol_code, const string &token, RandomIntContainer &ric)
{
    switch (symbol_code) {
    case 18:
        {
            // Dice
            size_t dpos = token.find_first_of("dD");
            int n;
            if (dpos > 0) n = atoi(token.substr(0,dpos).c_str()); else n=1;
            int d = atoi(token.substr(dpos+1).c_str());
            RandomInt *ri = new RIDice(n, d);
            ric.add(ri);
            stored_values.push_back(new RandomIntValue(file, gp.getLineNumber(), ri));
        }
        break;
    case 19:
        // Float
        stored_values.push_back(new FloatValue(file, gp.getLineNumber(), float(atof(token.c_str()))));
        break;
    case 20:
        // Hex Integer
        stored_values.push_back(new IntValue(file, gp.getLineNumber(), strtol(token.c_str()+1, 0, 16)));
        break;
    case 22:
        // Integer
        stored_values.push_back(new IntValue(file, gp.getLineNumber(), atoi(token.c_str())));
        break;
    case 23:
        // String
        stored_values.push_back(new StringValue(file, gp.getLineNumber(), token.substr(1, token.length()-2)));
        break;
    case 24:
        // Symbol
        stored_values.push_back(new SymbolValue(file, gp.getLineNumber(), *root_table, token));
        break;
    default:
        // No action necessary
        return;
    }
    // If we get to here then something was pushed onto back of stored_values
    // Push it onto the value_stack as well:
    value_stack.push(stored_values.back());
}

void KConfigLoader::reduce(int reduction, RandomIntContainer &ric)
{
    switch (reduction) {
    case 0:
        {
            // Assign
            const Value *target = value_stack.top();
            value_stack.pop();
            ASSERT(dynamic_cast<SymbolValue*>(value_stack.top()));
            const SymbolValue *symbol_name = static_cast<SymbolValue*>(value_stack.top());
            value_stack.pop();
            ASSERT(dynamic_cast<TableValue*>(value_stack.top()));
            TableValue *tbl = static_cast<TableValue*>(value_stack.top());
            if (tbl->getTableOptional(symbol_name->getName())) {
                throw KConfigError("multiple definition", symbol_name);
            } else {
                tbl->add(symbol_name->getName(), target);
            }
        }
        break;
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
        // Binary operations
        {
            const Value *v2 = value_stack.top(); // RHS comes off the stack first
            value_stack.pop();
            const Value *v1 = value_stack.top(); // Then LHS.
            value_stack.pop();
            ASSERT(v1 && v2);
            stored_values.push_back(new BinExpValue(file, gp.getLineNumber(), *v1, *v2, reduction));
            value_stack.push(stored_values.back());
        }
        break;
    case 17:
        // Unary minus
        {
            const Value *operand = value_stack.top();
            value_stack.pop();
            stored_values.push_back(new BinExpValue(file, gp.getLineNumber(), zero_value, *operand, OP_SUB));
            value_stack.push(stored_values.back());
        }
        break;
    case 21:
        // New table
        stored_values.push_back(new TableValue(file, gp.getLineNumber()));
        value_stack.push(stored_values.back());
        break;
    case 24:
        // New list
        stored_values.push_back(new ListValue(file, gp.getLineNumber()));
        value_stack.push(stored_values.back());
        break;
    case 30:
        // Include
        {
            const string filename = value_stack.top()->getString();
            value_stack.pop();
            if (loaded_files.find(filename) == loaded_files.end()) {
                waiting_files.insert(filename);
            }
        }
        break;
    case 31:
    case 32:
        // Add List (31 = Single Item, 32 = Repeated Item)
        {
            int count = 1;
            if (reduction == 32) {
                count = value_stack.top()->getInt();
                value_stack.pop();
            }
            const Value *val = value_stack.top();
            value_stack.pop();
            ASSERT(dynamic_cast<ListValue*>(value_stack.top()));
            ListValue *lst = static_cast<ListValue*>(value_stack.top());
            lst->add(val, count);
        }
        break;
    case 33:
        // Directive
        {
            const Value *args = value_stack.top();
            value_stack.pop();
            ASSERT(dynamic_cast<const SymbolValue*>(value_stack.top()));
            const SymbolValue *dname = static_cast<const SymbolValue*>(value_stack.top());
            value_stack.pop();
            stored_values.push_back(new DirectiveValue(file, gp.getLineNumber(), dname->getName(), args));
            value_stack.push(stored_values.back());
        }
        break;
    }
}

void KConfigLoader::processFile(istream &str, RandomIntContainer &ric)
{
    gp.setup(str);
    while (1) {
        GoldParser::ParseMessage p = gp.parse();
        switch (p) {
        case GoldParser::SHIFT:
            shift(gp.getTokenCode(), gp.getTokenData(), ric);
            break;
        case GoldParser::REDUCTION:
            reduce(gp.getReduction(), ric);
            break;
        case GoldParser::ACCEPT:
            return;
        default:
            // An error has occurred
            string err;
            switch (p) {
            case GoldParser::SYNTAX_ERROR:
                err = "syntax error";
                break;
            case GoldParser::LEXICAL_ERROR:
                err = "lexical error";
                break;
            case GoldParser::COMMENT_ERROR:
                err = "comment error";
                break;
            case GoldParser::INTERNAL_ERROR:
                err = "internal parser error";
                break;
            }
            // Push a dummy value for the current location.
            stored_values.push_back(new SymbolValue(file, gp.getLineNumber(), *root_table, "<error>"));
            throw KConfigError(err, stored_values.back());
        }
    }
}

KConfigLoader::KConfigLoader(const string &filename, KConfigSource &file_loader,
                             RandomIntContainer &ric,
                             lua_State * lua_state)
    : root_table(new RootTableValue("<internal>", 0, lua_state)),
      gp((const char *)compiled_grammar_table,
         (const char *)compiled_grammar_table+sizeof(compiled_grammar_table))
{
    value_stack.push(root_table);
    
    waiting_files.insert(filename);
    while (!waiting_files.empty()) {
        file = *(waiting_files.begin());
        waiting_files.erase(file);
        loaded_files.insert(file);
        shared_ptr<istream> str = file_loader.openFile(file);
        processFile(*str, ric);
    }
}

KConfigLoader::~KConfigLoader()
{
    // We have to delete everything from stored_values:
    for (size_t i=0; i<stored_values.size(); ++i) {
        delete stored_values[i];
    }
    delete root_table;
}


} // namespace KConfig
