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

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --------------------------------------------------
// Bezier helpers
// --------------------------------------------------

glm::vec3 EvaluateBezierPoint(
    const glm::vec3& p0,
    const glm::vec3& p1,
    const glm::vec3& p2,
    const glm::vec3& p3,
    float t)
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * p0
        + 3.0f * uu * t * p1
        + 3.0f * u * tt * p2
        + ttt * p3;
}

std::vector<glm::vec3> BuildCenterLine(
    int samplesPerSegment,
    const glm::vec3& p0,
    const glm::vec3& p1,
    const glm::vec3& p2,
    const glm::vec3& p3,
    const glm::vec3& p4,
    const glm::vec3& p5,
    const glm::vec3& p6,
    const glm::vec3& p7)
{
    std::vector<glm::vec3> points;

    for (int i = 0; i < samplesPerSegment; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(samplesPerSegment);
        points.push_back(EvaluateBezierPoint(p0, p1, p2, p3, t));
    }

    for (int i = 0; i < samplesPerSegment; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(samplesPerSegment);
        points.push_back(EvaluateBezierPoint(p4, p5, p6, p7, t));
    }

    return points;
}

glm::vec3 SampleCenterLine(const std::vector<glm::vec3>& path, float u)
{
    if (path.empty())
        return glm::vec3(0.0f);

    float wrapped = std::fmod(u, 1.0f);
    if (wrapped < 0.0f)
        wrapped += 1.0f;

    float scaled = wrapped * static_cast<float>(path.size());
    int i0 = static_cast<int>(std::floor(scaled)) % path.size();
    int i1 = (i0 + 1) % path.size();
    float localT = scaled - std::floor(scaled);

    return glm::mix(path[i0], path[i1], localT);
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

    // segment 1
    glm::vec3 p0(10.0f, 0.0f, 10.0f);
    glm::vec3 p1(10.0f, 3.0f, -10.0f);
    glm::vec3 p2(-10.0f, -3.0f, -10.0f);
    glm::vec3 p3(-10.0f, 0.0f, 10.0f);

    // segment 2
    glm::vec3 p4 = p3;
    glm::vec3 p5(-10.0f, 3.0f, 30.0f);
    glm::vec3 p6(10.0f, -3.0f, 30.0f);
    glm::vec3 p7 = p0;

    std::vector<float> track1 = Bezier::GenerateTrackMesh(50, 1.0f, p0, p1, p2, p3);
    std::vector<float> track2 = Bezier::GenerateTrackMesh(50, 1.0f, p4, p5, p6, p7);

    std::vector<float> fullTrack = track1;
    fullTrack.insert(fullTrack.end(), track2.begin(), track2.end());

    // centerline waar het schip echt over beweegt
    std::vector<glm::vec3> centerLine = BuildCenterLine(
        200,
        p0, p1, p2, p3,
        p4, p5, p6, p7
    );

    unsigned int railVBO, railVAO;
    glGenVertexArrays(1, &railVAO);
    glGenBuffers(1, &railVBO);

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

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT),
            0.1f,
            100.0f
        );
        glm::mat4 view = camera.GetViewMatrix();

     
        float shipSpeed = 0.08f;
        float progressOverPath = std::fmod(currentFrame * shipSpeed, 1.0f);
        glm::vec3 shipPosition = SampleCenterLine(centerLine, progressOverPath);

        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);


        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, shipPosition); // ipv eerdere vaste positie; modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.75f, 0.0f));

        modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); 


        modelMat = glm::scale(modelMat, glm::vec3(0.2f, 0.2f, 0.2f));

        modelShader.setMat4("model", modelMat);
        ourModel.Draw(modelShader);

        // -----------------------------
        // draw track
        // -----------------------------
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

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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