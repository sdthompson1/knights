/*
 * localization.hpp
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

#ifndef LOCALIZATION_HPP
#define LOCALIZATION_HPP

#include "player_id.hpp"

#include "core/utf8string.hpp"

#include <functional>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Coercri {
    class UTF8String;
}

// A localization string key
class LocalKey {
public:
    LocalKey() {}  // construct "null" key
    explicit LocalKey(const char *k) : key(k) {}
    explicit LocalKey(const std::string &k) : key(k) {}

    bool operator==(const LocalKey &other) const { return key == other.key; }
    bool operator!=(const LocalKey &other) const { return key != other.key; }
    bool operator<(const LocalKey &other) const { return key < other.key; }

    const std::string& getKey() const { return key; }

private:
    std::string key;
};

namespace std {
    template<>
    struct hash<LocalKey> {
        size_t operator()(const LocalKey& key) const {
            return hash<string>()(key.getKey());
        }
    };
}

// Flexible parameter: can be a LocalKey, a PlayerID, or an arbitrary string
class LocalParam {
public:
    enum class Type {
        LOCAL_KEY,
        PLAYER_ID,
        STRING,
        INTEGER
    };

    explicit LocalParam(const LocalKey &lk);
    explicit LocalParam(const PlayerID &id);
    explicit LocalParam(const Coercri::UTF8String &str);
    explicit LocalParam(int value);

    Type getType() const { return type; }
    const LocalKey& getLocalKey() const { return local_key; }
    const PlayerID& getPlayerID() const { return player_id; }
    const Coercri::UTF8String& getString() const { return string_value; }
    int getInteger() const { return int_value; }

    bool operator==(const LocalParam &other) const;
    bool operator!=(const LocalParam &other) const;

private:
    Type type;
    LocalKey local_key;
    PlayerID player_id;
    Coercri::UTF8String string_value;
    int int_value;
};


// Localization string lookup class
class Localization {
public:
    Localization() {
        // Default name lookup just returns the player id unmodified
        name_lookup = [](const PlayerID &id) { return Coercri::UTF8String::fromUTF8Safe(id.asString()); };
    }

    // Read strings from a file
    void readStrings(std::istream &file);

    // Set a player name lookup function
    void setPlayerNameLookup(std::function<Coercri::UTF8String(const PlayerID &)> func) {
        name_lookup = func;
    }

    // Get a plain string
    const Coercri::UTF8String & get(const LocalKey &key) const;

    // Get a string with another LocalKey as parameter
    Coercri::UTF8String get(const LocalKey &key, const LocalKey &param) const;

    // Get a string with an arbitrary UTF8String as parameter
    Coercri::UTF8String get(const LocalKey &key, const Coercri::UTF8String &param) const;

    // Get a string with a list of arbitrary parameters
    Coercri::UTF8String get(const LocalKey &key, const std::vector<LocalParam> &params) const;

    // Pluralize key by appending [one], [other] or a similar string
    // (If count < 0, this returns the key unchanged)
    LocalKey pluralize(const LocalKey &key, int count) const;

private:
    static Coercri::UTF8String substituteParameters(const Coercri::UTF8String& template_str, const std::vector<Coercri::UTF8String>& params);
    static Coercri::UTF8String buildList(const std::vector<Coercri::UTF8String>& params, size_t start_index);

    std::unordered_map<LocalKey, Coercri::UTF8String> strings;
    std::function<Coercri::UTF8String(const PlayerID &)> name_lookup;
};

#endif
