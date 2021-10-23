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

	float getMagnitudeXY()
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
};