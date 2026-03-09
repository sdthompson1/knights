--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2026.
-- Copyright (C) Kalle Marjola, 1994.
--

-- Merge two tables. Keys in table "b" will override keys in table "a"
-- if there are any duplicates.
function table_merge(a, b)
    local result = {}
    for k, v in pairs(a) do
        result[k] = v
    end
    for k, v in pairs(b) do
        result[k] = v
    end
    return result
end

kts.table_merge = table_merge
