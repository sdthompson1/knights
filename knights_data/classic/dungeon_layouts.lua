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


-- NB If exits is not specified then all possible exits are assumed.

function df_tiny()
   return {
      width = 1,
      height = 1,
      data = {
         { type="block" }
      }
   }
end
d_tiny = kts.DungeonLayout( "Tiny", df_tiny )

function df_small()
   return {
      width = 2,
      height = 1,
      data = { 
         {type="block"}, {type="block"}
      }
   }
end
d_small = kts.DungeonLayout( "Small", df_small )

function df_basic()
   return {
      name="Basic",
      width=2, 
      height=2, 
      data={
         { type="edge" }, { type="edge" },
         { type="edge" }, { type="edge" }
      }
   } 
end
d_basic = kts.DungeonLayout( "Basic", df_basic )

function df_big()
   if kts.RandomChance(0.75) then
      return { 
         width=2,
         height=3,
         data={
            { type="edge"  }, { type="edge"  },
            { type="block" }, { type="block" },
            { type="edge"  }, { type="edge"  }
         }
      }
   else
      return {
         width=2,
         height=3,
         data={
            { type="edge" }, { type="edge"  },
            { type="edge" }, { type="block" },
            { type="none" }, { type="edge"  }
         }
     }
   end
end
d_big = kts.DungeonLayout( "Big", df_big )

function df_huge()
   local choice = kts.RandomRange(1,3)
   if choice == 1 then
      return {
         width=3,
         height=3,
         data={
            { type="edge"  }, { type="block" }, { type="edge"  },
            { type="block" }, { type="block" }, { type="block" },
            { type="edge"  }, { type="block" }, { type="edge"  }
         }
      }
   elseif choice == 2 then
      return {
         width=3,
         height=3,
         data={
            { type="block" }, { type="edge"  }, { type="none"  },
            { type="edge"  }, { type="block" }, { type="block" },
            { type="none"  }, { type="block" }, { type="edge"  }
         }
      }
   elseif choice == 3 then
      return {
         width=3,
         height=3,
         data={
            { type="edge"  }, { type="block" }, { type="edge"  },
            { type="block" }, { type="block" }, { type="block" },
            { type="edge"  }, { type="block" }, { type="none"  } }
      }
   end
end
d_huge = kts.DungeonLayout( "Huge", df_huge )

function df_snake()
   local choice = kts.RandomRange(1,10)
   if choice <= 6 then
      return {
         width=3,
         height=1,
         data={ { type="edge" }, { type="block" }, { type="edge" } }
      }
   elseif choice == 7 then
      return {
         width=3,
         height=2,
         data={
            { type="block" }, { type="block" }, { type="edge" },
            { type="edge"  }, { type="none"  }, { type="none" } }
      }
   elseif choice == 8 then
      return {
         width=3,
         height=3,
         data={
            { type="block" }, { type="block" }, { type="edge" },
            { type="block" }, { type="none"  }, { type="none" },
            { type="edge"  }, { type="none"  }, { type="none" }  }
      }
   elseif choice == 9 then
      return {
         width=3,
         height=2,
         data={
            { type="edge"  }, { type="none"  }, { type="edge"  },
            { type="block" }, { type="block" }, { type="block" }  }
      }
   elseif choice == 10 then
      return {
         width=3,
         height=3,
         data={
            { type="edge"  }, { type="none"  }, { type="none"  },
            { type="block" }, { type="block" }, { type="block" },
            { type="none"  }, { type="none"  }, { type="edge"  }  }
      }
   end
end
d_snake = kts.DungeonLayout( "Snake", df_snake )

function df_long_snake()
   local choice = kts.RandomRange(1,5)
   if choice == 1 then
      return {
         width=2,
         height=3,
         data={
            { type="block", exits="se" }, { type="block", exits="sw" },
            { type="block", exits="ns" }, { type="block", exits="ns" },
            { type="edge",  exits="n"  }, { type="edge",  exits="n"  }  }
      }
   elseif choice == 2 then
      return {
         width=3, 
         height=3,
         data={
            { type="block" }, { type="block" }, { type="block" },
            { type="block" }, { type="none"  }, { type="edge"  },
            { type="edge"  }, { type="none"  }, { type="none"  }  }
      }
   elseif choice == 3 then
      return {
         width=3,
         height=3,
         data={
            { type="block" }, { type="block" }, { type="block" },
            { type="block" }, { type="none"  }, { type="block" },
            { type="edge"  }, { type="none"  }, { type="edge"  }  }
      }
   elseif choice == 4 then
      return {
         width=3,
         height=3,
         data={
            { type="block" }, { type="edge",  exits="w"  }, { type="none"  },
            { type="block" }, { type="block", exits="we" }, { type="block" },
            { type="none"  }, { type="edge",  exits="e"  }, { type="block" }  }
      }
   elseif choice == 5 then
      return {
         width=3,
         height=3,
         data={
            { type="edge",  exits="e" }, { type="block" }, { type="block" },
            { type="edge",  exits="s" }, { type="none"  }, { type="block" },
            { type="block"            }, { type="block" }, { type="block" }  }
      }
   end
end
d_long_snake = kts.DungeonLayout( "Long Snake", df_long_snake )

function df_ring()
   local choice = kts.RandomRange(1,6)
   if choice <= 4 then
      return {
         width=3,
         height=3,
         data={
            {type="block"            }, {type="block"              }, {type="block"            },
            {type="block", exits="ns"}, {type="special", exits="n" }, {type="block", exits="ns"},
            {type="block"            }, {type="block",   exits="we"}, {type="block"            }  }
      }
   elseif choice == 5 then
      return { 
         width=3,
         height=3,
         data={
            {type="block"            }, {type="block", exits="we"}, {type="block"  },
            {type="block", exits="ns"}, {type="block", exits="es"}, {type="block"  },
            {type="block"            }, {type="block"            }, {type="special"}  }
      }
   elseif choice == 6 then
      return {
         width=3,
         height=3,
         data={
            {type="block"            }, {type="block"            }, {type="special"},
            {type="block", exits="ns"}, {type="block", exits="ns"}, {type="none"   },
            {type="block"            }, {type="block"            }, {type="none"   }  }
      }
   end
end
d_ring = kts.DungeonLayout( "Ring", df_ring )
