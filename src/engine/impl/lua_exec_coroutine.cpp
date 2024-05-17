/*
 * lua_exec_coroutine.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#include "knights_callbacks.hpp"
#include "lua_exec_coroutine.hpp"
#include "lua_ref.hpp"
#include "lua_traceback.hpp"
#include "mediator.hpp"
#include "task.hpp"
#include "task_manager.hpp"

#include "lua.hpp"

#include "boost/noncopyable.hpp"

namespace {
    class CoroutineTask : public Task, boost::noncopyable {
    public:
        // Protocol: Push cxt, fn and args onto lua stack, then call ctor
        // which will pop them and push them onto the thread stack.
        // Then call execute(tm) to run it for the first time, or just
        // add it to TaskManager if you want a delayed initial
        // execution.
        explicit CoroutineTask(lua_State *lua, int nargs);
        ~CoroutineTask();                
        virtual void execute(TaskManager &tm); // overridden from Task

        bool doExec(TaskManager &tm);

    private:
        lua_State *getThread(lua_State *);
        void doExec(Mediator &m, TaskManager &tm, int nargs);
    };
}

CoroutineTask::CoroutineTask(lua_State *lua, int nargs)
{
    // [cxt fn args]
    
    // setup a new thread, store in registry
    lua_newthread(lua); // [cxt fn args thread]
    lua_State *thread = lua_tothread(lua, -1);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, this); // [cxt fn args]

    // move the main function and arguments across to the new thread.
    lua_xmove(lua, thread, nargs + 1);  // [cxt]

    // now save the context
    lua_rawsetp(lua, LUA_REGISTRYINDEX, ((char*)this)+1);  // []
}

CoroutineTask::~CoroutineTask()
{
    try {
        // We must remove the key from the registry
        Mediator &mediator(Mediator::instance());
        lua_State *lua = mediator.getLuaState();
        lua_pushnil(lua);
        lua_rawsetp(lua, LUA_REGISTRYINDEX, this);
        lua_pushnil(lua);
        lua_rawsetp(lua, LUA_REGISTRYINDEX, ((char*)this)+1); // extra key
    } catch (...) {
        // prevent (e.g.) MediatorNotFound from escaping
    }
}

lua_State * CoroutineTask::getThread(lua_State *lua)
{
    // get the thread from the registry
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);  // [thread]
    lua_State *thread = lua_tothread(lua, -1);
    lua_pop(lua, 1);      // []
    ASSERT(thread);
    return thread;
}

void CoroutineTask::execute(TaskManager &tm)
{
    // ignore return value from doExec
    doExec(tm);
}

bool CoroutineTask::doExec(TaskManager &tm)
{
    Mediator &mediator(Mediator::instance());
    lua_State *lua = mediator.getLuaState();
    lua_State *thread = getThread(lua);

    // work out how many args there are. (We assume that if the thread
    // stack is non-empty then this is the first run, otherwise, this
    // is a resume)
    const int init_stack = lua_gettop(thread);
    const int nargs = init_stack == 0 ? 0 : init_stack - 1;

    // Save the current value of "cxt" in the registry
    lua_getglobal(lua, "cxt");  // pushes oldcxt
    LuaRef cxt_save(lua);       // pops it again.

    // restore the context
    lua_rawgetp(thread, LUA_REGISTRYINDEX, ((char*)this)+1);
    lua_setglobal(thread, "cxt");

    // resume the thread
    //   nullptr => being called from top level (not from any particular thread)
    int nresults = 0;
    const int status = lua_resume(thread, nullptr, nargs, &nresults);

    // save the context again
    lua_getglobal(thread, "cxt");
    lua_rawsetp(thread, LUA_REGISTRYINDEX, ((char*)this)+1);

    // and restore the previous value of cxt
    cxt_save.push(lua);
    lua_setglobal(lua, "cxt");

    switch (status) {
    case LUA_OK:
        // Coroutine has returned. Clear its stack, then return w/o
        // rescheduling the task. This will cause *this to be deleted,
        // which in turn will remove the Lua thread from the registry,
        // the Lua GC will then destroy the thread.
        {
            bool result = false;
            if (nresults >= 1) {
                result = lua_toboolean(thread, -nresults) != 0;
            }
            lua_settop(thread, 0);
            return result;
        }

    case LUA_YIELD:
        // Coroutine has yielded a value.
        if (nresults >= 1 && lua_isnumber(thread, -nresults)) {
            const int time = lua_tointeger(thread, -nresults);
            if (time >= 0) {

                // reschedule the task
                tm.addTask(shared_from_this(), TP_NORMAL, tm.getGVT() + time);

                // remove yielded values from thread stack
                lua_pop(thread, nresults);

                return false;
            }
        }

        // Otherwise: push a fake error message then fall through
        // to error case.
        lua_settop(thread, 0);
        lua_pushstring(thread, "incorrect yield");

        // Fall through

    default:
        // Coroutine raised an error. The error message is on top of stack
        {
            const char *p = lua_tostring(thread, -1);
            std::string err(p ? p : "<No err msg>");
            err += LuaTraceback(thread);
            lua_settop(thread, 0);  // clear its stack
            mediator.getCallbacks().gameMsg(-1, err, true);  // display the error message
            return false;
        }
    }
}
            

bool LuaExecCoroutine(lua_State *lua, int nargs)
{
    shared_ptr<CoroutineTask> task(new CoroutineTask(lua, nargs));  // pops cxt, fn and args from stack.
    return task->doExec(Mediator::instance().getTaskManager());
}
