/*
 * kfile.cpp
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

#include "misc.hpp"

#include "kfile.hpp"

#include <sstream>
using namespace std;

namespace KConfig {

    KFile::KFile(const string &filename, KConfigSource &file_loader, RandomIntContainer &ric, lua_State *lua_state)
        : kcfg(filename, file_loader, ric, lua_state)
{ }
    
    
const Value * KFile::getTop() const
{
    if (stk.empty()) {
        error("stack underflow"); // throws
        return 0; // we never reach here
    } else {
        const Value * result = stk.back();
        // Ensure we return the actual target (as opposed to a symbol pointing to it)
        // This means that we can assume that the result of getTop() is a "unique" pointer
        // to the given value.
        if (result) return &(result->getTarget());
        else return 0;
    }
}

template<class T>
T KFile::get0(T (Value::*pfn)()const) const
{
    try {
        const Value *val = getTop();
        if (!val) throw KConfigError("Null value accessed");
        return (val->*pfn)();
    } catch (KConfigError &kc) {
        addTraceback(kc);
        throw kc;
    }
}

template<class T, class S>
T KFile::get1R(T (Value::*pfn)(S&)const, S& param) const
{
    try {
        const Value *val = getTop();
        if (!val) throw KConfigError("Null value accessed");
        return (val->*pfn)(param);
    } catch (KConfigError &kc) {
        addTraceback(kc);
        throw kc;
    }
}

bool KFile::isNone() const { return getTop() == 0; }
bool KFile::isInt() const { return !isNone() && get0(&Value::isInt); }
bool KFile::isFloat() const { return !isNone() && get0(&Value::isFloat); }
bool KFile::isString() const { return !isNone() && get0(&Value::isString); }
bool KFile::isTable() const { return !isNone() && get0(&Value::isTable); }
bool KFile::isList() const { return !isNone() && get0(&Value::isList); }
bool KFile::isDirective() const { return !isNone() && checkIsDirective(false); }
bool KFile::isRandom() const { return !isNone() && checkIsDirective(true); }
bool KFile::isRandomInt() const { return !isNone() && get0(&Value::isRandomInt); }
bool KFile::isLua() const { return !isNone() && get0(&Value::isLua); }

bool KFile::isStackEmpty() const { return stk.empty(); }

bool KFile::checkIsDirective(bool require_random) const
{
    bool dir = !isNone() && get0(&Value::isDirective);
    if (!dir) return false;
    pair<string,const Value*> p = getTop()->getDirective();
    bool is_random = (p.first=="Random");
    return (require_random && is_random || !require_random && !is_random);
}

int KFile::getInt() const { return get0(&Value::getInt); }
float KFile::getFloat() const { return get0(&Value::getFloat); }
string KFile::getString() const { return get0(&Value::getString); }
string KFile::getString(const string &dflt) const { if (isString()) return getString(); else return dflt; }
const RandomInt * KFile::getRandomInt(RandomIntContainer &m) const { return get1R(&Value::getRandomInt, m); }
void KFile::getLua() const { return get0(&Value::getLua); }
    
void KFile::popNone()
{
    if (!isNone()) errExpected("'None'");
    stk.pop_back();
}

int KFile::popInt()
{
    int result = getInt();
    stk.pop_back();
    return result;
}

int KFile::popInt(int dflt) {
    if (isInt()) {
        return popInt();
    } else {
        return dflt;
    }
}

float KFile::popFloat()
{
    float result = getTop()->getFloat();
    stk.pop_back();
    return result;
}

float KFile::popFloat(float dflt) {
    if (isFloat()) {
        return popFloat();
    } else {
        return dflt;
    }
}

string KFile::popString()
{
    string result = getTop()->getString();
    stk.pop_back();
    return result;
}

string KFile::popString(const string &dflt)
{
    if (isString()) {
        return popString();
    } else {
        return dflt;
    }
}

const RandomInt * KFile::popRandomInt(RandomIntContainer &ric)
{
    const RandomInt *result = getRandomInt(ric);
    stk.pop_back();
    return result;
}

const RandomInt * KFile::popRandomInt(RandomIntContainer &ric, const RandomInt *dflt)
{
    if (isRandomInt()) {
        return popRandomInt(ric);
    } else {
        return dflt;
    }
}

void KFile::popLua()
{
    getLua();
    pop();
}

void KFile::pop()
{
    getTop(); // check stack is non-empty
    stk.pop_back();
}

void KFile::pushSymbol(const string &symbol_name)
{
    stk.push_back(&kcfg.get(symbol_name));
}

void KFile::pushSymbolOptional(const string &symbol_name)
{
    stk.push_back(kcfg.getOptional(symbol_name));
}

KFile::List::List(KFile &kf_, const string &n, int n1, int n2, int n3, int n4, int n5)
    : kf(kf_), stksize(kf_.stk.size())
{
    if (!kf.isList()) {
        kf.errExpected(n);
    } else {
        bool good[5];
        size_t sz = kf.getTop()->getListSize();
        good[0] = n1 < 0 || n1 == sz;
        good[1] = n2 >= 0 && n2 == sz;
        good[2] = n3 >= 0 && n3 == sz;
        good[3] = n4 >= 0 && n4 == sz;
        good[4] = n5 >= 0 && n5 == sz;
        // If at least one good, then pass
        // If everybody is bad, then fail
        if (!good[0] && !good[1] && !good[2] && !good[3] && !good[4]) {
            ostringstream ost;
            ost << "list has wrong size, expected ";
            ost << n1;
            if (n2 >= 0) ost << " or " << n2;
            if (n3 >= 0) ost << " or " << n3;
            if (n4 >= 0) ost << " or " << n4;
            if (n5 >= 0) ost << " or " << n5;
            ost << " entries, found " << sz;
            kf.error(ost.str());
        } else {
            lst = kf.getTop();
        }
    }
}

KFile::List::~List()
{
    if (kf.stk.size() >= stksize) kf.stk.resize(stksize-1);
}

int KFile::List::getSize() const
{
    return lst->getListSize();
}

void KFile::List::push(int index)
{
    if (index >= lst->getListSize()) {
        kf.stk.push_back(0); // push "None"
    } else if (index < 0) {
        kf.error("negative index for List");
    } else {
        kf.stk.push_back(&lst->getList(index));
    }
}

KFile::Table::Table(KFile &kf_, const string &n)
    : kf(kf_), stksize(kf_.stk.size())
{
    if (!kf.isTable()) {
        kf.errExpected(n);
    } else {
        tab = kf.getTop();
    }
}

KFile::Table::~Table()
{
    // Check if all entries in the table were used. (But only if there
    // is not already some other exception going through.)
    if (!uncaught_exception()) {
        string found_key = tab->getTableNext(last_accessed_key);
        if (!found_key.empty()) {
            kf.errUnexpectedTableKey(found_key);
        }
    }
    if (kf.stk.size() >= stksize) kf.stk.resize(stksize-1);
}

void KFile::Table::push(const string &expected_key, bool check)
{
    if (check) {
        string found_key = tab->getTableNext(last_accessed_key);
        if (found_key != "" && found_key < expected_key) {
            kf.errUnexpectedTableKey(found_key);
        }
    }
    
    kf.stk.push_back(tab->getTableOptional(expected_key)); // pushes "None" if the key was not found.
    if (kf.stk.back()!=0) last_accessed_key = expected_key; // If it was successful then update "last_accessed_key".
}

void KFile::Table::reset()
{
    last_accessed_key = string();
}

KFile::Directive::Directive(KFile &kf_, const string &n)
    : kf(kf_), stksize(kf_.stk.size())
{
    if (!kf.isDirective()) {
        kf.errExpected(n);
    } else {
        pair<string,const Value *> p = kf.getTop()->getDirective();
        directive_name = p.first;
        arglist = p.second;
    }
}

KFile::Directive::~Directive()
{
    if (kf.stk.size() >= stksize) kf.stk.resize(stksize-1);
}

void KFile::Directive::pushArgList()
{
    kf.stk.push_back(arglist);
}

KFile::Random::Random(KFile &kf_, const string &n)
    : kf(kf_), stksize(kf_.stk.size())
{
    if (!kf.isRandom()) {
        kf.errExpected(n);
    } else {
        pair<string,const Value *> p = kf.getTop()->getDirective();
        arglist = p.second;
    }
}

KFile::Random::~Random()
{
    if (kf.stk.size() >= stksize) kf.stk.resize(stksize-1);
}

int KFile::Random::getSize() const
{
    return arglist->getListSizeP();
}

int KFile::Random::push(int index)
{
    if (index >= arglist->getListSizeP()) {
        kf.stk.push_back(0); // push "None"
        return 0;
    } else if (index < 0) {
        kf.error("negative index for Random"); // throws
        return 0;  // we never reach here
    } else {
        pair<const Value *, int> p = arglist->getListP(index);
        kf.stk.push_back(p.first);
        return p.second;
    }
}
    

void KFile::errUnexpectedTableKey(const string &what) const
{
    ostringstream str;
    str << "found unexpected table entry: " << what;
    error(str.str());
}

void KFile::errExpected(const string &what) const
{
    error("Config file error: expected " + what);
}

void KFile::error(const string &what) const
{
    if (stk.empty()) {
        throw KConfigError(what);
    } else {
        KConfigError kce(what, stk.back());
        addTraceback(kce);
        throw kce;
    }
}

void KFile::addTraceback(KConfigError &kce) const
{
    for (int i=int(stk.size())-2; i>=0; --i) {
        if (stk[i]) {
            kce.addTraceback(*stk[i]);
        }
    }
}

} // namespace KConfig
