/*! @file GoalKeeperProvider.h
    @brief Declaration of a behaviour provider for testing the goal keeper behaviour
 
    @class GoalKeeperProvider
    @brief A special behaviour for developing the goal keeper
 

    @author Jason Kulk
 
  Copyright (c) 2011 Jason Kulk
 
    This file is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NUbot.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOALKEEPER_PROVIDER_H
#define GOALKEEPER_PROVIDER_H

#include "../BehaviourProvider.h"
#include <boost/circular_buffer.hpp>

class GoalKeeperProvider : public BehaviourProvider
{
public:
    GoalKeeperProvider(Behaviour* manager);
    ~GoalKeeperProvider();
protected:
    void doBehaviour();

private:
    boost::circular_buffer<float> m_block_time;
};


#endif

