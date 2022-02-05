#include "INT24.h"

#include <stdlib.h>

void INT24::construct(INT32 value)
{
	this->value[0] = (BYTE)value;
	this->value[1] = (BYTE)(value >> 8);
	this->value[2] = (BYTE)(abs(value >> 16) * ((value < 0) ? -1 : 1));
}

INT24::operator float() const
{
	float fval = (float)(value[0] + (value[1] << 8) + (abs(value[2]) << 16));
	return fval * ((value[2] < 0) ? -1 : 1);
}

INT24 operator+(const INT24& a, const INT24& b) { return INT24((float)a + (float)b); }
INT24 operator+(const INT24& a, float b) { return INT24((float)a + b); }
INT24 operator-(const INT24& a, const INT24& b) { return INT24((float)a - (float)b); }
INT24 operator-(const INT24& a, float b) { return INT24((float)a - b); }
INT24 operator*(const INT24& a, float b) { return INT24((float)a * b); }
INT24 operator*(double a, const INT24& b) { return INT24((float)a * (float)b); }
