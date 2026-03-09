--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2026.
-- Copyright (C) Kalle Marjola, 1994.
--


d_huge = {
   name_key = "huge",
   func = function()
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
}

d_big = {
   name_key = "big",
   next = d_huge,
   func = function()
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
}

d_basic = {
   name_key = "basic",
   next = d_big,
   func = function()
      return {
         width=2, 
         height=2, 
         data={
            { type="edge" }, { type="edge" },
            { type="edge" }, { type="edge" }
         }
      } 
   end
}

d_small = {
   name_key = "small",
   next = d_basic,
   func = function()
      return {
         width = 2,
         height = 1,
         data = { 
            {type="block"}, {type="block"}
         }
      }
   end
}

d_tiny = {
   name_key = "tiny",
   next = d_small,
   func = function()
      return {
         width = 1,
         height = 1,
         data = {
            { type="block" }
         }
      }
  end
}

d_long_snake = {
   name_key = "long_snake",
   next = d_huge,
   func = function()
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
}

d_snake = {
   name_key = "snake",
   next = d_long_snake,
   func = function()
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
}

d_ring = {
   name_key = "ring",
   next = d_big,
   func = function()
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
}
