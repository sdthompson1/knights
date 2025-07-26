/*
 * healing_task.hpp
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
 * HealingTask is a Task that adds a certain amount to a creature's
 * health at regular intervals. It's used both by homes (in
 * Player::onMove) and by regeneration potions.
 *
 */

#ifndef HEALING_TASK_HPP
#define HEALING_TASK_HPP

#include "task.hpp"

class Creature;

class HealingTask : public Task {
public:
    HealingTask(weak_ptr<Creature> c, int dt, int ha) : creature(c), time_interval(dt),
                                                        healing_amount(ha) { }
    virtual void execute(TaskManager &);

protected:
    shared_ptr<Creature> getCreature() const { return creature.lock(); }

private:
    weak_ptr<Creature> creature;
    int time_interval, healing_amount;
};

#endif

