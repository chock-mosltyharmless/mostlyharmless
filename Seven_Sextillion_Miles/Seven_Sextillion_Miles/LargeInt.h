#pragma once

#define LI_NUM_INTEGERS 4

// It works by using 31 bit of the integer. The largest bit is used
// as a carry and always set to 0. The carry is used for computations
class LargeInt
{
public:

	// Construct to 0
	LargeInt(void)
	{
		for (int i = 0; i < LI_NUM_INTEGERS; i++)
		{
			data[i] = 0;
		}
	}

	// Construct from signed integer
	LargeInt(int base)
	{
		// carry bit is cleared
		data[0] = (unsigned int)base & 0x7fffffff;
		unsigned int sign1 = ((unsigned int)base) >> 31;
		unsigned int sign2 = sign1 | (sign1 << 1);
		unsigned int sign4 = sign2 | (sign2 << 2);
		unsigned int sign8 = sign4 | (sign4 << 4);
		unsigned int sign16 = sign4 | (sign4 << 8);
		unsigned int sign32 = sign16 | (sign16 << 16);
		unsigned int sign31 = sign32 & 0x7fffffff;
		for (int i = 1; i < LI_NUM_INTEGERS; i++)
		{
			data[i] = sign31;
		}
	}
	
	// Construct from floating point
	LargeInt(float base)
	{
		bool negate = false;
		if (base < 0)
		{
			base = -base;
			negate = true;
		}

		float multiplier = 1.0f;
		base += 0.5f; // adjust for rounding... Maybe this is not a smart idea?
		for (int i = 0; i < LI_NUM_INTEGERS; i++)
		{
			data[i] = (unsigned int)(base * multiplier) & 0x7FFFFFFF;
			multiplier /= (float)(1 << 31);
		}

		if (negate) neg();
	}

	~LargeInt(void)
	{
	}

	void add(LargeInt *other)
	{
		unsigned int carry = 0;
		for (int i = 0; i < LI_NUM_INTEGERS; i++)
		{
			data[i] += other->data[i] + carry;
			carry = data[i] >> 31;
			data[i] &= 0x7fffffff;
		}
	}

	void sub(LargeInt *other)
	{
		unsigned int carry = 0;
		for (int i = 0; i < LI_NUM_INTEGERS; i++)
		{
			data[i] -= other->data[i] + carry;
			carry = data[i] >> 31;
			data[i] &= 0x7fffffff;
		}
	}

	void neg(void)
	{
		unsigned int carry = 0;
		for (int i = 0; i < LI_NUM_INTEGERS; i++)
		{
			data[i] = -(int)data[i] - carry;
			carry = data[i] >> 31;
			data[i] &= 0x7fffffff;
		}
	}

	// returns -1, or 1 (1 is returned for 0, too).
	int signish(void)
	{
		if (data[LI_NUM_INTEGERS-1] & 0x40000000)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	float getFloat(void)
	{
		float result = 0.0f;
		float multiplier = 1.0f;
		bool needsNegation = false;
		if (signish() < 0)
		{
			multiplier = -1.0f;
			neg();
			needsNegation = true;
		}

		for (int i = 0; i < LI_NUM_INTEGERS; i++)
		{
			result += data[i] * multiplier;
			multiplier *= (float)(1 << 31);
		}

		if (needsNegation) neg();
	}

private:
	// Data is really signed, I just call it unsigned so that I can shift it...
	unsigned int data[LI_NUM_INTEGERS];
};

