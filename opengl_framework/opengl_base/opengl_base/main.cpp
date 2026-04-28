#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <cmath>

#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "bezier.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
bool useShipCamera = false;
bool tKeyPressed = false;

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float GetTimeAtDistance(
	float distance,
	const std::vector<Bezier::LookupEntry>& lookupTable)
{
	if (lookupTable.empty())
		return 0.0f;

	if (distance <= lookupTable.front().distance)
		return lookupTable.front().t;

	if (distance >= lookupTable.back().distance)
		return lookupTable.back().t;

	for (size_t i = 0; i < lookupTable.size() - 1; ++i)
	{
		const Bezier::LookupEntry& a = lookupTable[i];
		const Bezier::LookupEntry& b = lookupTable[i + 1];

		if (distance >= a.distance && distance <= b.distance)
		{
			float localFactor = (distance - a.distance) / (b.distance - a.distance);
			return a.t + localFactor * (b.t - a.t);
		}
	}

	return lookupTable.back().t;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Starwars", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stbi_set_flip_vertically_on_load(true);

	Shader modelShader("../../../shaders/model.vs", "../../../shaders/model.fs");
	Shader trackShader("../../../shaders/7.4camera.vs", "../../../shaders/7.4camera.fs");
	Model ourModel("../../../resources/objects/tie_fighter/scene.gltf");
	Model rocksModel("../../../resources/objects/rocks/3Drocks.obj");

	glm::vec3 p0(10.0f, 0.0f, 10.0f); // start point
	glm::vec3 p1(10.0f, 3.0f, -10.0f); // control 1
	glm::vec3 p2(-10.0f, -3.0f, -10.0f); // control 2
	glm::vec3 p3(-10.0f, 0.0f, 10.0f); // end point

	glm::vec3 p4 = p3;
	glm::vec3 p5(-10.0f, 3.0f, 30.0f);
	glm::vec3 p6(10.0f, -3.0f, 30.0f);
	glm::vec3 p7 = p0;

	std::vector<float> track1 = Bezier::GenerateTrackMesh(50, 1.0f, p0, p1, p2, p3);
	std::vector<float> track2 = Bezier::GenerateTrackMesh(50, 1.0f, p4, p5, p6, p7);

	std::vector<float> fullTrack = track1;
	fullTrack.insert(fullTrack.end(), track2.begin(), track2.end());

	std::vector<glm::vec3> rockPath1 = Bezier::GenerateCurveForwardDifferencing(50, p0, p1, p2, p3);
	std::vector<glm::vec3> rockPath2 = Bezier::GenerateCurveForwardDifferencing(50, p4, p5, p6, p7);
	std::vector<glm::vec3> fullRockPath = rockPath1;

	fullRockPath.insert(fullRockPath.end(), rockPath2.begin(), rockPath2.end());

	unsigned int railVBO, railVAO;
	glGenVertexArrays(1, &railVAO);
	glGenBuffers(1, &railVBO);

	std::vector<Bezier::LookupEntry> lookupTable1 =
		Bezier::GenerateDistanceLookupTable(1000, p0, p1, p2, p3);

	std::vector<Bezier::LookupEntry> lookupTable2 =
		Bezier::GenerateDistanceLookupTable(1000, p4, p5, p6, p7);

	float segment1Length = lookupTable1.back().distance;
	float segment2Length = lookupTable2.back().distance;
	float totalTrackLength = segment1Length + segment2Length;

	glBindVertexArray(railVAO);
	glBindBuffer(GL_ARRAY_BUFFER, railVBO);
	glBufferData(GL_ARRAY_BUFFER, fullTrack.size() * sizeof(float), fullTrack.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	unsigned int trackTexture;
	glGenTextures(1, &trackTexture);
	glBindTexture(GL_TEXTURE_2D, trackTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char* data = stbi_load("../../../textures/meteor-shower-transparent.png", &width, &height, &nrChannels, 0);

	if (data)
	{
		GLenum format = GL_RGB;
		if (nrChannels == 4) format = GL_RGBA;
		else if (nrChannels == 1) format = GL_RED;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	trackShader.use();
	trackShader.setInt("trackTexture", 0);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float nearPlane = useShipCamera ? 0.6f : 0.1f;
		float farPlane = 200.0f;

		glm::mat4 projection = glm::perspective(
			glm::radians(camera.Zoom),
			static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT),
			nearPlane,
			farPlane
		);

		float shipSpeed = 5.0f;
		float traveledDistance = std::fmod(currentFrame * shipSpeed, totalTrackLength);

		glm::vec3 shipDirection;
		glm::vec3 shipPosition;

		if (traveledDistance < segment1Length)
		{
			float t1 = GetTimeAtDistance(traveledDistance, lookupTable1);
			shipPosition = Bezier::CalculatePoint(t1, p0, p1, p2, p3);
			shipDirection = Bezier::CalculateLookingDirection(t1, p0, p1, p2, p3);
		}
		else
		{
			float distanceOnSegment2 = traveledDistance - segment1Length;
			float t2 = GetTimeAtDistance(distanceOnSegment2, lookupTable2);
			shipPosition = Bezier::CalculatePoint(t2, p4, p5, p6, p7);
			shipDirection = Bezier::CalculateLookingDirection(t2, p4, p5, p6, p7);
		}

		glm::mat4 view;

		if (useShipCamera)
		{
			glm::vec3 cameraOffset = -shipDirection * 4.0f + glm::vec3(0.0f, 2.0f, 0.0f);
			glm::vec3 cameraPosition = shipPosition + cameraOffset;
			glm::vec3 cameraTarget = shipPosition + shipDirection * 5.0f;

			view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else
		{
			view = camera.GetViewMatrix();
		}


		modelShader.use();
		modelShader.setMat4("projection", projection);
		modelShader.setMat4("view", view);

		glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

		glm::mat4 orientation = glm::inverse(glm::lookAt(
			glm::vec3(0.0f, 0.0f, 0.0f),
			-shipDirection,
			worldUp
		));  // - voor ship direction is quick fix direction

		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, shipPosition);
		modelMat *= orientation;
		modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // anders wijst schip naar beneden 
		modelMat = glm::scale(modelMat, glm::vec3(0.2f, 0.2f, 0.2f));

		modelShader.setMat4("model", modelMat);
		ourModel.Draw(modelShader);

		// rocks

		for (size_t i = 0; i < fullRockPath.size() - 1; i++)
		{
			glm::vec3 currentPos = fullRockPath[i];
			glm::vec3 nextPos = fullRockPath[i + 1];

			glm::vec3 localX = glm::normalize(nextPos - currentPos);
			glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 localZ = glm::normalize(glm::cross(localX, worldUp));
			glm::vec3 localY = glm::normalize(glm::cross(localZ, localX));

			glm::mat4 baseRotationMat(1.0f);
			baseRotationMat[0] = glm::vec4(localX, 0.0f);
			baseRotationMat[1] = glm::vec4(localY, 0.0f);
			baseRotationMat[2] = glm::vec4(localZ, 0.0f);

			// Seed the random generator using the index so rocks stay stationary between frames
			srand(static_cast<unsigned int>(i * 12345));

			// INCREASE THIS NUMBER to spawn more rocks per segment
			int rocksPerSegment = 12;
			for (int r = 0; r < rocksPerSegment; r++)
			{
				// Widen the spread so they don't clip into each other as much
				// (Change 1.5f, 1.5f, 4.0f to make the cloud wider or tighter)
				float offsetX = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;
				float offsetY = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;
				float offsetZ = ((rand() % 200) / 100.0f - 1.0f) * 1.0f;

				glm::vec3 jitteredPos = currentPos + (localX * offsetX) + (localY * offsetY) + (localZ * offsetZ);

				// Random spin (roll) around all 3 axes for complete randomization!
				float randomRotX = glm::radians((float)(rand() % 360));
				float randomRotY = glm::radians((float)(rand() % 360));
				float randomRotZ = glm::radians((float)(rand() % 360));

				glm::mat4 rollMat = glm::mat4(1.0f);
				rollMat = glm::rotate(rollMat, randomRotX, glm::vec3(1.0f, 0.0f, 0.0f));
				rollMat = glm::rotate(rollMat, randomRotY, glm::vec3(0.0f, 1.0f, 0.0f));
				rollMat = glm::rotate(rollMat, randomRotZ, glm::vec3(0.0f, 0.0f, 1.0f));

				// Random scale to make rocks vary in size
				float randScale = 0.15f + ((rand() % 100) / 100.0f) * 0.20f;

				glm::mat4 rockModelMat = glm::mat4(1.0f);
				rockModelMat = glm::translate(rockModelMat, jitteredPos);
				rockModelMat = rockModelMat * baseRotationMat * rollMat;
				rockModelMat = glm::scale(rockModelMat, glm::vec3(randScale, randScale, randScale));

				modelShader.setMat4("model", rockModelMat);
				rocksModel.Draw(modelShader);
			}
		}

		trackShader.use();
		trackShader.setMat4("projection", projection);
		trackShader.setMat4("view", view);

		glm::mat4 trackModelMat = glm::mat4(1.0f);
		trackShader.setMat4("model", trackModelMat);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, trackTexture);

		glBindVertexArray(railVAO);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(fullTrack.size() / 5));

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	trackShader.use();
	trackShader.setInt("trackTexture", 0);
	modelShader.use();

	glfwTerminate();
	return 0;

}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed)
	{
		useShipCamera = !useShipCamera;
		tKeyPressed = true;
		firstMouse = true;
	}

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
		tKeyPressed = false;

	if (useShipCamera)
		return;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}
