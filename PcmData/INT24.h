#include <Windows.h>

class INT24
{
public:
	INT24() { construct(0); }
	INT24(INT32 value) { construct(value); }
	INT24(float value) { construct((INT32)value); }

	operator INT32() const;

	static const INT32 MaxValue = +8388607;
	static const INT32 MinValue = -8388608;

protected:
	void construct(INT32);

	// 24bit internal value.
	// Note: Do not define this value in struct, or sizeof(INT24) can not be 3.
	BYTE value[3];
};

INT24 operator+(const INT24&, const INT24&);
INT24 operator+(const INT24&, float);
INT24 operator-(const INT24&, const INT24&);
INT24 operator-(const INT24&, float);
INT24 operator*(const INT24&, float);
INT24 operator*(double, const INT24&);
