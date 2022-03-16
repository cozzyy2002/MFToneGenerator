#include "INT24.h"

static const BYTE sign = 0x80;

void INT24::construct(INT32 value)
{
	BYTE* bytes = (BYTE*)&value;
	this->value[0] = bytes[0];
	this->value[1] = bytes[1];
	this->value[2] = bytes[2] | (bytes[3] & sign);
}

INT24::operator INT32() const
{
	BYTE bytes[4] = {
		value[0],
		value[1],
		value[2],
		(BYTE)((value[2] & sign) ? 0xff : 0)
	};
	return *((INT32*)bytes);
}

INT24 operator+(const INT24& a, const INT24& b) { return INT24((INT32)a + (INT32)b); }
INT24 operator+(const INT24& a, double b) { return INT24((INT32)a + b); }
INT24 operator-(const INT24& a, const INT24& b) { return INT24((INT32)a - (INT32)b); }
INT24 operator-(const INT24& a, double b) { return INT24((INT32)a - b); }
INT24 operator*(const INT24& a, double b) { return INT24((INT32)a * b); }
INT24 operator*(double a, const INT24& b) { return INT24((INT32)(a * (INT32)b)); }
