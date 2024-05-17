--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2024.
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
