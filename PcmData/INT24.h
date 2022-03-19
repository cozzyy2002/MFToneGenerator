#include <Windows.h>

class INT24
{
public:
	INT24() { construct(0); }
	INT24(INT32 value) { construct(value); }
	INT24(double value) { construct((INT32)value); }

	operator INT32() const { return toINT32(); }
	INT32 toINT32() const;

	INT24 operator+(const INT24& that) const { return INT24(this->toINT32() + that.toINT32()); }
	INT24 operator+(double that) const { return INT24(this->toINT32() + that); }
	INT24 operator-(const INT24& that) const { return INT24(this->toINT32() - that.toINT32()); }
	INT24 operator-(double that) const { return INT24(this->toINT32() - that); }
	INT24 operator*(double that) const { return INT24(this->toINT32() * that); }

	static const INT32 MaxValue = 0x007fffff;	// +8388607;
	static const INT32 MinValue = 0xff800000;	// -8388608;

protected:
	void construct(INT32);

	// 24bit internal value.
	// Note: Do not define this value in struct, or sizeof(INT24) can not be 3.
	BYTE value[3];
};
