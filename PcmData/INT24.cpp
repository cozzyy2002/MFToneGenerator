#include "INT24.h"

static const BYTE sign = 0x80;

void INT24::construct(INT32 value)
{
	BYTE* int32 = (BYTE*)&value;
	this->value[0] = int32[0];
	this->value[1] = int32[1];
	this->value[2] = int32[2] | (int32[3] & sign);
}

INT32 INT24::toINT32() const
{
	BYTE int32[4] = {
		value[0],
		value[1],
		value[2],
		(BYTE)((value[2] & sign) ? 0xff : 0)
	};
	return *((INT32*)int32);
}
