/*
 * DebugUser.cpp
 *
 *  Created on: 30 Apr 2023
 *      Author: rw123
 */
#include "DebugUser.hpp"
#if defined(ENV_CONFIG__SYSTEM_PC)
#include "Global.hpp"
#include "DebugPrint.hpp"
#endif

DebugUser::DebugUser()
{
  this->debugOn = false;
}

void DebugUser::debugWrite(char* str)
{
#if defined(ENV_CONFIG__SYSTEM_PC)
	if(this->debugOn)
	{
		debugPrint->writeLine(str, this->debugLevel);
	}
#endif
}

