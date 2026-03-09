--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2026.
-- Copyright (C) Kalle Marjola, 1994.
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
            return {key="desc_destroy_book_gems", params={S.gems_needed, S.num_gems}}
        else
            return "desc_destroy_book"
        end

    elseif S.mission == "escape" then
        if S.gems_needed > 0 then
            return {key="desc_escape_gems", params={describe_exit_point(S), S.gems_needed, S.num_gems}}
        else
            return {key="desc_escape", params={describe_exit_point(S)}}
        end

    else
        local what = ""
        if S.mission == "retrieve_book" then
            what = "desc_book"
        elseif S.mission == "retrieve_wand" then
            what = "desc_wand"
        end
        if S.gems_needed > 0 then
            return {key="desc_retrieve_gems",
                    params={what, describe_exit_point(S), S.gems_needed, S.num_gems}}
        else
            return {key="desc_retrieve",
                    params={what, describe_exit_point(S)}}
        end
    end
end

function describe_time(S)
    if S.time > 0 then
        if S.mission == "deathmatch" then
            return {key="desc_game_last_for", params={S.time}, plural=S.time}
        else
            return {key="desc_complete_within", params={S.time}, plural=S.time}
        end
    end
end

function describe_quest(S)

    -- Result is a list of messages, where each message is either a simple string
    -- (localization key), or a table with "key", "params" and optionally "plural".
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
