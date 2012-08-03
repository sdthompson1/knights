--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2012.
-- Copyright (C) Kalle Marjola, 1994.
--
-- Knights is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
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


-- Define some utility functions for working with the menu.

local M = {}


function M.get_menu_item(id)
   for k,v in ipairs(kts.MENU.items) do
      if v.id == id then
         return v
      end
   end
   error("Menu item '" .. id .. "' does not exist!")
end

function M.get_menu_choice(item_id, choice_id)
   local item = M.get_menu_item(item_id)
   for k,v in ipairs(item.choices) do
      if v.id == choice_id then
         return v
      end
   end
   error("Menu item '" .. item_id .. "' does not have any choice named '" .. choice_id .. "'!")
end

function M.add_menu_item(item)
   table.insert(kts.MENU.items, item) -- Add it at the end.
end

function M.add_menu_choice(item_id, new_choice)
   local item = M.get_menu_item(item_id)
   table.insert(item.choices, new_choice)  -- Add it at the end.
end


return M
