/*
 * Writer_config.hpp
 *
 *  Created on: 31 Oct 2023
 *      Author: rw123
 */

#ifndef WRITER_CONFIG_HPP_
#define WRITER_CONFIG_HPP_

#include "env.h"
//#include "Reader.hpp"

#define WRITER_CONFIG__WRITER0


//----------------------------------------------------------
// WRITER0
#ifdef WRITER_CONFIG__WRITER0

#include "uart_wrapper.hpp"

#define WRITER_CONFIG__WRITER0_ID 1 // WRITER0 instance id

#endif
//----------------------------------------------------------



#endif /* WRITER_CONFIG_HPP_ */
