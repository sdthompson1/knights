/*
 * kfile.hpp
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
 * KFile: The 'old' interface to KConfig. This is essentially an
 * adapter class from the interface used by KnightsConfig to the new
 * KConfigLoader interface.
 *
 */

#ifndef KCFG_KFILE_HPP
#define KCFG_KFILE_HPP

#include "kconfig_loader.hpp"

#include <string>
#include <vector>

namespace KConfig {

    using std::string;
    using std::vector;

    class Value;
    
    class KFile {
    public:
        KFile(const string &filename, KConfigSource &file_loader, RandomIntContainer &ric, lua_State *lua_state);

        // KFile works on a stack model; you push values onto the
        // stack using pushSymbol or the list or table functions, and
        // then you use the is*, get* and pop* functions to access
        // values on the stack.
        bool isNone() const;
        bool isInt() const;
        bool isFloat() const;    // includes ints (since these can be converted to floats).
        bool isString() const;
        bool isTable() const;
        bool isList() const;
        bool isDirective() const;  // doesn't include random directives.
        bool isRandom() const;     // random directives, ie Random(anything). doesn't include dice or bin-exprs.
        bool isRandomInt() const;  // Includes dice, binary expressions, and Random(list_of_numbers) directives.
        bool isLua() const;

        bool isStackEmpty() const;
        
        // "get" functions -- get value at top of stack w/o modifying stack.
        // If the value can't be converted then these will throw an exception. 
        int getInt() const;
        float getFloat() const;
        string getString() const;
        string getString(const string &dflt) const;
        const RandomInt * getRandomInt(RandomIntContainer&) const;
        void getLua() const;

        // "pop" functions -- as "get", but also removes top value from the stack.
        // (throws exception on error.)
        void popNone();
        int popInt();
        int popInt(int dflt);
        float popFloat();
        float popFloat(float dflt);
        string popString();
        string popString(const string &dflt);
        const RandomInt * popRandomInt(RandomIntContainer &c);
        const RandomInt * popRandomInt(RandomIntContainer &c, const RandomInt *dflt);
        void popLua();
        void pop();  // pops top value only, doesn't try to return anything.

        // look up something from the symbol table (and push it onto the stack).
        // Throws exception if the symbol was undefined. 
        void pushSymbol(const string &);

        // version which pushes "None" if the symbol was undefined
        void pushSymbolOptional(const string &);

        // List, Table, Directive and Random classes. These have three functions:
        // (i) allow access to a list, table or directive at the top of the stack
        // (ii) automatically pop the object on destruction.
        // (iii) handle error messages nicely.

        friend class List;
        friend class Table;
        friend class Directive;

        class List {
        public:
            // n1,n2,etc are expected no of list elements.
            // n is a "name" for the list (this will be reported in error messages). 
            List(KFile &kf_, const string &n, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1);
            ~List();
            int getSize() const;
            void push(int);  // push element (indices <0 are an error, indices >= getSize() push "None").
        private:
            KFile &kf;
            int stksize;
            const Value *lst; // never null
        };

        class Table {
        public:
            Table(KFile &kf_, const string &n);
            ~Table(); 
            // push: looks up a table key. (keys must be looked up in alphabetical order).
            // if key not found, None will be pushed.
            // if "check" is true then raise an exception for "unexpected" table keys.
            // reset: restarts the alphabetical ordering for "push" from the beginning.
            void push(const string &key, bool check = true);
            void reset();
        private:
            KFile &kf;
            int stksize;
            const Value *tab;  // never null
            string last_accessed_key;
        };

        class Directive {
        public:
            Directive(KFile &kf_, const string &n);
            ~Directive();
            void pushArgList(); // push all arguments as a list.
            string getName() const { return directive_name; }
        private:
            KFile &kf;
            int stksize;
            const Value *arglist;
            string directive_name;
        };
        
        class Random {
        public:
            Random(KFile &kf_, const string &n);
            ~Random();
            int getSize() const;  // no of args in the argument list.
            int push(int index);  // returns the "weight" associated with that item.
        private:
            KFile &kf;
            int stksize;
            const Value *arglist;
        };

        // returns Value pointer for the top of the stack (or 0 for "None")
        // (throws exception if stack is empty)
        const Value* getTop() const;

        //
        // Error functions (these throw an exception):-
        //
        
        // found unexpected table key "what"
        void errUnexpectedTableKey(const string &what) const;
        
        // the top item on the stack has the wrong type. "what" should
        // be set to the expected type. The error message is of the
        // form "THING: expected WHAT, found BLAH".
        void errExpected(const string &what) const;
        
        // a general error message
        void error(const string &msg) const;

    private:
        template<class T> T get0(T (Value::*pfn)()const ) const;
        template<class T, class S> T get1R(T (Value::*pfn)(S&)const, S& param) const;
        bool checkIsDirective(bool require_random) const;
        void addTraceback(KConfigError &kce) const;
        
    private:
        vector<const Value *> stk;
        KConfigLoader kcfg;
    };

}

#endif
