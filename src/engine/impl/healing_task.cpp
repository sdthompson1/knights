/*
 * healing_task.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#include "healing_task.hpp"
#include "creature.hpp"
#include "task_manager.hpp"

void HealingTask::execute(TaskManager &tm)
{
    shared_ptr<Creature> cr(creature.lock());
    if (cr && cr->getMap()) {
        cr->addToHealth(healing_amount);
        int gvt = tm.getGVT();
        tm.addTask(shared_from_this(), TP_NORMAL, gvt + time_interval);
    }
}
