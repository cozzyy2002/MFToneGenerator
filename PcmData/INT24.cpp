#include "INT24.h"

#include <stdlib.h>

void INT24::construct(INT32 value)
{
	this->value.lower = (UINT16)value;
	this->value.upper = (INT8)(value >> 16);
}

INT24::operator float() const
{
	float abs_value = (float)(abs(value.upper) << 16) + value.lower;
	return (value.upper < 0) ? -abs_value : abs_value;
}

bool INT24::operator<(const INT32 int32) const
{
	INT24 that(int32);
	if(this->value.upper < that.value.upper) return true;
	else if(this->value.upper > that.value.upper) return false;
	return this->value.lower < that.value.lower;
}

bool INT24::operator<(float that) const
{
	return this->operator<(INT32(that));
}

bool INT24::operator>(const INT32 int32) const
{
	INT24 that(int32);
	if(this->value.upper > that.value.upper) return true;
	else if(this->value.upper < that.value.upper) return false;
	return this->value.lower > that.value.lower;
}

bool INT24::operator>(float that) const
{
	return this->operator>(INT32(that));
}


INT24 operator+(const INT24& a, const INT24& b) { return INT24((float)a + (float)b); }
INT24 operator+(const INT24& a, float b) { return INT24((float)a + b); }
INT24 operator-(const INT24& a, const INT24& b) { return INT24((float)a - (float)b); }
INT24 operator-(const INT24& a, float b) { return INT24((float)a - b); }
INT24 operator*(const INT24& a, float b) { return INT24((float)a * b); }
INT24 operator*(double a, const INT24& b) { return INT24((float)a * (float)b); }
