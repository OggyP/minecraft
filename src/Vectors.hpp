class Vector3f
{
public:
	float x = 0;
	float y = 0;
	float z = 0;

	float directionH = 0;
	float directionV = 0;

	float getMagnitude()
	{
		return (float)sqrt(x * x + y * y + z * z);
	}

	// This will return the magnitude squared. Use this if you can as it will result faster performace.
	float getMagnitudeSqr()
	{
		return (float)sqrt(x * x + y * y + z * z);
	}

	float getIndex(int index)
	{
		if (index == 0)
		{
			return x;
		}
		if (index == 1)
		{
			return y;
		}
		if (index == 2)
		{
			return z;
		}
		return 0.0f;
	}

	void setIndex(int index, float value)
	{
		if (index == 0)
		{
			x = value;
		}
		else if (index == 1)
		{
			y = value;
		}
		else if (index == 2)
		{
			z = value;
		}
	}

	float getMagnitudeXY()
	{
		return (float)sqrt(x * x + y * y);
	}

	// This will return the magnitude of the X and Y values squared. Use this if you can as it will result faster performace.
	float getMagnitudeXYSqr()
	{
		return (float)sqrt(x * x + y * y);
	}

	std::array<float, 2> getDirection()
	{
		// horizonal
		return { (float)atan2(y, x), (float)atan2(z, getMagnitudeXY()) };
	}

	void updateCoords(float magnitude)
	{
		z = magnitude * sin(directionV);
		float xyMag = magnitude * cos(directionV);
		x = xyMag * cos(directionH);
		y = xyMag * sin(directionH);
	}

	void updateDirection()
	{
		directionH = atan2(y, x);
		directionV = atan2(z, getMagnitudeXY());
	}

	void normaliseVectorXY(double magnitude)
	{
		double mag = x * x + y * y;
		if (mag != 0)
		{
			double normaliseAmt = magnitude / sqrt(mag);
			x = x * normaliseAmt;
			y = y * normaliseAmt;
		}
	}
};

class Vector2f
{
public:
	float x = 0;
	float y = 0;

	float getMagnitude()
	{
		return sqrt(x * x + y * y);
	}

	// This will return the magnitude squared. Use this if you can as it will result faster performace.
	float getMagnitudeSqr()
	{
		return x * x + y * y;
	}

	float getDirection()
	{
		return atan2(y, x);
	}

	void setVector(float direction, float magnitude)
	{
		x = magnitude * cos(direction);
		y = magnitude * sin(direction);
	}

	void normaliseVector(float magnitude)
	{
		double normaliseAmt = magnitude / sqrt(x * x + y * y);
		x = x * normaliseAmt;
		y = y * normaliseAmt;
	}
};