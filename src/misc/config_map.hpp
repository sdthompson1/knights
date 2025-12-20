/*
 * config_map.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#ifndef CONFIG_MAP_HPP
#define CONFIG_MAP_HPP

#include "my_exceptions.hpp"

#include <map>
#include <string>

struct lua_State;

class BadConfig : public ExceptionBase {
public:
    explicit BadConfig(const std::string &key)
        : ExceptionBase(LocalMsg{LocalKey("config_key_not_found"), {LocalParam(Coercri::UTF8String::fromUTF8Safe(key))}}) {}
};
    
class ConfigMap {
public:
    void setInt(const std::string &key, int val) { ints.insert(std::make_pair(key, val)); }
    void setFloat(const std::string &key, float val) { floats.insert(std::make_pair(key, val)); }
    void setString(const std::string &key, const std::string &val) { strings.insert(std::make_pair(key, val)); }

    int getInt(const std::string &key) const {
        std::map<std::string, int>::const_iterator it = ints.find(key);
        if (it == ints.end()) throw BadConfig(key);
        else return it->second;
    }

    // returns corresponding 'int' if there is no 'float' available
    float getFloat(const std::string &key) const {
        std::map<std::string, float>::const_iterator it = floats.find(key);
        if (it == floats.end()) return float(getInt(key));
        else return it->second;
    }

    const std::string & getString(const std::string &key) const {
        std::map<std::string, std::string>::const_iterator it = strings.find(key);
        if (it == strings.end()) throw BadConfig(key);
        else return it->second;
    }

private:
    std::map<std::string, int> ints;
    std::map<std::string, float> floats;
    std::map<std::string, std::string> strings;
};

// Pop a Lua table from the top of the stack and use it to initialize
// a config map.
void PopConfigMap(lua_State *lua, ConfigMap &cfg_map);

#endif
