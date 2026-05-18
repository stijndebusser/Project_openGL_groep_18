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
#include "FrameBuffer.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
int PickNabooFighter(
	GLFWwindow* window,
	Shader& pickingShader,
	Model& nabooFighterModel,
	const glm::mat4& projection,
	const glm::mat4& view,
	const glm::mat4& nabooFighterMat);
bool useShipCamera = false;
bool tKeyPressed = false;
bool leftMouseButtonPressed = false;
int currentEffect = 0;

bool bloomEnabled = false;
bool bKeyPressed = false;

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int loadTexture(char const* path) {
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data) {
		GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else {
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}
	return textureID;
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

	glm::vec3 sunPos = glm::vec3(-20.0f, 10.0f, -30.0f);

	Shader modelShader("../../../shaders/model.vs", "../../../shaders/model.fs");
	Shader lightShader("../../../shaders/model.vs", "../../../shaders/lightsource.fs");
	Shader pickingShader("../../../shaders/model.vs", "../../../shaders/picking.fs");
	Shader chromaKeyShader("../../../shaders/chromakeyshader.vs", "../../../shaders/chromakeyshader.fs");
	Shader convolutionShader("../../../shaders/screen.vs", "../../../shaders/convolution.fs");
	Shader bloomShader("../../../shaders/screen.vs", "../../../shaders/bloom.fs");

	Model tieFighterModel("../../../resources/objects/tie_fighter/scene.gltf");
	Model nabooFighterModel("../../../resources/objects/naboo_fighter/scene.gltf");
	Model starDestroyerModel("../../../resources/objects/star_destroyer/scene.gltf");
	Model rocksModel("../../../resources/objects/rocks/3Drocks.obj");
	Model sunModel("../../../resources/objects/sun/scene.gltf");
	Model saturnModel("../../../resources/objects/saturn/scene.gltf");
	Model laserModel("../../../resources/objects/laser/scene.gltf");

	bool laserActive = false;
	float laserDistance = 0.0f;
	glm::vec3 laserPosition(0.0f);
	glm::vec3 laserDirection(0.0f);
	glm::mat4 laserOrientation = glm::mat4(1.0f);

	unsigned int greenScreenTexture = loadTexture("../../../resources/images/greenscreenmask.png");

	Framebuffer overlayQuad(SCR_WIDTH, SCR_HEIGHT);
	Framebuffer mainSceneFBO(SCR_WIDTH, SCR_HEIGHT);

	Framebuffer brightLightsFBO(SCR_WIDTH, SCR_HEIGHT);
	Framebuffer blurredLightsFBO(SCR_WIDTH, SCR_HEIGHT);

	glm::vec3 p0(10.0f, 0.0f, 10.0f); // start point
	glm::vec3 p1(10.0f, 3.0f, -10.0f); // control 1
	glm::vec3 p2(-10.0f, -3.0f, -10.0f); // control 2
	glm::vec3 p3(-10.0f, 0.0f, 10.0f); // end point

	glm::vec3 p4 = p3;
	glm::vec3 p5(-10.0f, 3.0f, 30.0f);
	glm::vec3 p6(10.0f, -3.0f, 30.0f);
	glm::vec3 p7 = p0;

	std::vector<glm::vec3> rockPath1 = Bezier::GenerateCurveForwardDifferencing(50, p0, p1, p2, p3);
	std::vector<glm::vec3> rockPath2 = Bezier::GenerateCurveForwardDifferencing(50, p4, p5, p6, p7);
	std::vector<glm::vec3> fullRockPath = rockPath1;

	fullRockPath.insert(fullRockPath.end(), rockPath2.begin(), rockPath2.end());


	std::vector<Bezier::LookupEntry> lookupTable1 =
		Bezier::GenerateDistanceLookupTable(1000, p0, p1, p2, p3);

	std::vector<Bezier::LookupEntry> lookupTable2 =
		Bezier::GenerateDistanceLookupTable(1000, p4, p5, p6, p7);

	float segment1Length = lookupTable1.back().distance;
	float segment2Length = lookupTable2.back().distance;
	float totalTrackLength = segment1Length + segment2Length;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		mainSceneFBO.Bind();
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float nearPlane = useShipCamera ? 0.05f : 0.1f;
		float farPlane = 5000.0f;

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
			float t1 = Bezier::GetTimeAtSpecificDistance(traveledDistance, lookupTable1);
			shipPosition = Bezier::CalculatePoint(t1, p0, p1, p2, p3);
			shipDirection = Bezier::CalculateLookingDirection(t1, p0, p1, p2, p3);
		}
		else
		{
			float distanceOnSegment2 = traveledDistance - segment1Length;
			float t2 = Bezier::GetTimeAtSpecificDistance(distanceOnSegment2, lookupTable2);
			shipPosition = Bezier::CalculatePoint(t2, p4, p5, p6, p7);
			shipDirection = Bezier::CalculateLookingDirection(t2, p4, p5, p6, p7);
		}
		glm::vec3 shipHeightOffset(0.0f, 1.5f, 0.0f);
		shipPosition += shipHeightOffset;

		glm::mat4 view;

		if (useShipCamera)
		{
			glm::vec3 forward = glm::normalize(shipDirection);
			glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

			glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
			glm::vec3 up = glm::normalize(glm::cross(right, forward));

			glm::vec3 cameraPosition =
				shipPosition
				- forward * 0.8f
				+ up * 0.25f
				- right * 0.5f;

			glm::vec3 cameraTarget = cameraPosition + forward * 10.0f;

			view = glm::lookAt(cameraPosition, cameraTarget, up);
		}
		else
		{
			view = camera.GetViewMatrix();
		}


		modelShader.use();
		modelShader.setMat4("projection", projection);
		modelShader.setMat4("view", view);

		modelShader.setVec3("viewPos", camera.Position);

		modelShader.setVec3("light.position", sunPos);
		modelShader.setVec3("light.ambient", 0.5f, 0.5f, 0.5f);
		modelShader.setVec3("light.diffuse", 5.0f, 5.0f, 5.0f);
		modelShader.setVec3("light.specular", 3.0f, 3.0f, 3.0f);

		modelShader.setFloat("light.constant", 1.0f);
		modelShader.setFloat("light.linear", 0.022f);
		modelShader.setFloat("light.quadratic", 0.0019f);

		glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

		glm::mat4 orientation = glm::inverse(glm::lookAt(
			glm::vec3(0.0f, 0.0f, 0.0f),
			-shipDirection,
			worldUp
		));  // - voor ship direction is quick fix direction


		glm::mat4 tieFighterModelMat = glm::mat4(1.0f);
		tieFighterModelMat = glm::translate(tieFighterModelMat, shipPosition);
		tieFighterModelMat *= orientation;
		tieFighterModelMat = glm::rotate(tieFighterModelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // anders wijst schip naar beneden
		tieFighterModelMat = glm::scale(tieFighterModelMat, glm::vec3(0.2f, 0.2f, 0.2f));

		// nabooFighter
		glm::mat4 nabooFighterMat = glm::mat4(1.0f);
		nabooFighterMat = glm::translate(nabooFighterMat, shipPosition + glm::normalize(shipDirection) * 5.0f);
		nabooFighterMat *= orientation;

		nabooFighterMat = glm::rotate(nabooFighterMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		nabooFighterMat = glm::rotate(nabooFighterMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		nabooFighterMat = glm::scale(nabooFighterMat, glm::vec3(0.3f, 0.3f, 0.3f));

		bool leftMouseButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		if (leftMouseButtonDown && !leftMouseButtonPressed)
		{
			int pickedID = PickNabooFighter(
				window,
				pickingShader,
				nabooFighterModel,
				projection,
				view,
				nabooFighterMat);

			if (pickedID == 1)
			{
				laserActive = true;
				laserDistance = 0.0f;
				laserPosition = glm::vec3(tieFighterModelMat * glm::vec4(2.65f, 1.1f, 3.35f, 1.0f));
				laserDirection = glm::normalize(glm::vec3(tieFighterModelMat * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
				laserOrientation = orientation;
			}

			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		leftMouseButtonPressed = leftMouseButtonDown;

		modelShader.use();

		modelShader.setMat4("model", tieFighterModelMat);
		tieFighterModel.Draw(modelShader);

		modelShader.setMat4("model", nabooFighterMat);
		nabooFighterModel.Draw(modelShader);


		// rocks
		modelShader.use();

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

			srand(static_cast<unsigned int>(i * 12345));

			int rocksPerSegment = 12;
			for (int r = 0; r < rocksPerSegment; r++)
			{
				float offsetX = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;
				float offsetY = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;
				float offsetZ = ((rand() % 200) / 100.0f - 1.0f) * 1.0f;

				glm::vec3 jitteredPos = currentPos + (localX * offsetX) + (localY * offsetY) + (localZ * offsetZ);

				float randomRotX = glm::radians((float)(rand() % 360));
				float randomRotY = glm::radians((float)(rand() % 360));
				float randomRotZ = glm::radians((float)(rand() % 360));

				glm::mat4 rollMat = glm::mat4(1.0f);
				rollMat = glm::rotate(rollMat, randomRotX, glm::vec3(1.0f, 0.0f, 0.0f));
				rollMat = glm::rotate(rollMat, randomRotY, glm::vec3(0.0f, 1.0f, 0.0f));
				rollMat = glm::rotate(rollMat, randomRotZ, glm::vec3(0.0f, 0.0f, 1.0f));

				float randScale = 0.15f + ((rand() % 100) / 100.0f) * 0.20f;

				glm::mat4 rockModelMat = glm::mat4(1.0f);
				rockModelMat = glm::translate(rockModelMat, jitteredPos);
				rockModelMat = rockModelMat * baseRotationMat * rollMat;
				rockModelMat = glm::scale(rockModelMat, glm::vec3(randScale, randScale, randScale));

				modelShader.setMat4("model", rockModelMat);
				rocksModel.Draw(modelShader);
			}
		}



		lightShader.use();
		lightShader.setMat4("projection", projection);
		lightShader.setMat4("view", view);

		float rotationSpeed = 0.05f;
		float rotationAngle = currentFrame * rotationSpeed;

		// sun
		glm::mat4 sunMat = glm::mat4(1.0f);
		sunMat = glm::translate(sunMat, sunPos);
		sunMat = glm::rotate(sunMat, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		sunMat = glm::scale(sunMat, glm::vec3(1.0f));
		lightShader.setMat4("model", sunMat);
		lightShader.setVec3("lightColor", glm::vec3(1.0f, 0.5f, 0.3f));
		lightShader.setFloat("intensity", 2.0f);
		sunModel.Draw(lightShader);

		// star destoyer
		modelShader.use();
		modelShader.setMat4("projection", projection);
		modelShader.setMat4("view", view);


		glm::mat4 starDestroyerMat = glm::mat4(1.0f);

		starDestroyerMat = glm::translate(starDestroyerMat, glm::vec3(0.0f, 10.0f, 55.0f));

		starDestroyerMat = glm::rotate(starDestroyerMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		starDestroyerMat = glm::rotate(starDestroyerMat, glm::radians(-25.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		starDestroyerMat = glm::scale(starDestroyerMat, glm::vec3(5.0f));

		modelShader.setMat4("model", starDestroyerMat);
		starDestroyerModel.Draw(modelShader);

		// saturn
		modelShader.use();
		glm::mat4 saturnMat = glm::mat4(1.0f);
		saturnMat = glm::translate(saturnMat, glm::vec3(-7.0f, 6.0f, 17.0f));
		saturnMat = glm::rotate(saturnMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		saturnMat = glm::scale(saturnMat, glm::vec3(7.0f));
		modelShader.setMat4("model", saturnMat);
		saturnModel.Draw(modelShader);

		if (laserActive)
		{
			float laserSpeed = 45.0f;
			laserDistance += laserSpeed * deltaTime;
			laserPosition += laserDirection * laserSpeed * deltaTime;

			glm::mat4 laserMat = glm::mat4(1.0f);
			laserMat = glm::translate(laserMat, laserPosition);
			laserMat *= laserOrientation;
			laserMat = glm::scale(laserMat, glm::vec3(0.04f, 0.04f, 0.15f));

			modelShader.setMat4("model", laserMat);
			laserModel.Draw(modelShader);

			if (laserDistance > 80.0f)
				laserActive = false;
		}
		mainSceneFBO.Unbind();

		if (bloomEnabled) {
			// isolate the lightsource
			brightLightsFBO.Bind();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // render sun only on a black surface
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lightShader.use();
			sunModel.Draw(lightShader);
			brightLightsFBO.Unbind();

			blurredLightsFBO.Bind();
			glDisable(GL_DEPTH_TEST);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			convolutionShader.use();
			convolutionShader.setInt("screenTexture", 0);
			convolutionShader.setInt("effectType", 1); // blur the sun a bit for the blooming effect

			blurredLightsFBO.Draw(convolutionShader.ID, brightLightsFBO.textureColorbuffer);
			blurredLightsFBO.Unbind();
			glEnable(GL_DEPTH_TEST);
		}

		glDisable(GL_DEPTH_TEST);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (bloomEnabled) { // here we actually do the adding processing in the bloom.fs shader
			bloomShader.use();
			bloomShader.setInt("scene", 0);
			bloomShader.setInt("bloomBlur", 1);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, blurredLightsFBO.textureColorbuffer);
			glActiveTexture(GL_TEXTURE0);

			mainSceneFBO.Draw(bloomShader.ID, mainSceneFBO.textureColorbuffer);
		}
		else {
			convolutionShader.use();
			convolutionShader.setInt("screenTexture", 0);
			convolutionShader.setInt("effectType", currentEffect);

			mainSceneFBO.Draw(convolutionShader.ID, mainSceneFBO.textureColorbuffer);
		}


		if (useShipCamera) {
			glDisable(GL_DEPTH_TEST); // ignore depth if helmet is on (2D)

			chromaKeyShader.use();
			chromaKeyShader.setInt("chromaKeyTexture", 0);
			overlayQuad.Draw(chromaKeyShader.ID, greenScreenTexture);

			glEnable(GL_DEPTH_TEST);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

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

int PickNabooFighter(
	GLFWwindow* window,
	Shader& pickingShader,
	Model& nabooFighterModel,
	const glm::mat4& projection,
	const glm::mat4& view,
	const glm::mat4& nabooFighterMat)
{
	int windowWidth = 0;
	int windowHeight = 0;
	int framebufferWidth = 0;
	int framebufferHeight = 0;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

	double mouseX = windowWidth / 2.0;
	double mouseY = windowHeight / 2.0;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	if (mouseX < 0.0 || mouseX >= windowWidth || mouseY < 0.0 || mouseY >= windowHeight)
	{
		mouseX = windowWidth / 2.0;
		mouseY = windowHeight / 2.0;
	}

	int pixelX = static_cast<int>(mouseX * framebufferWidth / windowWidth);
	int pixelY = framebufferHeight - static_cast<int>(mouseY * framebufferHeight / windowHeight) - 1;

	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pickingShader.use();
	pickingShader.setMat4("projection", projection);
	pickingShader.setMat4("view", view);
	pickingShader.setMat4("model", nabooFighterMat);
	pickingShader.setVec4("PickingColor", 1.0f / 255.0f, 0.0f, 0.0f, 1.0f);
	nabooFighterModel.Draw(pickingShader);

	glFlush();
	glFinish();

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	unsigned char data[4] = { 0, 0, 0, 0 };
	glReadPixels(pixelX, pixelY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glEnable(GL_BLEND);

	return data[0] + data[1] * 256 + data[2] * 256 * 256;
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

	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bKeyPressed)
	{
		bloomEnabled = !bloomEnabled;
		bKeyPressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
	{
		bKeyPressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		currentEffect = 0; // Normal
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		currentEffect = 1; // Gaussian blur
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		currentEffect = 2; // Laplacian edge highlighting

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
