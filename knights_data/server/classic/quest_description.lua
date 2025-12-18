--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2025.
-- Copyright (C) Kalle Marjola, 1994.
--
-- Knights is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 2 of the License, or
-- (at your option) any later version.
--
-- Knights is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with Knights.  If not, see <http://www.gnu.org/licenses/>.
--

function describe_exit_point(S)
    return "desc_exit_" .. S.exit
end

function describe_book(S)
    if S.book ~= "none" then
        return "desc_" .. S.book
    end
end

function describe_wand(S)
    if S.wand ~= "none" then
        return "desc_" .. S.wand
    end
end

function describe_mission(S)
    if S.mission == "duel_to_death" then
        return "desc_duel_to_death"

    elseif S.mission == "destroy_book" then
        if S.gems_needed > 0 then
            return {"desc_destroy_book_gems", S.gems_needed, S.num_gems}
        else
            return "desc_destroy_book"
        end

    elseif S.mission == "escape" then
        if S.gems_needed > 0 then
            return {"desc_escape_gems", describe_exit_point(S), S.gems_needed, S.num_gems}
        else
            return {"desc_escape", describe_exit_point(S)}
        end

    else
        local what = ""
        if S.mission == "retrieve_book" then
            what = "desc_book"
        elseif S.mission == "retrieve_wand" then
            what = "desc_wand"
        end
        if S.gems_needed > 0 then
            return {"desc_retrieve_gems", what, describe_exit_point(S), S.gems_needed, S.num_gems}
        else
            return {"desc_retrieve", what, describe_exit_point(S)}
        end
    end
end

function describe_time(S)
    if S.time > 0 then
        if S.mission == "deathmatch" then
            return {"desc_game_last_for", S.time, plural=S.time}
        else
            return {"desc_complete_within", S.time, plural=S.time}
        end
    end
end

function describe_quest(S)

    local result = {}

    if S.quest ~= "custom" and quest_table[S.quest].description ~= nil then
        result = quest_table[S.quest].description(S)

    elseif S.mission == "deathmatch" then
        result = {"desc_deathmatch_1", "desc_deathmatch_2"}

    else
        if S.quest == "custom" then
            table.insert(result, "desc_custom_quest")
        end

        table.insert(result, describe_mission(S))

        local book_desc = describe_book(S)
        if book_desc then
            table.insert(result, book_desc)
        end

        local wand_desc = describe_wand(S)
        if wand_desc then
            table.insert(result, wand_desc)
        end
    end

    local time_desc = describe_time(S)
    if time_desc then
        table.insert(result, time_desc)
    end

    return result
end
