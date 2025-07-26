/*
 * monster_task.cpp
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

#include "misc.hpp"

#include "knight.hpp"
#include "mediator.hpp"
#include "monster_manager.hpp"
#include "monster_task.hpp"
#include "player.hpp"
#include "task_manager.hpp"

void MonsterTask::execute(TaskManager &tm)
{
    Mediator &mediator = Mediator::instance();
    const int radius = mediator.cfgInt("monster_radius");
    const int interval = mediator.cfgInt("monster_interval");
    
    int left = 999999, right = 0,
        bottom = 999999, top = 0;
    Mediator &med(mediator);
    for (std::vector<Player*>::const_iterator it = med.getPlayers().begin();
    it != med.getPlayers().end(); ++it) {
        shared_ptr<Knight> kt = (*it)->getKnight();
        if (kt && kt->getMap()) {
            const int x = kt->getPos().getX();
            if (x-radius < left) left = x-radius;
            if (x+1+radius > right) right = x+1+radius;
            const int y = kt->getPos().getY();
            if (y-radius < bottom) bottom = y-radius;
            if (y+1+radius > top) top = y+1+radius;
        }
    }
    med.getMonsterManager().doMonsterGeneration(*med.getMap(), left, bottom, right, top);

    tm.addTask(shared_from_this(), TP_LOW, tm.getGVT() + interval);
}
