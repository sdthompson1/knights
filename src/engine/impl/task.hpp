/*
 * task.hpp
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

/*
 * Task: something to be run at a certain point in the future.
 * 
 */

#ifndef TASK_HPP
#define TASK_HPP

#include "boost/enable_shared_from_this.hpp"
using namespace boost;

class TaskManager;

enum TaskPri { TP_LOW, TP_NORMAL };

class Task : public enable_shared_from_this<Task> {
    friend class TaskManager;
    friend class CompareTime;
public:
    Task() : time(-1) { }
    virtual ~Task() { }

    // tasks are removed from the TaskManager before execution. They should be re-added (using
    // addTask) if re-execution is desired. (A reference to the TaskManager is passed in
    // to execute().)
    // Note: the TaskManager will hold a shared_ptr to the Task during the execute() call;
    // the task is therefore guaranteed not to be deleted during execute().
    virtual void execute(TaskManager &) = 0;

    // find (absolute) time that we will run next.
    // returns negative value if not currently inserted into a task manager, or during
    // execute().
    int getExecTime() const { return time; }

private:
    TaskPri pri;
    int time;  // time of next run. (this is always -ve if task not currently scheduled.)
};

#endif
