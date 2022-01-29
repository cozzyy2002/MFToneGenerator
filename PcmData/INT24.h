#include <Windows.h>

class INT24
{
public:
	INT24() : value({0}) {}
	INT24(INT32 value) { construct(value); }
	INT24(float value) { construct((INT32)value); }

	operator float() const;

	bool operator<(INT32) const;
	bool operator<(float) const;
	bool operator>(INT32) const;
	bool operator>(float) const;

protected:
	void construct(INT32);

	// 24bit internal type and value.
	struct value_t
	{
		UINT16 lower;
		INT8 upper;
	};
	value_t value;
};

INT24 operator+(const INT24&, const INT24&);
INT24 operator+(const INT24&, float);
INT24 operator-(const INT24&, const INT24&);
INT24 operator-(const INT24&, float);
INT24 operator*(const INT24&, float);
INT24 operator*(double, const INT24&);
