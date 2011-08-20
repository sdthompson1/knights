/*
 * task_manager.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include "task.hpp"
#include "task_manager.hpp"

#include <utility>
using namespace std;

TaskManager::QueueType::iterator TaskManager::findTask(shared_ptr<Task> t)
{
    pair<QueueType::iterator,QueueType::iterator> p = task_queue[t->pri].equal_range(t); // compare with CompareTime
    for (QueueType::iterator it = p.first; it != p.second; ++it) {
        if (*it == t) {   // compare by pointer
            return it;
        }
    }
    return task_queue[t->pri].end();
}

void TaskManager::addTask(shared_ptr<Task> t, TaskPri pri, int exec_time)
{
    ASSERT(t->time == -1); // same task must never be added twice.
    t->pri = pri;
    t->time = exec_time;
    task_queue[pri].insert(t);
}

void TaskManager::changeTaskPri(shared_ptr<Task> t, TaskPri new_pri)
{
    if (t->pri == new_pri) return;
    QueueType::iterator it = findTask(t);
    if (it == task_queue[t->pri].end()) return;
    task_queue[t->pri].erase(it);
    t->pri = new_pri;
    task_queue[new_pri].insert(t);
}

void TaskManager::changeExecTime(shared_ptr<Task> t, int new_exec_time)
{
    if (t->time == new_exec_time) return;
    QueueType::iterator it = findTask(t);
    if (it == task_queue[t->pri].end()) return;
    task_queue[t->pri].erase(it);
    t->time = new_exec_time;
    task_queue[t->pri].insert(t);
}

void TaskManager::rmTask(shared_ptr<Task> t)
{
    if (!t) return;
    QueueType::iterator it = findTask(t);
    if (it != task_queue[t->pri].end()) {
        t->time = -1;
        task_queue[t->pri].erase(it);
    }
}

void TaskManager::rmAllTasks()
{
    for (int i = 0; i < NUM_QUEUES; ++i) {
        for (QueueType::iterator it = task_queue[i].begin(); it != task_queue[i].end(); ++it) {
            (*it)->time = -1;
        }
        task_queue[i].clear();
    }
}

void TaskManager::advanceToTime(int target_time)
{
    // Time should not go backwards -- don't want any of those nasty paradoxes :)
    if (target_time <= gvt) return;

    // For now, just run all the normal-pri tasks first, then all the low-pri tasks.
    // In future, could ignore some of the low-pri ones dependent on how much CPU time is
    // available.
    for (int tp = 1; tp>=0; --tp) {
        while (!task_queue[tp].empty()) {
            shared_ptr<Task> t = *(task_queue[tp].begin());
            ASSERT(t);
            if (t->time > target_time) break;
            if (t->time > gvt) gvt = t->time;
            task_queue[tp].erase(task_queue[tp].begin());
            t->time = -1;
            t->execute(*this);
        }
    }
    
    // finally bring gvt up to target_time
    gvt = target_time;
}
