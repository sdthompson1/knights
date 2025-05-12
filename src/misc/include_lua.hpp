/*
 * include_lua.hpp
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


/*
There seems to be some disagreement about the correct way to include
the Lua headers.

If one is building Lua directly from source, there are three cases:

1) We are using Lua from a C program. In this case we build Lua using
   a C compiler, and we use #include "lua.h" in the C program.

2) We are using Lua from a C++ program, and we want to use Lua built
   as C++ (so that Lua errors are thrown as C++ exceptions, rather
   than using setjmp/longjmp). In this case we can build Lua using a
   C++ compiler, and again use #include "lua.h" in the C++ program.

3) We are using Lua from a C++ program, but we want to compile Lua as
   C (so that Lua errors are handled using setjmp/longjmp). In this
   (unusual) case, we would compile Lua using a C compiler, then use
   #include "lua.hpp" (not #include "lua.h") from the C++ program.
   Alternatively, one can wrap the include of "lua.h" in an `extern
   "C"` (which is essentially what lua.hpp does). This ensures that
   the C++-compiled program can correctly link with the symbols in the
   C-compiled Lua library.

In our case, we wish to use method 2 (because we need to propagate Lua
errors through C++ code, so we need the C++ exception handling). This
means that to include Lua, we should include "lua.h" (and also
"lualib.h" and "lauxlib.h" if desired).

The issue is that when using pre-packaged Lua packages, such as the
Debian "liblua" package or the vcpkg "lua" package, things seem to work
differently. In these cases, the Lua library is compiled using `extern
"C"` linkage, even though it is a C++ library, compiled with a C++
compiler. This means that when using either of these packages (as
opposed to building Lua directly from source), the "lua.h" include must
be wrapped in `extern "C"`.

Our solution is to use a preprocessor symbol
LUA_INCLUDES_REQUIRE_EXTERN_C, and make sure that this is defined both
in the Linux Makefile and in the Visual Studio project files, for
normal Knights builds. (This is assuming that the person building
Knights will get their Lua from an external package, rather than
building Lua themselves.) However, when building the Knights "virtual
server" (see src/virtual_server/Makefile), we leave
LUA_INCLUDES_REQUIRE_EXTERN_C undefined, since we typically build Lua
from source in that particular case.
*/

#ifdef LUA_INCLUDES_REQUIRE_EXTERN_C
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef LUA_INCLUDES_REQUIRE_EXTERN_C
}
#endif
