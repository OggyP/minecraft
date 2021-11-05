#define UNUSED(x) (void)(x)
#include "Platform/Platform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Vectors.hpp"
#include "openGLsetup.hpp"
#include "terrain.hpp"

// // WINDOWS IS STUPID
// # define M_E		2.7182818284590452354	/* e */
// # define M_LOG2E	1.4426950408889634074	/* log_2 e */
// # define M_LOG10E	0.43429448190325182765	/* log_10 e */
// # define M_LN2		0.69314718055994530942	/* log_e 2 */
// # define M_LN10		2.30258509299404568402	/* log_e 10 */
// # define M_PI		3.14159265358979323846	/* pi */
#define M_PI_2 1.57079632679489661923 /* pi/2 */
// # define M_PI_4		0.78539816339744830962	/* pi/4 */
// # define M_1_PI		0.31830988618379067154	/* 1/pi */
// # define M_2_PI		0.63661977236758134308	/* 2/pi */
// # define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
// # define M_SQRT2	1.41421356237309504880	/* sqrt(2) */
// # define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

int screenSize[2] = { 1680, 1050 };
float position[3] = { 0.0f, 0.0f, 150.0f };
Vector3f wantToLook;
Vector3f lookingAt;

bool gameRunning = true;
bool windowHasFocus = true;

// render distance in chunks
const int renderDistance = 5;
const int chunksAmt = (2 * renderDistance + 1);

float blockScale = 0.1;

const int GPUchunkUploadLimit = 10000000;

bool spawnChunksLoaded = false;

// map of chunks
sf::Mutex ChunkMapMutex;
std::map<int, std::map<int, GameChunk>> chunkMap;
std::map<int, std::map<int, GameChunk>>::iterator chunkXitr;
std::map<int, GameChunk>::iterator chunkYitr;

// map of chunk verticies
sf::Mutex ChunkVerticiesMutex;
std::vector<std::pair<std::array<int, 2>, std::vector<GLfloat>>> newVerticies;

std::map<int, std::map<int, std::array<GLuint, 2>>> bufferMap;

// SHADERS =========================================
// Vertex
const GLchar* sceneVertexSource = R"glsl(
    #version 150 core

    in vec3 position;
    in vec2 texcoord;
	in float brightness;

    out vec2 texCoord;
	out float Brightness;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;
	uniform float scale;

    void main()
    {
        gl_Position =  proj * view * model * vec4(position * scale, 1.0);
        texCoord = texcoord;
		Brightness = brightness;
    }
)glsl";
// Fragment
const GLchar* sceneFragmentSource = R"glsl(
    #version 150 core

    in vec2 texCoord;
	in float Brightness;

    out vec4 outColor;

	uniform sampler2D blockTexture;

    void main()
    {
        outColor = texture(blockTexture, texCoord) * Brightness;
    }
)glsl";
// ==== Screen ---
const GLchar* screenVertexSource = R"glsl(
    #version 150 core
    in vec2 position;
    in vec2 texcoord;
    out vec2 Texcoord;
    void main()
    {
        Texcoord = texcoord;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";
const GLchar* screenFragmentSource = R"glsl(
    #version 150 core
    in vec2 Texcoord;

    out vec4 outColor;

    uniform sampler2D texFramebuffer;

    void main()
	{
		outColor = texture(texFramebuffer, Texcoord) * 1.5;
    }
)glsl";
//============================

#include "verticies.txt"

void renderingThread(sf::Window* window);
void renderingThread(sf::Window* window)
{
	// activate the window's context
	window->setActive(true);

	// GLEW stuff
	glewExperimental = GL_TRUE;
	glewInit();

	// Create VAO
	GLuint vaoWorld, vaoQuad;
	glGenVertexArrays(1, &vaoWorld);
	glGenVertexArrays(1, &vaoQuad);

	glBindVertexArray(vaoWorld);

	// Load vertex data
	GLuint vboQuad, vboChunks[(2 * renderDistance + 1) * (2 * renderDistance + 1)];
	glGenBuffers((2 * renderDistance + 1) * (2 * renderDistance + 1), &vboChunks[0]);
	glGenBuffers(1, &vboQuad);

	glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Create shader programs
	GLuint sceneVertexShader, sceneFragmentShader, sceneShaderProgram;
	createShaderProgram(sceneVertexSource, sceneFragmentSource, sceneVertexShader, sceneFragmentShader, sceneShaderProgram);

	glBindVertexArray(vaoWorld);
	for (int i = 0; i < (2 * renderDistance + 1) * (2 * renderDistance + 1); i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboChunks[i]);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		//drawArrays
	}

	// Specify the layout of the vertex data
	glBindVertexArray(vaoWorld);
	specifySceneVertexAttributes(sceneShaderProgram);

	// Load the one texture
	// GLuint blockTexture = loadTexture("./content/block.png");
	GLuint blockTexture;
	glGenTextures(1, &blockTexture);

	sf::Image imageLoad;
	if (imageLoad.loadFromFile("./content/block.png"))
	{
		glBindTexture(GL_TEXTURE_2D, blockTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageLoad.getSize().x, imageLoad.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageLoad.getPixelsPtr());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	// End load image

	glUseProgram(sceneShaderProgram);
	glUniform1i(glGetUniformLocation(sceneShaderProgram, "texCat"), 0);
	// Create framebuffer ==============================================================================================
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	GLuint screenVertexShader, screenFragmentShader, screenShaderProgram;
	createShaderProgram(screenVertexSource, screenFragmentSource, screenVertexShader, screenFragmentShader, screenShaderProgram);

	glBindVertexArray(vaoQuad);
	glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
	specifyScreenVertexAttributes(screenShaderProgram);

	glUseProgram(screenShaderProgram);
	glUniform1i(glGetUniformLocation(screenShaderProgram, "blockTexture"), 0);

	GLuint frameBuffer;
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// Create texture to hold color buffer
	GLuint texColorBuffer;
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenSize[0], screenSize[1], 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

	// Create Renderbuffer Object to hold depth and stencil buffers
	GLuint rboDepthStencil;
	glGenRenderbuffers(1, &rboDepthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenSize[0], screenSize[1]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//==================================================================================================================

	// setup projection
	glUseProgram(sceneShaderProgram);

	GLint uniView = glGetUniformLocation(sceneShaderProgram, "view");

	glm::mat4 proj = glm::perspective(glm::radians(90.0f), (float)screenSize[0] / screenSize[1], 0.0001f, 100.0f);
	GLint uniProj = glGetUniformLocation(sceneShaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	GLint uniModel = glGetUniformLocation(sceneShaderProgram, "model");
	glm::mat4 model = glm::mat4(0.1f);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	glUniform1f(glGetUniformLocation(sceneShaderProgram, "scale"), blockScale);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	std::array<std::array<int, 3>, chunksAmt * chunksAmt> chunkIds;
	int chunkVetexSizes[chunksAmt * chunksAmt];
	for (int i = 0; i < chunksAmt * chunksAmt; i++)
	{
		chunkIds[i] = { 0, 0, 0 };
	}
	// std::fill_n(chunkIds, chunksAmt * chunksAmt, defaultVal);

	// the rendering loop
	int totalFrameCount = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
	bool logFPStoConsole = false;

	sf::Event event;
	while (gameRunning)
	{
		if (spawnChunksLoaded && !logFPStoConsole)
		{
			startTime = std::chrono::high_resolution_clock::now();
			logFPStoConsole = true;
		}
		while (window->pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window->close();
			else if (event.type == sf::Event::GainedFocus)
				windowHasFocus = true;
			else if (event.type == sf::Event::LostFocus)
				windowHasFocus = false;
			else if (event.type == sf::Event::Resized)
			{
				// update the view to the new size of the window
				screenSize[0] = event.size.width;
				screenSize[1] = event.size.height;
				glViewport(0, 0, event.size.width, event.size.height);
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) || event.type == sf::Event::Closed)
			{
				gameRunning = false;
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		// Clear the screen to white
		glClearColor(0.450, 0.937, 0.968, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Bind our framebuffer and draw 3D scene (spinning cube)

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blockTexture);

		glBindVertexArray(vaoWorld);
		glEnable(GL_DEPTH_TEST);
		glUseProgram(sceneShaderProgram);

		float scaledPlayerPos[3] = { position[0] * blockScale, position[1] * blockScale, position[2] * blockScale };

		glm::mat4 view = glm::lookAt(
			glm::vec3(scaledPlayerPos[0], scaledPlayerPos[1], scaledPlayerPos[2]),
			glm::vec3(lookingAt.x + scaledPlayerPos[0], lookingAt.y + scaledPlayerPos[1], lookingAt.z + scaledPlayerPos[2]),
			glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		const int currentChunk[2] = { (int)floor(position[0] / chunkSize), (int)floor(position[1] / chunkSize) };
		const int chunkBoundaries[2][2] = { { currentChunk[0] - renderDistance, currentChunk[0] + renderDistance }, { currentChunk[1] - renderDistance, currentChunk[1] + renderDistance } };
		// Draw cube
		// Render frame

		// Used to cap the max VBO uploads per frame
		int VBOuploads = 0;

		for (int i = 0; i < chunksAmt * chunksAmt; i++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vboChunks[i]);
			glEnableVertexAttribArray(vboChunks[i]);
			// x, y, draw (bool)
			if (!chunkIds[i][2] || !(chunkIds[i][0] >= chunkBoundaries[0][0] && chunkIds[i][0] <= chunkBoundaries[0][1] && chunkIds[i][1] >= chunkBoundaries[1][0] && chunkIds[i][1] <= chunkBoundaries[1][1]))
			{
				ChunkVerticiesMutex.lock();
				int vectorSize = newVerticies.size();
				if (vectorSize > 0 && VBOuploads < GPUchunkUploadLimit)
				{
					auto newChunkInfo = newVerticies[vectorSize - 1];
					newVerticies.pop_back();
					if (newChunkInfo.first[0] >= chunkBoundaries[0][0] && newChunkInfo.first[0] <= chunkBoundaries[0][1] && newChunkInfo.first[1] >= chunkBoundaries[1][0] && newChunkInfo.first[1] <= chunkBoundaries[1][1])
					{
						VBOuploads++;
						chunkIds[i][2] = true;
						chunkIds[i][0] = newChunkInfo.first[0];
						chunkIds[i][1] = newChunkInfo.first[1];

						glBufferData(GL_ARRAY_BUFFER, sizeof(float) * newChunkInfo.second.size(), newChunkInfo.second.data(), GL_DYNAMIC_DRAW);
						chunkVetexSizes[i] = newChunkInfo.second.size() / 6;
					}
				}
				ChunkVerticiesMutex.unlock();
			}
			if (chunkIds[i][2])
			{
				specifySceneVertexAttributes(sceneShaderProgram);
				glDrawArrays(GL_TRIANGLES, 0, chunkVetexSizes[i]);
			}
		}

		// Bind default framebuffer and draw contents of our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindVertexArray(vaoQuad);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(screenShaderProgram);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texColorBuffer);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		// end the current frame -- this is a rendering function (it requires the context to be active)
		window->display();
		if (logFPStoConsole)
		{
			totalFrameCount++;
			auto now = std::chrono::high_resolution_clock::now();
			float timeDiff = std::chrono::duration_cast<std::chrono::duration<float>>(now - startTime).count();
			UNUSED(timeDiff);
			// std::cout << "Frames: " << totalFrameCount / timeDiff << "\n";
		}
	}

	// close
	glDeleteRenderbuffers(1, &rboDepthStencil);
	glDeleteTextures(1, &texColorBuffer);
	glDeleteFramebuffers(1, &frameBuffer);

	glDeleteTextures(1, &blockTexture);

	glDeleteProgram(screenShaderProgram);
	glDeleteShader(screenFragmentShader);
	glDeleteShader(screenVertexShader);

	glDeleteProgram(sceneShaderProgram);
	glDeleteShader(sceneFragmentShader);
	glDeleteShader(sceneVertexShader);

	for (int i = 0; i < chunksAmt * chunksAmt; i++)
	{
		glDeleteBuffers(1, &vboChunks[i]);
	}

	glDeleteVertexArrays(1, &vaoWorld);

	newVerticies.clear();

	glDeleteBuffers(1, &vboQuad);

	glDeleteVertexArrays(1, &vaoQuad);
}

void chunkGenThread();
void chunkGenThread()
{
	bool firstLoad = true;
	int previousChunk[2] = { 0, 0 };
	bool updateChunks = true;
	while (gameRunning)
	{
		const int currentChunk[2] = { (int)floor(position[0] / chunkSize), (int)floor(position[1] / chunkSize) };
		if ((currentChunk[0] != previousChunk[0] || currentChunk[1] != previousChunk[1]) || updateChunks)
		{
			// [[lower bounds x, higher bounds x], [lower bounds y, higher bounds y]] (inclusive)
			const int currentChunkConstraints[2][2] = { { currentChunk[0] - renderDistance, currentChunk[0] + renderDistance }, { currentChunk[1] - renderDistance, currentChunk[1] + renderDistance } };
			const int previousChunkConstraints[2][2] = { { previousChunk[0] - renderDistance, previousChunk[0] + renderDistance }, { previousChunk[1] - renderDistance, previousChunk[1] + renderDistance } };
			updateChunks = false;
			previousChunk[0] = currentChunk[0];
			previousChunk[1] = currentChunk[1];
			const int chunkOffsets[2] = { -(currentChunk[0] - renderDistance) + 1, -(currentChunk[1] - renderDistance) + 1 };
			bool haveChunk[chunksAmt + 2][chunksAmt + 2] = { 0 };
			GameChunk* chunkPointers[chunksAmt + 2][chunksAmt + 2];

			ChunkMapMutex.lock();
			for (chunkXitr = chunkMap.begin(); chunkXitr != chunkMap.end(); chunkXitr++)
			{
				if (chunkXitr->first >= currentChunkConstraints[0][0] - 1 && chunkXitr->first <= currentChunkConstraints[0][1] + 1)
				{
					for (chunkYitr = chunkXitr->second.begin(); chunkYitr != chunkXitr->second.end(); chunkYitr++)
					{
						if (chunkYitr->first >= currentChunkConstraints[1][0] - 1 && chunkYitr->first <= currentChunkConstraints[1][0] + 1)
						{
							haveChunk[chunkXitr->first + chunkOffsets[0]][chunkYitr->first + chunkOffsets[1]] = true;
							chunkPointers[chunkXitr->first + chunkOffsets[0]][chunkYitr->first + chunkOffsets[1]] = &chunkYitr->second;
						}
					}
				}
			}
			ChunkMapMutex.unlock();
			for (int idxX = 0; idxX < 2 * renderDistance + 3 && gameRunning; idxX++)
			{
				for (int idxY = 0; idxY < 2 * renderDistance + 3 && gameRunning; idxY++)
				{
					if (!haveChunk[idxX][idxY])
					{
						GameChunk newChunk(idxX - chunkOffsets[0], idxY - chunkOffsets[1]); // Make new chunk
						// std::vector<GLfloat> newChunkVerticies = newChunk.genVerticies();
						ChunkMapMutex.lock();
						chunkMap.insert(std::make_pair(newChunk.chunkX, std::map<int, GameChunk>()));			// Inset y map to make sure it exists
						chunkMap[newChunk.chunkX].insert(std::make_pair(newChunk.chunkY, std::move(newChunk))); // insert chunk
						chunkPointers[idxX][idxY] = &chunkMap[newChunk.chunkX][newChunk.chunkY];
						ChunkMapMutex.unlock();
					}
				}
			}
			for (int idxX = 1; idxX < 2 * renderDistance + 2 && gameRunning; idxX++)
			{
				for (int idxY = 1; idxY < 2 * renderDistance + 2 && gameRunning; idxY++)
				{
					if (!chunkPointers[idxX][idxY]->hasVerticies)
					{
						std::vector<GLfloat> newChunkVerticies = chunkPointers[idxX][idxY]->genVerticies(chunkPointers[idxX + 1][idxY], chunkPointers[idxX - 1][idxY], chunkPointers[idxX][idxY + 1], chunkPointers[idxX][idxY - 1]);
						ChunkVerticiesMutex.lock();
						newVerticies.push_back(std::make_pair(std::array<int, 2>({ idxX - chunkOffsets[0], idxY - chunkOffsets[1] }), newChunkVerticies));
						ChunkVerticiesMutex.unlock();
					}
					else if (idxX - chunkOffsets[0] < previousChunkConstraints[0][0] || idxX - chunkOffsets[0] > previousChunkConstraints[0][1] || idxY - chunkOffsets[1] < previousChunkConstraints[1][0] || idxY - chunkOffsets[1] > previousChunkConstraints[1][1])
					{
						ChunkVerticiesMutex.lock();
						newVerticies.push_back(std::make_pair(std::array<int, 2>({ idxX - chunkOffsets[0], idxY - chunkOffsets[1] }), chunkPointers[idxX][idxY]->savedVerticies));
						ChunkVerticiesMutex.unlock();
					}
				}
			}
			// UNUSED(chunkPointers);
		}
		if (firstLoad)
		{
			std::cout << "Loaded All init chunks\n";
			spawnChunksLoaded = true;
		}
		firstLoad = false;
		sf::sleep(sf::milliseconds(1));
	}
}

int previousChunkCollide[2] = { 0, 0 };
GameChunk* savedChunk;
bool collisionChunkIsLoaded = false;

/**
 * World collision check
 *
 * @param[in] x float world coordinate
 * @param[in] y float world coordinate
 * @param[in] z float world coordinate
 *
 * @return Boolean value, true if that point is NOT inside of a block. False if the chunk has not been generated.
 */
bool pointCollided(float x, float y, float z);
bool pointCollided(float x, float y, float z)
{
	int currentChunk[2] = { (int)floor(x / chunkSize), (int)floor(y / chunkSize) };
	int currentBlock[3] = { (int)floor((int)floor(x) % chunkSize), (int)floor((int)floor(y) % chunkSize), (int)floor(z - 1) };

	if (currentBlock[0] < 0)
	{
		currentBlock[0] += chunkSize;
	}

	if (currentBlock[1] < 0)
	{
		currentBlock[1] += chunkSize;
	}

	if (previousChunkCollide[0] != currentChunk[0] || previousChunkCollide[1] != currentChunk[1] || spawnChunksLoaded)
	{
		spawnChunksLoaded = false;
		bool newChunkLoaded = false;
		ChunkMapMutex.lock();
		for (chunkXitr = chunkMap.begin(); chunkXitr != chunkMap.end(); chunkXitr++)
		{
			if (chunkXitr->first == currentChunk[0])
			{
				for (chunkYitr = chunkXitr->second.begin(); chunkYitr != chunkXitr->second.end(); chunkYitr++)
				{
					if (chunkYitr->first == currentChunk[1])
					{
						savedChunk = &chunkYitr->second;
						previousChunkCollide[0] = currentChunk[0];
						previousChunkCollide[1] = currentChunk[1];
						collisionChunkIsLoaded = true;
						newChunkLoaded = true;
					}
				}
			}
		}
		if (newChunkLoaded)
		{
			collisionChunkIsLoaded = true;
		}
		else
		{
			collisionChunkIsLoaded = false;
		}
		ChunkMapMutex.unlock();
	}
	if (collisionChunkIsLoaded)
	{
		if (savedChunk->tiles[currentBlock[0]][currentBlock[1]][currentBlock[2]] == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

int main()
{
	util::Platform platform;

#if defined(_DEBUG)
	std::cout << "Hello World!" << std::endl;
#endif
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 2; // Optional
	// Request OpenGL version 3.2
	settings.majorVersion = 3;
	settings.minorVersion = 2;
	settings.attributeFlags = sf::ContextSettings::Core;

	sf::Window window(sf::VideoMode(screenSize[0], screenSize[1]), "Bad Minecraft", sf::Style::Resize | sf::Style::Close, settings);
	platform.setIcon(window.getSystemHandle());

	window.setMouseCursorGrabbed(true);
	window.setMouseCursorVisible(false);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	screenSize[0] = window.getSize().x;
	screenSize[1] = window.getSize().y;

	window.setActive(false);
	// window.setPosition(sf::Vector2i(0, 0));

	// launch the rendering thread
	sf::Thread renderThread(&renderingThread, &window);
	renderThread.launch();

	// Launch chunk gen thread
	sf::Thread chunkThread(&chunkGenThread);
	chunkThread.launch();

	int mouseCoord[2];
	float mouseSensitivity = 0.001;
	float mouseSmoothing = 50.0f;
	float movementSpeed = 0.07f; // Should be blocks / second

	const float playerDimensions[3] = { 0.3, 0.3, 1.8 }; // x radius, y radius, height
	bool onFloor = false;
	auto timeSinceLastJump = std::chrono::high_resolution_clock::now();
	bool flying = false;

	Vector3f movementVector;

	// Constants
	const float gravity = 0.0093f;
	const float jumpHeight = 2.0f;
	// Handle all input
	sf::Clock deltaClock;
	while (gameRunning)
	{
		sf::Time dt = deltaClock.restart();
		float deltaTimeMovementSpeed = movementSpeed * (float)dt.asMicroseconds() / 10000.0f;
		float deltaTimeMouseSmoothing = mouseSmoothing * (float)dt.asSeconds();
		if (deltaTimeMouseSmoothing > 0.05f)
			deltaTimeMouseSmoothing = 0.05f;

		bool cPressed = false;

		if (windowHasFocus)
		{
			mouseCoord[0] = sf::Mouse::getPosition(window).x;
			mouseCoord[1] = sf::Mouse::getPosition(window).y;

			if (mouseCoord[0] != screenSize[0] / 2 && mouseCoord[1] != screenSize[1] / 2)
			{
				// Mouse has moved
				wantToLook.directionH -= (mouseCoord[0] - screenSize[0] / 2) * mouseSensitivity;
				wantToLook.directionV -= (mouseCoord[1] - screenSize[1] / 2) * mouseSensitivity;
				if (wantToLook.directionV > M_PI_2 - 0.1)
				{
					wantToLook.directionV = M_PI_2 - 0.1;
				}
				if (wantToLook.directionV < -M_PI_2 + 0.1)
				{
					wantToLook.directionV = -M_PI_2 + 0.1;
				}

				wantToLook.updateCoords(10);

				// Move mouse back to the middle of the screen
				sf::Mouse::setPosition(sf::Vector2i(screenSize[0] / 2, screenSize[1] / 2), window);
			}

			lookingAt.x += (wantToLook.x - lookingAt.x) * deltaTimeMouseSmoothing;
			lookingAt.y += (wantToLook.y - lookingAt.y) * deltaTimeMouseSmoothing;
			lookingAt.z += (wantToLook.z - lookingAt.z) * deltaTimeMouseSmoothing;

			Vector2f wantToMove;

			// XY Movement
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			{
				wantToMove.x += cos(wantToLook.directionH);
				wantToMove.y += sin(wantToLook.directionH);
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			{
				wantToMove.x += -cos(wantToLook.directionH);
				wantToMove.y += -sin(wantToLook.directionH);
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			{
				wantToMove.x += cos(wantToLook.directionH + M_PI_2);
				wantToMove.y += sin(wantToLook.directionH + M_PI_2);
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			{
				wantToMove.x += -cos(wantToLook.directionH + M_PI_2);
				wantToMove.y += -sin(wantToLook.directionH + M_PI_2);
			}

			// Flying and jumping
			if (!flying)
			{
				movementVector.z -= gravity * dt.asMilliseconds();
				if (onFloor && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
				{
					auto now = std::chrono::high_resolution_clock::now();
					float timeDiff = std::chrono::duration_cast<std::chrono::duration<float>>(now - timeSinceLastJump).count();
					if (timeDiff > 0.03f)
					{
						movementVector.z = jumpHeight;
						timeSinceLastJump = std::chrono::high_resolution_clock::now();
					}
				}
			}
			else
			{
				movementVector.z = 0;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
					movementVector.z += deltaTimeMovementSpeed * 100;
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
					movementVector.z -= deltaTimeMovementSpeed * 100;
			}

			// Other keyboard checks
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
			{
				if (!cPressed)
					flying = !flying;
				cPressed = true;
				sf::sleep(sf::milliseconds(5));
			}
			else
				cPressed = false;

			if (onFloor)
			{
				movementVector.x = wantToMove.x;
				movementVector.y = wantToMove.y;
			}
			else
			{
				movementVector.x += wantToMove.x * (float)dt.asMicroseconds() / 200000.0f;
				movementVector.y += wantToMove.y * (float)dt.asMicroseconds() / 200000.0f;
			}

			if (movementVector.getMagnitudeXYSqr() > 1.0f)
				movementVector.normaliseVectorXY(1.0);

			if (movementVector.x != 0.0 || movementVector.y != 0.0f || movementVector.z != 0.0f)
				for (int i = 0; i < 3; i++)
				{
					float directionValue = movementVector.getIndex(i) * deltaTimeMovementSpeed;
					float newPos[3] = { position[0], position[1], position[2] };
					newPos[i] += directionValue;

					bool notCollided = true;
					// Check each of the player's collision points
					for (float x = -playerDimensions[0]; x <= playerDimensions[0] && notCollided; x += 2 * playerDimensions[0])
						for (float y = -playerDimensions[1]; y <= playerDimensions[1] && notCollided; y += 2 * playerDimensions[1])
							for (float z = 0; z <= playerDimensions[2] && notCollided; z += playerDimensions[2] / 2)
								notCollided = pointCollided(newPos[0] + 0.5f + x, newPos[1] + 0.5f + y, newPos[2] + z);

					if (notCollided)
						position[i] += directionValue;
					else
						movementVector.setIndex(i, 0.0f);

					if (i == 2)
					{
						if (!notCollided)
						{
							movementVector.z = 0;
							if (directionValue < 0 && collisionChunkIsLoaded)
								onFloor = true;
							else
							{
								onFloor = false;
							}
						}
						else
						{
							onFloor = false;
						}
					}
				}
			sf::sleep(sf::milliseconds(1));
		}
	}

	renderThread.wait();
	chunkThread.wait();

	return 0;
}
