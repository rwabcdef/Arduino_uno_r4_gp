/*
 * DebugUser.hpp
 *
 *  Created on: 30 Apr 2023
 *      Author: rw123
 */

#ifndef DEBUGUSER_HPP_
#define DEBUGUSER_HPP_

#include "env.hpp"

class DebugUser
{
protected:
	DebugPrint_defs::Levels debugLevel;
public:
	bool debugOn;
	DebugUser();
	void debugWrite(char* str);

};

#endif /* DEBUGUSER_HPP_ */
