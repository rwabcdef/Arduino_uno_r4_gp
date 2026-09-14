/*
 * Frame.hpp
 *
 *  Created on: 22 Apr 2023
 *      Author: rw123
 */

#ifndef FRAME_HPP_
#define FRAME_HPP_

#include "env.h"
#include <stdint.h>

namespace SerLink
{

class Frame{
public:
	static const char TYPE_TRANSMISSION = 'T';
	static const char TYPE_UNIDIRECTION = 'U';
	static const char TYPE_ACK = 'A';
	static const int LEN_PROTOCOL = 5;
	static const int LEN_TYPE = 1;
	static const int LEN_ROLLCODE = 3;
	static const int LEN_DATALEN = 3;
  static const int LEN_HEADER = LEN_PROTOCOL + LEN_TYPE + LEN_ROLLCODE + LEN_DATALEN;
  static const int INDEX_START_PROTOCOL = 0;
  static const int INDEX_START_TYPE = LEN_PROTOCOL;
  static const int INDEX_START_ROLLCODE = INDEX_START_TYPE + LEN_TYPE;
  static const int INDEX_START_DATALEN = INDEX_START_ROLLCODE + LEN_ROLLCODE;
  static const int INDEX_START_DATA = INDEX_START_DATALEN + LEN_DATALEN;
  static const int MAX_DATALEN = 64;

  // DataLen (Ack code) return codes
  static const int ACK_OK = 900;

  char protocol[LEN_PROTOCOL];
	char type;
	uint16_t rollCode;
	uint16_t dataLen;
	//char data[MAX_DATALEN];
	char* buffer;

  Frame(char* pBuffer);
  Frame(char* pProtocol, char type, uint16_t rollCode, char* pBuffer, uint16_t dataLen, char* pData);
  void setProtocol(char* pProtocol);
  void toString(char* const pStr, uint8_t* pRetCode);
  void copy(Frame* copy);
  void toDebugString(char* pStr);
  static void incRollCode(uint16_t* rollCode);
  static void fromString(char* str, Frame* pFrame);
  static void int3dToStr(const uint16_t rollCode, char* pStr);
  static void str3dToInt(char* str, uint16_t* value);

private:

    static uint8_t digitCharValue(char c);
};

}
#endif /* FRAME_HPP_ */
