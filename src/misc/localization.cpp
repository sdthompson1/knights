/*
 * localization.cpp
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

#include "misc.hpp"

#include "include_lua.hpp"
#include "localization.hpp"
#include "player_id.hpp"

#include <sstream>
#include <iostream>

LocalParam::LocalParam(const LocalKey &lk) : type(Type::LOCAL_KEY), local_key(lk) {}

LocalParam::LocalParam(const PlayerID &id) : type(Type::PLAYER_ID), player_id(id) {}

LocalParam::LocalParam(const Coercri::UTF8String &str) : type(Type::STRING), string_value(str) {}

LocalParam::LocalParam(int value) : type(Type::INTEGER), int_value(value) {}

bool LocalParam::operator==(const LocalParam &other) const
{
    if (type != other.type) return false;
    switch (type) {
        case Type::LOCAL_KEY: return local_key == other.local_key;
        case Type::PLAYER_ID: return player_id == other.player_id;
        case Type::STRING: return string_value == other.string_value;
        case Type::INTEGER: return int_value == other.int_value;
    }
    return false;
}

bool LocalParam::operator!=(const LocalParam &other) const
{
    return !(*this == other);
}

LocalMsg PopLocalMsgFromLua(lua_State *lua)
{
    LocalMsg msg;

    if (lua_isstring(lua, -1)) {
        // Found a plain string key
        msg.key = LocalKey(lua_tostring(lua, -1));
        lua_pop(lua, 1);
        return msg;
    }

    if (!lua_istable(lua, -1)) {
        lua_pushstring(lua, "incorrect message format: should be string or table");
        lua_error(lua);
    }

    // Read the key
    lua_getfield(lua, -1, "key");
    if (!lua_isstring(lua, -1)) {
        lua_pushstring(lua, "message table must have a string 'key' field");
        lua_error(lua);
    }
    msg.key = LocalKey(lua_tostring(lua, -1));
    lua_pop(lua, 1);

    // Read the params
    lua_getfield(lua, -1, "params");
    if (!lua_isnil(lua, -1)) {
        if (!lua_istable(lua, -1)) {
            lua_pushstring(lua, "message 'params' field must be a table");
            lua_error(lua);
        }

        lua_len(lua, -1);
        size_t len = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        for (size_t i = 1; i <= len; ++i) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, -2);

            if (lua_isnumber(lua, -1)) {
                msg.params.push_back(LocalParam(static_cast<int>(lua_tointeger(lua, -1))));
            } else if (lua_isstring(lua, -1)) {
                msg.params.push_back(LocalParam(LocalKey(lua_tostring(lua, -1))));
            } else {
                lua_pushstring(lua, "message params must be strings or integers");
                lua_error(lua);
            }

            lua_pop(lua, 1);
        }
    }
    lua_pop(lua, 1);

    // Read the plural value, if present
    lua_getfield(lua, -1, "plural");
    if (!lua_isnil(lua, -1)) {
        if (!lua_isnumber(lua, -1)) {
            lua_pushstring(lua, "message 'plural' field must be a number");
            lua_error(lua);
        }
        msg.count = static_cast<int>(lua_tointeger(lua, -1));
    }
    lua_pop(lua, 1);

    // Pop the table itself
    lua_pop(lua, 1);

    return msg;
}

void Localization::readStrings(std::istream &file,
                               std::function<Coercri::UTF8String (const Coercri::UTF8String &)> filter_func)
{
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // skip empty lines and comments

        std::istringstream iss(line);
        std::string key;
        if (iss >> key) {
            // Skip whitespace after the key
            while (iss.peek() == ' ' || iss.peek() == '\t') {
                iss.get();
            }

            // Rest of line is the message
            std::string message;
            if (std::getline(iss, message)) {
                // Trim trailing whitespace
                size_t end = message.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    message = message.substr(0, end + 1);
                } else {
                    message.clear();
                }

                LocalKey local_key(key);

                if (strings.find(local_key) != strings.end()) {
                    throw std::runtime_error("Duplicate localization key: " + key);
                }

                strings[local_key] = filter_func(Coercri::UTF8String::fromUTF8(message));
            }
        }
    }
}

Coercri::UTF8String Localization::substituteParameters(const Coercri::UTF8String& template_str, const std::vector<Coercri::UTF8String>& params)
{
    const std::string& input = template_str.asUTF8();
    std::string result;
    result.reserve(input.size() + params.size() * 10); // rough estimate to avoid reallocations
    
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '{') {
            size_t start = i + 1;
            size_t end = start;
            
            // Find the closing brace and parse the number (and optional ...)
            while (end < input.size() && input[end] >= '0' && input[end] <= '9') {
                ++end;
            }
            
            // Check for ... after the number
            bool is_list = false;
            if (end + 2 < input.size() && input.substr(end, 3) == "...") {
                is_list = true;
                end += 3;
            }
            
            if (end < input.size() && input[end] == '}' && end > start) {
                // Parse the parameter index
                int param_index = 0;
                size_t num_end = is_list ? end - 3 : end;
                for (size_t j = start; j < num_end; ++j) {
                    param_index = param_index * 10 + (input[j] - '0');
                }
                
                // Substitute if valid index
                if (param_index >= 0 && param_index < static_cast<int>(params.size())) {
                    if (is_list) {
                        result += buildList(params, param_index).asUTF8();
                    } else {
                        result += params[param_index].asUTF8();
                    }
                    i = end; // Skip past the closing brace
                } else {
                    result += input[i]; // Keep the '{' if invalid placeholder
                }
            } else {
                result += input[i]; // Keep the '{' if not a valid placeholder
            }
        } else {
            result += input[i];
        }
    }
    
    return Coercri::UTF8String::fromUTF8(result);
}

Coercri::UTF8String Localization::buildList(const std::vector<Coercri::UTF8String>& params, size_t start_index)
{
    if (start_index >= params.size()) {
        return Coercri::UTF8String::fromUTF8("");
    }
    
    size_t count = params.size() - start_index;
    
    if (count == 1) {
        return params[start_index];
    } else {
        std::string result;
        for (size_t i = start_index; i < params.size(); ++i) {
            if (i > start_index) {
                if (i == params.size() - 1) {
                    result += " and ";
                } else {
                    result += ", ";
                }
            }
            result += params[i].asUTF8();
        }
        return Coercri::UTF8String::fromUTF8(result);
    }
}

Coercri::UTF8String Localization::get(const LocalKey &key) const
{
    // Look for the key in the strings table
    auto it = strings.find(key);
    if (it != strings.end()) {
        return it->second;
    }

    // Empty key is a special case - displays as an empty string
    if (key == LocalKey()) {
        return Coercri::UTF8String();
    }

    // Otherwise, it is a missing string
    return Coercri::UTF8String::fromUTF8Safe("Missing string [" + key.getKey() + "]");
}

Coercri::UTF8String Localization::get(const LocalKey &key, const LocalKey &param) const
{
    const Coercri::UTF8String& template_str = get(key);
    const Coercri::UTF8String& param_str = get(param);

    std::vector<Coercri::UTF8String> params = { param_str };
    return substituteParameters(template_str, params);
}

Coercri::UTF8String Localization::get(const LocalKey &key, const Coercri::UTF8String &param) const
{
    const Coercri::UTF8String& template_str = get(key);

    std::vector<Coercri::UTF8String> params = { param };
    return substituteParameters(template_str, params);
}

Coercri::UTF8String Localization::get(const LocalKey &key, const std::vector<LocalParam> &params) const
{
    const Coercri::UTF8String& template_str = get(key);
    
    std::vector<Coercri::UTF8String> converted_params;
    converted_params.reserve(params.size());
    
    for (const LocalParam& param : params) {
        switch (param.getType()) {
        case LocalParam::Type::LOCAL_KEY:
            converted_params.push_back(get(param.getLocalKey()));
            break;
        case LocalParam::Type::PLAYER_ID:
            converted_params.push_back(name_lookup(param.getPlayerID()));
            break;
        case LocalParam::Type::STRING:
            converted_params.push_back(param.getString());
            break;
        case LocalParam::Type::INTEGER:
            converted_params.push_back(Coercri::UTF8String::fromUTF8(std::to_string(param.getInteger())));
            break;
        }
    }
    
    return substituteParameters(template_str, converted_params);
}

LocalKey Localization::pluralize(const LocalKey &key, int count) const
{
    // This logic works for English - for other languages, different logic may be needed
    if (count < 0) {
        return key;
    } else if (count == 1) {
        return LocalKey(key.getKey() + "[one]");
    } else {
        return LocalKey(key.getKey() + "[other]");
    }
}
