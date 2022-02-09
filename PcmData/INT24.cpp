#include "INT24.h"

void INT24::construct(INT32 value)
{
	this->value[0] = (BYTE)value;
	this->value[1] = (BYTE)(value >> 8);
	this->value[2] = (BYTE)(abs(value >> 16) * ((value < 0) ? -1 : 1));
}

INT24::operator INT32() const
{
	static const BYTE sign = 0x80;
	BYTE bytes[4] = {
		value[0],
		value[1],
		value[2],
		(BYTE)((value[2] & sign) ? 0xff : 0)
	};
	return *((INT32*)bytes);
}

INT24 operator+(const INT24& a, const INT24& b) { return INT24((float)a + (float)b); }
INT24 operator+(const INT24& a, float b) { return INT24((float)a + b); }
INT24 operator-(const INT24& a, const INT24& b) { return INT24((float)a - (float)b); }
INT24 operator-(const INT24& a, float b) { return INT24((float)a - b); }
INT24 operator*(const INT24& a, float b) { return INT24((float)a * b); }
INT24 operator*(double a, const INT24& b) { return INT24((float)a * (float)b); }
