#include <SimplexNoise.hpp>
#include <chrono>
#include <random>

/* Seed */
std::random_device rd;

/* Random number generator */
std::default_random_engine generator(rd());

/* Distribution on which to apply the generator */
std::uniform_int_distribution<unsigned> distribution(0, 0xFFFF);

std::array<unsigned, 2> seed = { distribution(generator), distribution(generator) };

const int chunkSize = 16;
const int chunkHeight = 256;

double MapValue(double a0, double a1, double b0, double b1, double a);
double MapValue(double a0, double a1, double b0, double b1, double a)
{
	return b0 + (b1 - b0) * ((a - a0) / (a1 - a0));
}

struct block
{
	int blockType;
	float lightLevel;
	float blockOffset[2] = { 0, 0 };
};

class GameChunk
{
public:
	block tiles[chunkSize][chunkSize][chunkHeight];
	int chunkX;
	int chunkY;
	bool hasVerticies = false;
	std::vector<GLfloat> savedVerticies;
	GameChunk()
	{}
	GameChunk(int chunkXin, int chunkYin)
	{
		chunkX = chunkXin;
		chunkY = chunkYin;
		for (int x = 0; x < chunkSize; x++)
		{
			for (int y = 0; y < chunkSize; y++)
			{
				float xVal2Dnoise = (seed[0] + x + chunkX * chunkSize) * 0.005 + 0.0005;
				float yVal2Dnoise = (seed[1] + y + chunkY * chunkSize) * 0.005 + 0.0005;
				float xVal2DnoiseLower = (seed[0] + x + chunkX * chunkSize) * 0.02 + 0.001;
				float yVal2DnoiseLower = (seed[1] + y + chunkY * chunkSize) * 0.02 + 0.001;
				float xVal3Dnoise = (seed[0] + x + chunkX * chunkSize) * 0.02 + 0.01;
				float yVal3Dnoise = (seed[1] + y + chunkY * chunkSize) * 0.02 + 0.01;
				float noiseVal = 128 + SimplexNoise::noise(xVal2Dnoise, yVal2Dnoise) * 50;
				noiseVal += SimplexNoise::noise(xVal2DnoiseLower, yVal2DnoiseLower) * 5;
				// if (noiseVal > 78)
				// 	noiseVal += (noiseVal - 78) * (noiseVal - 78) * 2;
				float noiseValMinus1 = noiseVal - 1;
				float noiseValMinusFifty = noiseVal - 50;
				bool previousHadLightLevel = false;
				float previousBrightness = 1.0f;
				for (int z = chunkHeight - 1; z >= 0; z--)
				{
					float zVal = z * 0.02 + 0.01;
					float zCheckVal = MapValue(0, 94, 0, 0.7, z);
					if (z < 8)
					{
						zCheckVal += 0.15 * (8 - z);
					}
					if (z <= noiseVal && SimplexNoise::noise(xVal3Dnoise, yVal3Dnoise, zVal) <= zCheckVal)
					{
						struct block newBlock;
						newBlock.blockOffset[0] = 0.0f;
						newBlock.blockOffset[1] = 0.0f;
						newBlock.blockType = 1;
						newBlock.lightLevel = 1.0f;
						if (z <= noiseValMinus1)
						{
							if (z <= noiseValMinusFifty)
								newBlock.lightLevel = 0.5f;
							else
							{
								if (!previousHadLightLevel)
									newBlock.lightLevel -= (noiseVal - (float)z) / 100.0f;
								else
									newBlock.lightLevel = previousBrightness - 0.01;
							}
						}
						previousBrightness = newBlock.lightLevel;
						tiles[x][y][z] = newBlock;
						previousHadLightLevel = true;
					}
					else
					{
						struct block newBlock;
						newBlock.blockOffset[0] = 0.0f;
						newBlock.blockOffset[1] = 0.0f;
						newBlock.blockType = 0;
						newBlock.lightLevel = 1.0f;
						if (z <= noiseVal)
						{
							if (z <= noiseValMinus1)
							{
								if (z <= noiseValMinusFifty)
									newBlock.lightLevel = 0.5f;
								else
								{
									if (previousHadLightLevel)
										newBlock.lightLevel -= (noiseVal - (float)z) / 100.0f;
									else
										newBlock.lightLevel = previousBrightness - 0.01;
								}
							}
							previousHadLightLevel = true;
							previousBrightness = newBlock.lightLevel;
						}
						else
							previousHadLightLevel = false;
						tiles[x][y][z] = newBlock;
					}
				}
			}
		}
	}
	// std::array<int, 4>, std::vector<GameChunk>
	std::vector<GLfloat> genVerticies(GameChunk* PlusX, GameChunk* MinusX, GameChunk* PlusY, GameChunk* MinusY) // -1 meaning no chunk available, the array has the index of each chunk in the vector (NESW)
	{
		std::vector<GLfloat> chunkVertices;
		for (int x = 0; x < chunkSize; x++)
		{
			for (int y = 0; y < chunkSize; y++)
			{
				for (int z = 0; z < chunkHeight; z++)
				{
					if (tiles[x][y][z].blockType != 0)
					{
						auto currentBlock = &tiles[x][y][z];
						// Up
						const int blockCoords[2] = { chunkX * chunkSize + x, chunkY * chunkSize + y };
						if (z + 1 >= chunkHeight || tiles[x][y][z + 1].blockType == 0)
						{
							float brightness = currentBlock->lightLevel;
							std::vector<GLfloat> face = {
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness
							};
							chunkVertices.insert(chunkVertices.end(), face.begin(), face.end());
						}
						if (z - 1 < 0 || tiles[x][y][z - 1].blockType == 0)
						{
							float brightness = currentBlock->lightLevel * 0.7f;
							std::vector<GLfloat> face = {
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness, // Down
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness
							};
							chunkVertices.insert(chunkVertices.end(), face.begin(), face.end());
						}
						if ((x - 1 >= 0 && tiles[x - 1][y][z].blockType == 0) || (x - 1 < 0 && MinusX->tiles[15 - x][y][z].blockType == 0))
						{
							float brightness = currentBlock->lightLevel * 0.75f;
							std::vector<GLfloat> face = {
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness
							};
							chunkVertices.insert(chunkVertices.end(), face.begin(), face.end());
						}
						if ((x + 1 < chunkSize && tiles[x + 1][y][z].blockType == 0) || (x + 1 >= chunkSize && PlusX->tiles[15 - x][y][z].blockType == 0))
						{
							// Plus X
							float brightness = currentBlock->lightLevel * 0.75f;
							std::vector<GLfloat> face = {
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
							};
							chunkVertices.insert(chunkVertices.end(), face.begin(), face.end());
						}
						if ((y + 1 < chunkSize && tiles[x][y + 1][z].blockType == 0) || (y + 1 >= chunkSize && PlusY->tiles[x][15 - y][z].blockType == 0))
						{
							float brightness = currentBlock->lightLevel * 0.86f;
							std::vector<GLfloat> face = {
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness
							};
							chunkVertices.insert(chunkVertices.end(), face.begin(), face.end());
						}
						if ((y - 1 >= 0 && tiles[x][y - 1][z].blockType == 0) || (y - 1 < 0 && MinusY->tiles[x][15 - y][z].blockType == 0))
						{
							float brightness = currentBlock->lightLevel * 0.86f;
							std::vector<GLfloat> face = {
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.124f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								-0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.0f + currentBlock->blockOffset[1],
								brightness,
								-0.5f + blockCoords[0],
								-0.5f + blockCoords[1],
								0.5f + z,
								0.0f + currentBlock->blockOffset[0],
								0.124f + currentBlock->blockOffset[1],
								brightness
							};
							chunkVertices.insert(chunkVertices.end(), face.begin(), face.end());
						}
					}
				}
			}
		}
		savedVerticies = chunkVertices;
		hasVerticies = true;
		return chunkVertices;
	}
};