#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include "stb_image.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include <cmath>
#include <vector>   
#include <ctime>

// window
gps::Window myWindow;

// matrices
glm::mat4 view;
glm::mat4 projection;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
GLint cameraPosLoc;

glm::mat4 lightSpaceMatrix;
GLint lightSpaceMatrixLoc;
GLint shadowMapLoc;


// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// point light (movable debug lamp)
GLint lightPos2Loc = -1;
GLint lightColor2Loc = -1;

// fog uniforms
GLint fogColorLoc = -1;
GLint fogDensityLoc = -1;
GLint fogCenterXZLoc = -1;
GLint fogInnerRadLoc = -1;
GLint fogOuterRadLoc = -1;

gps::Shader depthShader;

GLuint shadowFBO = 0;
GLuint shadowMap = 0;

static const unsigned int SHADOW_WIDTH = 2048;
static const unsigned int SHADOW_HEIGHT = 2048;


// camera
gps::Camera myCamera(
    glm::vec3(2.0f, 2.0f, 8.0f),                    // position
    glm::vec3(3.4273f, 0.800309f, 6.92084f),        // target (pug)
    glm::vec3(0.0f, 1.0f, 0.0f)
);

float cameraSpeed = 2.5f; 
bool pressedKeys[1024];

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// mouse look
bool firstMouse = true;
double lastX = 512.0, lastY = 384.0;
float mouseSensitivity = 0.08f;

// render mode
enum class RenderMode { Solid, Wireframe, Points };
RenderMode renderMode = RenderMode::Solid;

// presentation camera animation
bool presentationMode = false;
float presentationRadius = 10.0f;
glm::vec3 presentationTarget = glm::vec3(0.0f, 1.0f, 0.0f);

// models
gps::Model3D garden;
gps::Model3D pug;

// per-object transforms
glm::vec3 gardenPos(0.0f, 0.0f, 0.0f);
glm::vec3 gardenRot(0.0f, 0.0f, 0.0f); // degrees
float gardenScale = 1.0f;

glm::vec3 pugPos(0.0f, 0.0f, 0.0f);
glm::vec3 pugRot(0.0f, 0.0f, 0.0f); // degrees
float pugScale = 1.0f;
//garden lamps
glm::vec3 lampPosA(6.5167f, 2.00031f, 7.46291f);
glm::vec3 lampPosB(13.4391f, 2.00031f, 9.7713f);

glm::vec3 lampColor(2.0f, 1.55f, 1.05f); // warm lantern light
bool lampsEnabled = true;

// which object is controlled
enum class SelectedObject { Garden, Pug };
SelectedObject selected = SelectedObject::Garden;
bool enterPressed = false;

// shaders
gps::Shader myBasicShader;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

static std::vector<AABB> colliders;

// player body radius (camera collision)
static const float playerRadius = 0.35f;

static const float gardenMinX = -4.6f;
static const float gardenMaxX = 16.1f;

static const float gardenMinZ = -3.6f;
static const float gardenMaxZ = 19.76f;

// floor & ceiling
static const float groundMinY = 0.75f;
static const float ceilingMaxY = 3.8f;

gps::Shader sakuraShader;

GLuint sakuraVAO, sakuraVBO, sakuraSeedVBO;

const int PETAL_COUNT = 600;
std::vector<glm::vec3> sakuraPositions;
std::vector<float> sakuraSeeds;

float sakuraTime = 0.0f;
glm::vec3 sakuraTreePosA = glm::vec3(15.55f, 7.8f, 3.64f);
glm::vec3 sakuraTreePosB = glm::vec3(-0.71f, 7.8f, 13.82f);
glm::vec3 sakuraTreePosC = glm::vec3(15.28f, 7.8f, 16.12f);

gps::Shader skyboxShader;

GLuint skyboxVAO = 0;
GLuint skyboxVBO = 0;
GLuint cubemapTex = 0;

std::vector<glm::vec3> presentationPoints = {
    { 0.110114f, 1.40657f, 7.72186f },
    { -1.15373f, 1.091f, 14.0603f },
    { 4.72644f, 1.46494f, 14.9234f },
    { 13.1005f, 1.79564f, 15.469f },
    { 14.999f, 1.37455f, 8.37873f },
    { 12.4394f, 1.5763f, 5.5074f },
    { 7.65837f, 2.25419f, 0.253555f },
    { 5.50631f, 2.251f, 0.522661f },
    { 3.74401f, 2.251f, 0.957677f },
    { 0.959815f, 2.251f, 1.75131f },
    { 0.375391f, 2.251f, 2.80276f },
    { -0.179611f, 2.251f, 4.35139f },
    { 6.49703f, 1.59893f, 15.0376f },
    { 10.5825f, 0.877511f, 19.3574f }
};

int presentationIndex = 0;
float presentationT = 0.0f;
float presentationSpeed = 0.4f; 

bool shadowsEnabled = true;
bool directionalLightEnabled = true;

bool isFullscreen = false;

int windowedX = 100;
int windowedY = 100;
int windowedW = 1024;
int windowedH = 768;


GLuint loadCubemap(const std::vector<std::string>& faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false); 

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load cubemap face: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return textureID;
}

void toggleFullscreen(GLFWwindow* window)
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if (!isFullscreen)
    {
        // save windowed position + size
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedW, &windowedH);

        // go fullscreen
        glfwSetWindowMonitor(
            window,
            monitor,
            0, 0,
            mode->width,
            mode->height,
            mode->refreshRate
        );

        isFullscreen = true;
    }
    else
    {
        // back to windowed
        glfwSetWindowMonitor(
            window,
            nullptr,
            windowedX,
            windowedY,
            windowedW,
            windowedH,
            0
        );

        isFullscreen = false;
    }
}


void initSkybox()
{
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    std::vector<std::string> faces = {
        "resources/skybox/px.png",
        "resources/skybox/nx.png",
        "resources/skybox/py.png",
        "resources/skybox/ny.png",
        "resources/skybox/pz.png",
        "resources/skybox/nz.png"
    };

    cubemapTex = loadCubemap(faces);

    // set sampler once
    skyboxShader.useShaderProgram();
    GLint loc = glGetUniformLocation(skyboxShader.shaderProgram, "skybox");
    if (loc != -1) glUniform1i(loc, 0);
}


static glm::vec3 clampInsideGarden(const glm::vec3& p)
{
    glm::vec3 out = p;

    out.x = glm::clamp(out.x, gardenMinX + playerRadius, gardenMaxX - playerRadius);
    out.z = glm::clamp(out.z, gardenMinZ + playerRadius, gardenMaxZ - playerRadius);

    // floor
    if (out.y < groundMinY) out.y = groundMinY;

    // ceiling
    if (out.y > ceilingMaxY) out.y = ceilingMaxY;

    return out;
}

// sphere vs AABB
static bool sphereIntersectsAABB(const glm::vec3& c, float r, const AABB& b)
{
    glm::vec3 closest = glm::clamp(c, b.min, b.max);
    glm::vec3 d = c - closest;
    return glm::dot(d, d) < (r * r);
}

static glm::vec3 resolveSphereAABB(glm::vec3 c, float r, const AABB& b)
{
    glm::vec3 closest = glm::clamp(c, b.min, b.max);
    glm::vec3 delta = c - closest;

    if (glm::length(delta) < 1e-6f) {
        float left = c.x - b.min.x;
        float right = b.max.x - c.x;
        float down = c.y - b.min.y;
        float up = b.max.y - c.y;
        float back = c.z - b.min.z;
        float front = b.max.z - c.z;

        float m = left; int axis = 0; float sign = -1.0f;
        if (right < m) { m = right; axis = 0; sign = +1.0f; }
        if (down < m) { m = down;  axis = 1; sign = -1.0f; }
        if (up < m) { m = up;    axis = 1; sign = +1.0f; }
        if (back < m) { m = back;  axis = 2; sign = -1.0f; }
        if (front < m) { m = front; axis = 2; sign = +1.0f; }

        if (axis == 0) c.x += sign * (r + 0.001f);
        if (axis == 1) c.y += sign * (r + 0.001f);
        if (axis == 2) c.z += sign * (r + 0.001f);
        return c;
    }

    float dist = glm::length(delta);
    glm::vec3 n = delta / dist;
    float push = (r - dist) + 0.001f;
    return c + n * push;
}

static void solveCameraCollisions()
{
    glm::vec3 p = myCamera.getPosition();

    // garden bounds (walls + floor + ceiling)
    p = clampInsideGarden(p);

    // house AABB
    for (const auto& b : colliders) {
        if (sphereIntersectsAABB(p, playerRadius, b)) {
            p = resolveSphereAABB(p, playerRadius, b);
        }
    }

    myCamera.setPosition(p);
}

void printCameraPosition()
{
    glm::vec3 p = myCamera.getPosition();
    std::cout
        << "CAMERA POSITION: "
        << "x = " << p.x
        << ", y = " << p.y
        << ", z = " << p.z
        << std::endl;
}

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")\n";
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

static void applyRenderMode()
{
    switch (renderMode) {
    case RenderMode::Solid:     glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  break;
    case RenderMode::Wireframe: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  break;
    case RenderMode::Points:    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
    }
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);

    projection = glm::perspective(glm::radians(45.0f),
        (float)width / (float)height,
        0.1f, 500.0f);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) pressedKeys[key] = true;
        else if (action == GLFW_RELEASE) pressedKeys[key] = false;
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) selected = SelectedObject::Garden;
        if (key == GLFW_KEY_2) selected = SelectedObject::Pug;

        // toggle lamps
        if (key == GLFW_KEY_L) lampsEnabled = !lampsEnabled;

        if (key == GLFW_KEY_F1) renderMode = RenderMode::Solid;
        if (key == GLFW_KEY_F2) renderMode = RenderMode::Wireframe;
        if (key == GLFW_KEY_F3) renderMode = RenderMode::Points;
        if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
        {
            enterPressed = true;
        }

        if (key == GLFW_KEY_P && action == GLFW_PRESS)
        {
            presentationMode = !presentationMode;
            presentationIndex = 0;
            presentationT = 0.0f;

            if (presentationMode)
                myCamera.setPosition(presentationPoints[0]);
        }
        
        if (key == GLFW_KEY_O && action == GLFW_PRESS)
        {
            shadowsEnabled = !shadowsEnabled;
            std::cout << "Shadows: " << (shadowsEnabled ? "ON" : "OFF") << std::endl;
        }

        if (key == GLFW_KEY_K && action == GLFW_PRESS)
        {
            directionalLightEnabled = !directionalLightEnabled;
            std::cout << "Directional light: "
                << (directionalLightEnabled ? "ON" : "OFF")
                << std::endl;
        }

        if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
        {
            toggleFullscreen(window);
        }

    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos; lastY = ypos;
        firstMouse = false;
        return;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    myCamera.rotate((float)yoffset, (float)xoffset);
}

static void updateDeltaTime()
{
    float currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}

static void moveSelectedObject(glm::vec3 deltaPos, glm::vec3 deltaRotDeg, float deltaScale)
{
    glm::vec3* pos = nullptr;
    glm::vec3* rot = nullptr;
    float* sca = nullptr;

    if (selected == SelectedObject::Garden) {
        pos = &gardenPos; rot = &gardenRot; sca = &gardenScale;
    }
    else {
        pos = &pugPos; rot = &pugRot; sca = &pugScale;
    }

    *pos += deltaPos;
    *rot += deltaRotDeg;
    *sca = glm::max(0.05f, (*sca + deltaScale));
}

void processMovement()
{
    float v = cameraSpeed * deltaTime;

    if (!presentationMode) {
        if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, v);
        if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, v);
        if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, v);
        if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, v);
    }

    float r = 60.0f * deltaTime;
    float s = 0.6f * deltaTime;

    if (pressedKeys[GLFW_KEY_Q]) moveSelectedObject(glm::vec3(0), glm::vec3(0, -r, 0), 0);
    if (pressedKeys[GLFW_KEY_E]) moveSelectedObject(glm::vec3(0), glm::vec3(0, r, 0), 0);

    if (pressedKeys[GLFW_KEY_Z]) moveSelectedObject(glm::vec3(0), glm::vec3(0), -s);
    if (pressedKeys[GLFW_KEY_X]) moveSelectedObject(glm::vec3(0), glm::vec3(0), s);

    // camera position probe (press C once)
    static bool cPressed = false;
    if (pressedKeys[GLFW_KEY_C]) {
        if (!cPressed) { printCameraPosition(); cPressed = true; }
    }
    else {
        cPressed = false;
    }

    if (!presentationMode) {
        solveCameraCollisions();
    }
}

void initOpenGLWindow()
{
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks()
{
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);

    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState()
{
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void initModels()
{
    garden.LoadModel("models/japan_garden/garden.obj");
    pug.LoadModel("models/pug_mabel/pug.obj");

    gardenScale = 0.03f;
    pugScale = 0.8f;

    gardenPos = glm::vec3(0.0f);
    gardenRot = glm::vec3(0.0f);

    pugPos = glm::vec3(3.4273f, 0.800309f, 6.92084f);
    pugRot = glm::vec3(0.0f, 230.0f, 0.0f);

    colliders.clear();

    colliders.push_back(AABB{
        glm::vec3(15.35f, 0.75f, 7.0f),  
        glm::vec3(16.10f, 3.8f, 16.2f)    
        });

    colliders.push_back(AABB{
        glm::vec3(-3.3f, 0.6f, -2.8f),
        glm::vec3(7.8f,  1.9f,  7.2f)
        });
}

void initShadowMap()
{
    glGenFramebuffers(1, &shadowFBO);

    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.f, 1.f, 1.f, 1.f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void initShaders()
{
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    sakuraShader.loadShader("shaders/sakura.vert", "shaders/sakura.frag");
    skyboxShader.loadShader("shaders/skybox.vert", "shaders/skybox.frag");
    // shadow depth shader
    depthShader.loadShader("shaders/depth.vert", "shaders/depth.frag");
}

void initUniforms()
{
    myBasicShader.useShaderProgram();
    

    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // view
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // projection
    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 500.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // directional light
    lightDir = glm::normalize(glm::vec3(-0.5f, 0.6f, 0.6f));

    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    if (lightDirLoc != -1) glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColor = glm::vec3(1.0f, 0.75f, 0.55f);
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    if (lightColorLoc != -1) glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // point light (lamps via these uniforms)
    lightPos2Loc = glGetUniformLocation(myBasicShader.shaderProgram, "lightPos2");
    lightColor2Loc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor2");

    // fog
    fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    fogCenterXZLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogCenterXZ");
    fogInnerRadLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogInnerRadius");
    fogOuterRadLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogOuterRadius");

    if (fogColorLoc != -1) glUniform3f(fogColorLoc, 0.78f, 0.80f, 0.83f);
    if (fogDensityLoc != -1) glUniform1f(fogDensityLoc, 0.045f);
    if (fogCenterXZLoc != -1) glUniform2f(fogCenterXZLoc, 0.0f, 0.0f);
    if (fogInnerRadLoc != -1) glUniform1f(fogInnerRadLoc, 12.0f);
    if (fogOuterRadLoc != -1) glUniform1f(fogOuterRadLoc, 16.0f);

    cameraPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "cameraPos");

    // shadow uniforms (set sampler once)
    GLint shadowMapLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");
    if (shadowMapLoc != -1) glUniform1i(shadowMapLoc, 3);

    lightSpaceMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix");
    glm::vec3 lightPos = -lightDir * 30.0f; // pull sun back

    glm::mat4 lightView = glm::lookAt(
        lightPos,
        glm::vec3(5.0f, 0.0f, 8.0f), // look at garden center
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 lightProjection = glm::ortho(
        -30.0f, 30.0f,
        -30.0f, 30.0f,
        1.0f, 80.0f
    );

    lightSpaceMatrix = lightProjection * lightView;

    lightSpaceMatrixLoc =
        glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix");

    glUniformMatrix4fv(
        lightSpaceMatrixLoc,
        1,
        GL_FALSE,
        glm::value_ptr(lightSpaceMatrix)
    );

}

static glm::mat4 composeModelMatrix(glm::vec3 pos, glm::vec3 rotDeg, float scale)
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, pos);
    m = glm::rotate(m, glm::radians(rotDeg.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rotDeg.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rotDeg.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, glm::vec3(scale));
    return m;
}
static void renderModelWithShader(gps::Model3D& m, gps::Shader& shader, const glm::mat4& modelMatrix, bool uploadNormalMatrix)
{
    shader.useShaderProgram();

    GLint locModel = glGetUniformLocation(shader.shaderProgram, "model");
    if (locModel != -1) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    if (uploadNormalMatrix && shader.shaderProgram == myBasicShader.shaderProgram) {
        glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * modelMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    m.Draw(shader);
}

static void updatePresentationCamera()
{
    if (presentationPoints.size() < 2)
        return;

    glm::vec3 p0 = presentationPoints[presentationIndex];
    glm::vec3 p1 = presentationPoints[(presentationIndex + 1) % presentationPoints.size()];

    presentationT += deltaTime * presentationSpeed;

    if (presentationT >= 1.0f) {
        presentationT = 0.0f;
        presentationIndex = (presentationIndex + 1) % presentationPoints.size();
        p0 = presentationPoints[presentationIndex];
        p1 = presentationPoints[(presentationIndex + 1) % presentationPoints.size()];
    }

    glm::vec3 camPos = glm::mix(p0, p1, presentationT);
    glm::vec3 lookTarget = glm::mix(p0, p1, glm::min(presentationT + 0.05f, 1.0f));

    myCamera.setPosition(camPos);
    view = glm::lookAt(camPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}



void renderSakuraPetals()
{
    if (sakuraVAO == 0 || sakuraShader.shaderProgram == 0)
        return;

    sakuraShader.useShaderProgram();

    sakuraTime += deltaTime * 0.6f;

    GLint loc;

    loc = glGetUniformLocation(sakuraShader.shaderProgram, "uTime");
    if (loc != -1) glUniform1f(loc, sakuraTime);

    glm::mat4 view = myCamera.getViewMatrix();

    loc = glGetUniformLocation(sakuraShader.shaderProgram, "view");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));

    loc = glGetUniformLocation(sakuraShader.shaderProgram, "projection");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projection));
    GLint treeALoc = glGetUniformLocation(sakuraShader.shaderProgram, "treePosA");
    if (treeALoc != -1)
        glUniform3fv(treeALoc, 1, glm::value_ptr(sakuraTreePosA));

    GLint treeBLoc = glGetUniformLocation(sakuraShader.shaderProgram, "treePosB");
    if (treeBLoc != -1)
        glUniform3fv(treeBLoc, 1, glm::value_ptr(sakuraTreePosB));

    GLint treeCLoc = glGetUniformLocation(sakuraShader.shaderProgram, "treePosC");
    if (treeCLoc != -1)
        glUniform3fv(treeCLoc, 1, glm::value_ptr(sakuraTreePosC));


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(sakuraVAO);
    glDrawArrays(GL_POINTS, 0, PETAL_COUNT);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void renderSkybox()
{
    glDepthFunc(GL_LEQUAL); 
    glDepthMask(GL_FALSE);  

    skyboxShader.useShaderProgram();

    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(myCamera.getViewMatrix()));
    GLint viewLocSB = glGetUniformLocation(skyboxShader.shaderProgram, "view");
    if (viewLocSB != -1) glUniformMatrix4fv(viewLocSB, 1, GL_FALSE, glm::value_ptr(viewNoTrans));

    GLint projLocSB = glGetUniformLocation(skyboxShader.shaderProgram, "projection");
    if (projLocSB != -1) glUniformMatrix4fv(projLocSB, 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void renderScene()
{

    float time = (float)glfwGetTime();
    glm::vec3 pugAnimPos = pugPos;
    glm::vec3 pugAnimRot = pugRot;
    pugAnimPos.y += 0.08f * std::sin(time * 2.0f);
    pugAnimRot.y += 6.0f * std::sin(time * 1.2f);

    glm::mat4 gardenModel = composeModelMatrix(gardenPos, gardenRot, gardenScale);
    glm::mat4 pugModel = composeModelMatrix(pugAnimPos, pugAnimRot, pugScale);

    if (presentationMode) updatePresentationCamera();
    else view = myCamera.getViewMatrix();

    float near_plane = 1.0f;
    float far_plane = 60.0f;

    glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);

    glm::vec3 sceneCenter = glm::vec3(
        0.5f * (gardenMinX + gardenMaxX),
        1.5f,
        0.5f * (gardenMinZ + gardenMaxZ)
    );

    glm::vec3 lightPos = sceneCenter - lightDir * 25.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.useShaderProgram();
    GLint lsmLocDepth = glGetUniformLocation(depthShader.shaderProgram, "lightSpaceMatrix");
    if (lsmLocDepth != -1) glUniformMatrix4fv(lsmLocDepth, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    renderModelWithShader(garden, depthShader, gardenModel, false);
    renderModelWithShader(pug, depthShader, pugModel, false);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // restore viewport to screen
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    applyRenderMode();

    myBasicShader.useShaderProgram();
// directional light ON / OFF
    GLint dirLightLoc =
        glGetUniformLocation(myBasicShader.shaderProgram, "useDirectionalLight");

    if (dirLightLoc != -1)
        glUniform1i(dirLightLoc, directionalLightEnabled ? 1 : 0);

    // shadows ONLY if sun is ON
    GLint useShadowsLoc =
        glGetUniformLocation(myBasicShader.shaderProgram, "useShadows");

    bool finalShadows = shadowsEnabled && directionalLightEnabled;

    if (useShadowsLoc != -1)
        glUniform1i(useShadowsLoc, finalShadows ? 1 : 0);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, finalShadows ? shadowMap : 0);


    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // bind shadow map to texture unit 3
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowsEnabled ? shadowMap : 0);

    if (lightSpaceMatrixLoc != -1) {
        glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    }

    //  GARDEN LAMPS 
    if (lampsEnabled && lightPos2Loc != -1 && lightColor2Loc != -1)
    {
        glm::vec3 blendedPos = 0.5f * (lampPosA + lampPosB);
        glUniform3fv(lightPos2Loc, 1, glm::value_ptr(blendedPos));

        // stronger to simulate 2 lamps
        glUniform3f(lightColor2Loc, 3.2f, 2.4f, 1.6f);
    }
    else if (lightColor2Loc != -1)
    {
        glUniform3f(lightColor2Loc, 0.0f, 0.0f, 0.0f);
    }

    // draw scene normally
    renderSkybox();
    renderModelWithShader(garden, myBasicShader, gardenModel, true);
    renderModelWithShader(pug, myBasicShader, pugModel, true);
    renderSakuraPetals();

}

void initSakuraPetals()
{
    std::vector<glm::vec3> positions;
    std::vector<float> seeds;

    for (int i = 0; i < PETAL_COUNT; i++)
    {
        float canopyRadius = 4.3f;

        positions.emplace_back(
            ((rand() % 1000) / 1000.0f - 0.5f) * 6.0f,   
            (rand() % 1000) / 1000.0f * 1.2f,            
            ((rand() % 1000) / 1000.0f - 0.5f) * 6.0f
        );




        seeds.push_back((rand() % 1000) / 1000.0f);
    }

    glGenVertexArrays(1, &sakuraVAO);
    glBindVertexArray(sakuraVAO);

    glGenBuffers(1, &sakuraVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sakuraVBO);
    glBufferData(GL_ARRAY_BUFFER,
        positions.size() * sizeof(glm::vec3),
        positions.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glGenBuffers(1, &sakuraSeedVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sakuraSeedVBO);
    glBufferData(GL_ARRAY_BUFFER,
        seeds.size() * sizeof(float),
        seeds.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
}


void cleanup()
{
    myWindow.Delete();

    if (shadowMap) glDeleteTextures(1, &shadowMap);
    if (shadowFBO) glDeleteFramebuffers(1, &shadowFBO);
}

int main(int argc, const char* argv[])
{
    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    glEnable(GL_PROGRAM_POINT_SIZE);
    initSkybox();
    initSakuraPetals();
    initShadowMap();

    initUniforms();
    setWindowCallbacks();

    glCheckError();
    

    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        updateDeltaTime();
        processMovement();
        renderScene();
        if (enterPressed)
        {
            glm::vec3 pos = myCamera.getPosition();

            std::cout << "Sakura spawn position:\n";
            std::cout << "glm::vec3("
                << pos.x << "f, "
                << pos.y << "f, "
                << pos.z << "f);\n\n";

            enterPressed = false;
        }

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());
        glCheckError();
    }

    cleanup();
    return EXIT_SUCCESS;
}
