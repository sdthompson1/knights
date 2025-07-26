/*
 * task_manager.hpp
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
 * NB don't put the same task in 2 different taskmanagers, or add the same task twice to a taskmanager ....
 * 
 */

#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include "task.hpp"   // for CompareTime, TaskPri

#include "boost/shared_ptr.hpp"
#include <functional>
#include <set>

class CompareTime {
public:
    bool operator()(const boost::shared_ptr<Task> &lhs, const boost::shared_ptr<Task> &rhs) const {
        return lhs->time < rhs->time;
    }
};

class TaskManager {
public:
    TaskManager() : gvt(0) { }
    
    void addTask(boost::shared_ptr<Task> t, TaskPri pri, int exec_time);
    void changeTaskPri(boost::shared_ptr<Task> t, TaskPri new_pri);
    void changeExecTime(boost::shared_ptr<Task> t, int new_exec_time);
    void rmTask(boost::shared_ptr<Task> t);  // does nothing if t not found in the taskmanager
    void rmAllTasks();
    
    // This increases time then runs tasks as they become ready.
    void advanceToTime(int target_time);

    // How long (from current GVT) until the next task is going to become ready.
    int getTimeToNextUpdate() const;
    
    // For information
    int getGVT() const { return gvt; }
    
private:
    int gvt;
    typedef std::multiset<boost::shared_ptr<Task>, CompareTime > QueueType;
    enum { NUM_QUEUES = 2 };
    QueueType task_queue[NUM_QUEUES];

    QueueType::iterator findTask(boost::shared_ptr<Task>);
};

#endif
