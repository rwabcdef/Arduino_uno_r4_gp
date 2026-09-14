/*
 * Utils.cpp
 *
 *  Created on: 9 Jun 2024
 *      Author: rw123
 */
#include "SerLink_Utils.hpp"

namespace SerLink
{

static const char digits[10] = {'0','1','2','3','4','5','6','7','8','9'};

void Utils::uint16ToStr(uint16_t value, char* str, uint8_t numDigits, char padChar)
{
  uint16_t working = value;
  uint8_t i, j;

  // Zero pad the output string
  for(i=0; i<numDigits; i++)
  {
    str[i] = padChar;
  }

  if(numDigits < 5)
  {
    // Reduce value to remove unwanted most significant digit(s).
    uint16_t modVal = 1;
    for(i=0; i<numDigits; i++)
    {
      modVal *= 10;
    }
    working = working % modVal;
  }

  for(i=0; i<numDigits; i++)
  {
    uint16_t divider = 1;
    for(j=0; j<(numDigits - i - 1); j++)
    {
      divider *= 10;
    }
    uint8_t digit = working / divider;
    str[i] = digits[digit];

    uint16_t subVal = digit * divider;
    working -= subVal;
  }
  str[i] = 0;
}

uint8_t Utils::strToUint8(char* str, uint8_t numDigits)
{
  uint8_t i, j, k, total = 0;

  if(numDigits == 0 || numDigits > 3)
  {
    numDigits = 3;
  }

  // calculate multiplier for current digit,
  // e.g. for hundreds digit - multiplier is 100.
  for(i=numDigits; i>0; i--)
  {
    uint8_t multiplier = 1;
    for(j=i-1; j>0; j--)
    {
      multiplier *= 10;
    }

    uint8_t charIndex = numDigits - i;
    uint8_t value = 0;

    for(k=0; k<10; k++)
    {
      if(str[charIndex] == digits[k])
      {
        value = k * multiplier;
      }
    }

    total += value;
  }
  return total;
}

}


