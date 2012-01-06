/*
 * knight_task.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
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

/*
 * This task reads controller inputs from the DungeonView and forwards
 * them to the player's knight. It also handles the calling of
 * Player::computeAvailableControls at regular intervals.
 *
 * (KnightTask can really be thought of as part of Player. In fact the
 * two files could probably be merged.)
 *
 */

#ifndef KNIGHT_TASK_HPP
#define KNIGHT_TASK_HPP

#include "task.hpp"

#include "boost/shared_ptr.hpp"

class Knight;
class Player;

class KnightTask : public Task {
public:
    KnightTask(Player &p) : player(p), stored_control(0), xbow_load_time(0), xbow_action_timer(0) { }
    virtual void execute(TaskManager &);

private:
    void doControls(shared_ptr<Knight>);
    
    Player &player;
    const Control *stored_control;
    int xbow_load_time;
    int xbow_action_timer;
};

#endif
