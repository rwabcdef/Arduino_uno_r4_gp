/*
 * Utils.hpp
 *
 *  Created on: 22 Apr 2023
 *      Author: rw123
 */

#ifndef SERLINK_UTILS_HPP_
#define SERLINK_UTILS_HPP_

#include <stdint.h>

namespace SerLink
{
class Utils{
public:
	static void uint16ToStr(uint16_t value, char* str, uint8_t numDigits = 5, char padChar = '0');
	static uint8_t strToUint8(char* str, uint8_t numDigits = 0);
};

}
#endif /* SERLINK_UTILS_HPP_ */
