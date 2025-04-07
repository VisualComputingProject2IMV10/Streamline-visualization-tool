/**
* @file Source.cpp
 * @brief Main application file for the Streamline Visualization tool
 *
 * This file contains the main application logic for the 3D streamline visualization
 * tool, including initialization, rendering loop, event handling, and GUI.
 */
#if defined(_POSIX_VERSION)
#include <unistd.h>
#endif

#include "extra/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <string>
#include <cmath>

#include "include/Constants.h"
#include "include/Shader.h"
#include "include/DataReader.h"
#include "include/VectorField.h"
#include "include/StreamlineTracer.h"
#include "include/StreamlineRenderer.h"

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float xFov = 0.0f;
float yFov = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

//projection matrices
glm::mat4 projection;
glm::mat4 view;

// Data variables
float* globalScalarData = nullptr;
int scalarDimX = 0, scalarDimY = 0, scalarDimZ = 0;

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//initial settings
const char* currentDataset = BRAIN_DATASET;
const char* currentScalarFile = BRAIN_SCALAR_PATH;
const char* currentVectorFile = BRAIN_VECTOR_PATH;

// Streamline parameters
float stepSize = 0.5f;
float maxLength = 50.0f;
int maxSteps = 1;
float maxAngleDegrees = 45;
float maxAngle = maxAngleDegrees * (std::_Pi_val / 180); //about 45 degrees

float lineWidth = 2.0f;

// Global objects
VectorField* vectorField = nullptr;
StreamlineTracer* streamlineTracer = nullptr;
StreamlineRenderer* streamlineRenderer = nullptr;
Shader* sliceShader = nullptr;
Shader* streamlineShader = nullptr;
Shader* glyphShader = nullptr;
int dimX = 0, dimY = 0, dimZ = 0;
unsigned int texture = 0;
unsigned int sliceVAO = 0, sliceVBO = 0, sliceEBO = 0;

//threedimensional
int currentSliceX = 0;
int currentSliceY = 0;
int currentSliceZ = 0;
int selectedAxis = AXIS_Y; //0 = x, 1 = y, 2 = z


// Interactive seeding
glm::vec3 mouseSeedLoc;
bool useMouseSeeding = false;
bool paramsChanged = false; //for the gui
bool viewAxisChanged = false;

//std::vector<std::vector<Point3D>> manualStreamlines;
//float manualSeedLineWidth = 3.0f;

//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------

// GLFW callback functions
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int keyCode, int scancode, int actionType, int mods);

// Data handling functions
void seedStreamlinesAtPoint(float x, float y, float z);
float sampleScalarData(float x, float y, float z);

void updatePVMatrices();

/**
 * Load the data files into memory and generate the corresponding 3d texture.
 */
void loadCurrentDataFiles()
{
    std::cout << "Starting loading data file for  " << currentDataset << std::endl;

    // Clean up old resources
    if (vectorField) {
        delete vectorField;
        vectorField = nullptr;
    }
    if (globalScalarData) {
        delete[] globalScalarData;
        globalScalarData = nullptr;
    }
    if (streamlineRenderer) {
        delete streamlineRenderer;
        streamlineRenderer = nullptr;
    }
    if (streamlineTracer) {
        delete streamlineTracer;
        streamlineTracer = nullptr;
    }
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }

    //load the scalar data
    if (readData(currentScalarFile, globalScalarData, dimX, dimY, dimZ) != EXIT_SUCCESS) {
        std::cerr << "Failed to read scalar data from " << currentScalarFile << std::endl;
        return;
    }

    // Store dimensions for global access
    scalarDimX = dimX;
    scalarDimY = dimY;
    scalarDimZ = dimZ;

    std::cout << "Loaded scalar data: " << dimX << "x" << dimY << "x" << dimZ << std::endl;

    //loading vector data
    try {
        vectorField = new VectorField(currentVectorFile); //todo why is this a pointer in the first place?
    } catch (const std::exception& e) {
        std::cerr << "Error loading vector field: " << e.what() << std::endl;
        return;
    }

    std::cout << "Loaded vector data" << std::endl;

    //calculate an image texture of the scalar data with opacity 0 where the vector field has a zero vector
    bool* zeroMask = vectorField->getZeroMask(dimX, dimY, dimZ);
    float* imagedata = new float[dimX * dimY * dimZ * 2]; //two components per voxel
    for (size_t x = 0; x < dimX; x++)
    {
        for (size_t y = 0; y < dimY; y++)
        {
            for (size_t z = 0; z < dimZ; z++)
            {
                int imgIndex = 2 * x + 2 * dimX * y + 2 * dimX * dimY * z;
                imagedata[imgIndex] = globalScalarData[x + y * dimX + z * dimX * dimY];
                imagedata[imgIndex + 1] = zeroMask[x + y * dimX + z * dimX * dimY] ? 1.0f : 0.0f;
            }
        }
    }

    // Setup or update the 3D texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor);
    if (USE_SMOOTH_BACKGROUND)
    {
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RG, dimX, dimY, dimZ, 0, GL_RG, GL_FLOAT, imagedata);

    delete[] imagedata;
    imagedata = nullptr;

    std::cout << "Generated 3d texture" << std::endl;

    currentSliceX = dimX / 2;
    currentSliceY = dimY / 2;
    currentSliceZ = dimZ / 2;

    // Initialize camera position based on data dimensions
    updatePVMatrices();
}

/**
 * Update the vertex data to show the currently selected slice.
 */
void initImgPlane()
{
    float vertexData[] =
    {
        //position                   //texture coord: the slice is selected in the vertex shader
        //current axis Z
        0.0f, 0.0f, -2.0f * dimZ,    0.0f, 0.0f, 0.5f, //bottom left
        dimX, 0.0f, -2.0f * dimZ,    1.0f, 0.0f, 0.5f, //bottom right
        0.0f, dimY, -2.0f * dimZ,    0.0f, 1.0f, 0.5f, //top left
        dimX, dimY, -2.0f * dimZ,    1.0f, 1.0f, 0.5f,  //top right

        //current axis Y
        0.0f, 2.0f * dimY, 0.0f,    0.0f, 0.5f, 0.0f, //bottom left
        dimX, 2.0f * dimY, 0.0f,    1.0f, 0.5f, 0.0f, //bottom right
        0.0f, 2.0f * dimY, dimZ,    0.0f, 0.5f, 1.0f, //top left
        dimX, 2.0f * dimY, dimZ,    1.0f, 0.5f, 1.0f,  //top right


        //current axis X
        -2.0f * dimX, 0.0f, 0.0f,    0.5f, 0.0f, 0.0f, //bottom left
        -2.0f * dimX, 0.0f, dimZ,    0.5f, 0.0f, 1.0f, //bottom right
        -2.0f * dimX, dimY, 0.0f,    0.5f, 1.0f, 0.0f, //top left
        -2.0f * dimX, dimY, dimZ,    0.5f, 1.0f, 1.0f  //top right
    };

    unsigned int vertexIndices[] =
    {
        0, 1, 2,
        1, 2, 3,
        0xFFFF,
        4, 5, 6,
        5, 6, 7,
        0xFFFF,
        8, 9, 10,
        9, 10, 11
    };
    
    //init buffers
    if (!sliceVAO) {
        glGenVertexArrays(1, &sliceVAO);
        glGenBuffers(1, &sliceVBO);
        glGenBuffers(1, &sliceEBO);
    }
    glBindVertexArray(sliceVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sliceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sliceEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndices), vertexIndices, GL_STATIC_DRAW);

    //set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)0); //position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT))); //tex coord
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); //unbind vertex array
}

/**
 * Generates streamlines
 */
std::vector<std::vector<Point3D>> generateStreamlines()
{
    if (!vectorField)
    {
        throw std::runtime_error("No vectorfield initialized when trying to generate streamlines");
    }

    std::vector<std::vector<Point3D>> streamlines;
    try
    {
        std::cout << "Started seeding" << std::endl;

        std::vector<Point3D> seeds;

        //todo give different options for seeding
        if (useMouseSeeding)
        {
            float seedRadius = std::max(std::max(dimX / 30.0f, dimY / 30.0f), dimZ / 30.0f);
            seeds = streamlineTracer->generateMouseSeeds(currentSliceX, currentSliceY, currentSliceZ, selectedAxis, mouseSeedLoc, seedRadius, 50.0f);
        }
        else
        {
            seeds = streamlineTracer->generateSliceGridSeeds(currentSliceX, currentSliceY, currentSliceZ, selectedAxis);
        }
        std::cout << "Seeded " << seeds.size() << " seeds from the current slice" << std::endl;
    
        if (!seeds.empty()) 
        {
            streamlines = streamlineTracer->traceAllStreamlines(seeds);
            //streamlines = tracer.traceVectors(seeds);
            std::cout << "Generated " << streamlines.size() << " streamlines" << std::endl;
        }
        else 
        {
            std::cout << "No seeds generated, skipping streamline tracing" << std::endl;
        }
        return streamlines;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error generating streamlines: " << e.what() << std::endl;
        return streamlines;
    }
}

void regenerateStreamLines()
{
    paramsChanged = false;
    if (streamlineTracer)
    {
        streamlineTracer->maxAngle = maxAngle;
        streamlineTracer->maxLength = maxLength;
        streamlineTracer->maxSteps = maxSteps;
        streamlineTracer->stepSize = stepSize;
    }

    if (vectorField && streamlineRenderer) {
        std::vector<std::vector<Point3D>> streamlines = generateStreamlines();
        streamlineRenderer->prepareStreamlines(streamlines);
    }
}

void updatePVMatrices()
{
    // Initialize camera position based on data dimensions
    if (selectedAxis == AXIS_X)
    {
        cameraPos = glm::vec3(dimX, -dimY / 2.0f, -dimZ / 2.0f);
        cameraFront = glm::vec3(-1.0f, 0.0f, 0.0f);
        cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
        projection = glm::ortho(0.0f, (float)dimY-1.0f, 0.0f, (float)dimZ-1.0f, NEAR_CAM_PLANE, FAR_CAM_PLANE);
        ////not sure if this is quite right
        //cameraPos = glm::vec3(dimX, -dimY / 2.0f, -dimZ / 2.0f);
        //cameraFront = glm::vec3(-1.0f, 0.0f, 0.0f);
        //cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
        //projection = glm::ortho(0.0f, (float)dimZ, 0.0f, (float)dimY, NEAR_CAM_PLANE, FAR_CAM_PLANE);

    }
    else if (selectedAxis == AXIS_Y)
    {
        cameraPos = glm::vec3(-dimX / 2.0f, -dimY, -dimZ / 2.0f);
        cameraFront = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
        projection = glm::ortho(0.0f, (float)dimZ-1.0f, 0.0f, (float)dimX-1.0f, NEAR_CAM_PLANE, FAR_CAM_PLANE);
    }
    else if (selectedAxis == AXIS_Z)
    {
        cameraPos = glm::vec3(-dimX / 2.0f, -dimY / 2.0f, dimZ);
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        projection = glm::ortho(0.0f, (float)dimX-1.0f, 0.0f, (float)dimY-1.0f, NEAR_CAM_PLANE, FAR_CAM_PLANE);
    }
    else throw std::runtime_error("invalid axis selected");

    //initialize view matrix
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

/**
 * @brief Process continuous keyboard input each frame
 *
 * This function handles continuous keyboard input for camera movement
 * and other real-time controls.
 *
 * @param window GLFW window handle
 */
void processInput(GLFWwindow* window) {
    // Exit application when Escape is pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

/**
 * @brief Handle mouse movement for camera control
 *
 * This callback processes mouse movement for camera rotation in both
 * standard and orbit camera modes.
 *
 * @param window GLFW window handle
 * @param xpos Current mouse X position
 * @param ypos Current mouse Y position
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Standard camera control (non-orbit)
    bool moveCamera = /*cameraMode ||*/ (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    if (!moveCamera) {
        firstMouse = true;
        return;
    }

    // Initialize reference position on first movement
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Calculate mouse movement since last frame
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    //correct the offset
    int scrWidth, scrHeight;
    glfwGetWindowSize(window, &scrWidth, &scrHeight);

    //move the camera since we use an ortho perspective
    if (selectedAxis == AXIS_X)
    {
        xoffset *= (((float)dimY - 2 * xFov) / scrWidth);
        yoffset *= (((float)dimZ - 2 * yFov) / scrHeight);
        cameraPos.y -= xoffset;
        cameraPos.z -= yoffset;
    }
    else if (selectedAxis == AXIS_Y)
    {
        xoffset *= (((float)dimX - 2 * xFov) / scrWidth);
        yoffset *= (((float)dimZ - 2 * yFov) / scrHeight);
        cameraPos.x -= xoffset;
        cameraPos.z -= yoffset;
    }
    else if (selectedAxis == AXIS_Z)
    {
        xoffset *= (((float)dimX - 2 * xFov) / scrWidth);
        yoffset *= (((float)dimY - 2 * yFov) / scrHeight);
        cameraPos.x -= xoffset;
        cameraPos.y -= yoffset;
    }

    //update view matrix
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

/**
 * @brief Handle window resize events
 *
 * This callback is triggered when the window is resized, ensuring
 * the OpenGL viewport matches the new window dimensions.
 *
 * @param window GLFW window handle
 * @param width New window width
 * @param height New window height
 */
void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Unused parameter
    (void)window;

    // Update OpenGL viewport to match the new window size
    glViewport(0, 0, width, height);
}

/**
 * @brief Generate and render a streamline from a specific seed point
 *
 * This function creates a streamline from a user-specified seed point,
 * adds it to the manual collection, and renders it immediately.
 *
 * @param x X coordinate of the seed point
 * @param y Y coordinate of the seed point
 * @param z Z coordinate of the seed point
 */
void seedStreamlinesAtPoint(float x, float y, float z) {
    //// Check if vector field exists
    //if (!vectorField) return;

    //// Clamp coordinates to valid range
    //x = std::max(0.0f, std::min(x, (float)dimX - 1.0f));
    //y = std::max(0.0f, std::min(y, (float)dimY - 1.0f));
    //z = std::max(0.0f, std::min(z, (float)dimZ - 1.0f));

    //std::cout << "Seeding streamline at (" << x << ", " << y << ", " << z << ")" << std::endl;

    //// Create a seed point
    //Point3D seed(x, y, z);

    //// Create a streamline tracer with current parameters
    //StreamlineTracer tracer(vectorField, stepSize, maxSteps, maxLength);

    //// Trace streamline in both directions from the seed
    //std::vector<Point3D> newStreamline = tracer.traceStreamline(seed);

    //// Only add if streamline has sufficient points
    //if (newStreamline.size() > 2) {
    //    // Store in manual streamlines collection for persistence
    //    manualStreamlines.push_back(newStreamline);

    //    // Prepare container for just this new streamline
    //    std::vector<std::vector<Point3D>> singleStreamline;
    //    singleStreamline.push_back(newStreamline);

    //    // Add to existing renderer for immediate visualization
    //    if (streamlineRenderer) {
    //        // Save current line width
    //        float oldWidth = lineWidth;

    //        // Use special width for manually seeded streamlines
    //        glLineWidth(manualSeedLineWidth);

    //        // Add to renderer
    //        streamlineRenderer->addStreamlines(singleStreamline);

    //        // Restore original line width
    //        glLineWidth(oldWidth);
    //    }

    //    std::cout << "Added a new streamline with " << newStreamline.size() << " points" << std::endl;
    //} else {
    //    std::cout << "Generated streamline was too short (" << newStreamline.size() << " points)" << std::endl;
    //}
}

/**
 * @brief Sample scalar data at a floating-point position with trilinear interpolation
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Z coordinate
 * @return Interpolated scalar value
 */
float sampleScalarData(float x, float y, float z) {
    // Check if scalar data exists
    if (!globalScalarData || scalarDimX <= 0 || scalarDimY <= 0 || scalarDimZ <= 0) {
        return 0.0f;
    }

    if (false)
    {
    //todo simplified way:
    int x0 = std::roundf(x);
    int y0 = std::roundf(y);
    int z0 = std::roundf(z);

    float intensity = globalScalarData[z0 + scalarDimZ * (y0 + scalarDimY * x0)]; //wrong index
    return intensity;
    }
    else
    {
    // Ensure coordinates are within bounds
    x = std::max(0.0f, std::min(x, static_cast<float>(scalarDimX - 1.01f)));
    y = std::max(0.0f, std::min(y, static_cast<float>(scalarDimY - 1.01f)));
    z = std::max(0.0f, std::min(z, static_cast<float>(scalarDimZ - 1.01f)));

    // Get integer coordinates for the 8 surrounding voxels
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int z0 = static_cast<int>(z);
    int x1 = std::min(x0 + 1, scalarDimX - 1);
    int y1 = std::min(y0 + 1, scalarDimY - 1);
    int z1 = std::min(z0 + 1, scalarDimZ - 1);

    // Calculate interpolation weights
    float wx = x - x0;
    float wy = y - y0;
    float wz = z - z0;

    // Get values at the 8 surrounding voxels
    float v000 = globalScalarData[z0 * scalarDimY * scalarDimX + y0 * scalarDimX + x0];
    float v001 = globalScalarData[z1 * scalarDimY * scalarDimX + y0 * scalarDimX + x0];
    float v010 = globalScalarData[z0 * scalarDimY * scalarDimX + y1 * scalarDimX + x0];
    float v011 = globalScalarData[z1 * scalarDimY * scalarDimX + y1 * scalarDimX + x0];
    float v100 = globalScalarData[z0 * scalarDimY * scalarDimX + y0 * scalarDimX + x1];
    float v101 = globalScalarData[z1 * scalarDimY * scalarDimX + y0 * scalarDimX + x1];
    float v110 = globalScalarData[z0 * scalarDimY * scalarDimX + y1 * scalarDimX + x1];
    float v111 = globalScalarData[z1 * scalarDimY * scalarDimX + y1 * scalarDimX + x1];

    // Perform trilinear interpolation
    float v00 = v000 * (1 - wz) + v001 * wz;
    float v01 = v010 * (1 - wz) + v011 * wz;
    float v10 = v100 * (1 - wz) + v101 * wz;
    float v11 = v110 * (1 - wz) + v111 * wz;

    float v0 = v00 * (1 - wy) + v01 * wy;
    float v1 = v10 * (1 - wy) + v11 * wy;

    return v0 * (1 - wx) + v1 * wx;
    }
    
}

/**
 * @brief Handle mouse button events
 *
 * This callback processes mouse clicks for orbit mode,
 * interactive streamline seeding, and other interaction.
 *
 * @param window GLFW window handle
 * @param button Mouse button that was pressed/released
 * @param action GLFW_PRESS or GLFW_RELEASE
 * @param mods Modifier keys (shift, ctrl, alt)
 */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) 
{

    (void)mods;  // Unused parameter
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    // Handle left mouse button
    if (button == GLFW_MOUSE_BUTTON_LEFT) 
    {
        if (action == GLFW_PRESS)
        {
            if (useMouseSeeding)
            {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                int scrWidth, scrHeight;
                glfwGetWindowSize(window, &scrWidth, &scrHeight);

                float ndcX = (2.0f * xpos) / scrWidth - 1.0f;
                float ndcY = 1.0f - (2.0f * ypos) / scrHeight;

                if (selectedAxis == AXIS_X)
                {

                }
                else if (selectedAxis == AXIS_Y)
                {
                    glm::vec4 ray_clip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f); //homogenous clip space coords of the ray
                    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
                    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 1.0f); //only the x and y are important and we want the ray to point into the screen
                    glm::vec4 ray_world = glm::inverse(view) * ray_eye;
                    mouseSeedLoc = glm::vec3(ray_world.x + dimX / 2.0f, ray_world.y + dimY / 2.0f, ray_world.z);
                }
                else if (selectedAxis == AXIS_Z)
                {
                    glm::vec4 ray_clip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f); //homogenous clip space coords of the ray
                    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
                    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 1.0f); //only the x and y are important and we want the ray to point into the screen
                    glm::vec4 ray_world = glm::inverse(view) * ray_eye;
                    mouseSeedLoc = glm::vec3(ray_world.x + dimX / 2.0f, ray_world.y + dimY / 2.0f, ray_world.z);
                }
                
            
                regenerateStreamLines();
            }

        }
    }
}

/**
 * @brief Handle keyboard input events
 *
 * This callback processes key presses and releases for camera control
 * and application shortcuts.
 *
 * @param window GLFW window handle
 * @param keyCode Key code of the pressed/released key
 * @param scancode Platform-specific scancode
 * @param actionType Action type (press, release, repeat)
 * @param mods Modifier keys (shift, ctrl, alt)
 */
void key_callback(GLFWwindow* window, int keyCode, int scancode, int actionType, int mods) {
    
}


/**
 * @brief Handle mouse scroll wheel events
 *
 * This callback adjusts the camera field of view (zoom) based on
 * mouse scroll wheel movement.
 *
 * @param window GLFW window handle
 * @param xoffset Horizontal scroll offset (usually 0)
 * @param yoffset Vertical scroll offset
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {

    (void)window;   // Unused parameter
    (void)xoffset;  // Unused parameter

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    // Adjust field of view (zoom) based on scroll wheel
    xFov -= (float)yoffset * 2.0f;
    yFov -= (float)yoffset * 2.0f;

    //update projection matrix
    if (xFov > ((float) dimX / 2.0f) - 1.0f) xFov = ((float)dimX / 2.0f) - 1.0f;
    if (yFov > ((float) dimY / 2.0f) - 1.0f) yFov = ((float)dimY / 2.0f) - 1.0f;

    projection = glm::ortho(0.0f + xFov, (float)dimX - xFov, 0.0f + yFov, (float)dimY - yFov, NEAR_CAM_PLANE, FAR_CAM_PLANE);

}

/**
 * Take all the actions for switching between datasets
 */
void switchDataSet()
{
    std::cout << "Updated Scalar File Path: " << currentScalarFile << std::endl;
    std::cout << "Updated Vector File Path: " << currentVectorFile << std::endl;

    // Initial data loading
    //loadData();
    loadCurrentDataFiles();
    initImgPlane();

    //Initialize a streamline tracer and renderer
    streamlineTracer = new StreamlineTracer(vectorField, stepSize, maxSteps, maxLength, maxAngle); //TODO should this be a pointer?
    streamlineRenderer = new StreamlineRenderer(streamlineShader);

    //generate the initial streamlines
    std::vector<std::vector<Point3D>> streamlines = generateStreamlines();
    streamlineRenderer->prepareStreamlines(streamlines);
}

/**
 * @brief Main entry point for the application
 *
 * Sets up GLFW, OpenGL context, loads data, and runs the main render loop
 */
int main(int argc, char* argv[]) {
    GLFWwindow* window;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL context version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Streamline Visualization", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set context and callbacks
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to load GLAD" << std::endl;
        return -1;
    }

    // Configure OpenGL state
    //glEnable(GL_DEPTH_TEST);
    glLineWidth(lineWidth);

    //enable line antialiasing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //somehow this also moves the background around or something???

    // Enable primitive restart to allow drawing the different streamlines from the same elements buffer
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFF);

    // Create shaders
    sliceShader = new Shader("shaders/vertexShader1.vs", "shaders/FragShader1.fs");
    streamlineShader = new Shader("shaders/streamlineVertex.vs", "shaders/streamlineFragment.fs");
    glyphShader = new Shader("shaders/glyphVertex.vs", "shaders/glyphFragment.fs");
    std::cout << "Shaders loaded with ID's: " << sliceShader->ID  << ", " << streamlineShader->ID << ", " << glyphShader->ID << std::endl;

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    switchDataSet();

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);

        // Clear the screen
        //glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClearColor(0.1f, 0.054f, 0.3f, 1.0f); //some kind of dark blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //image plane model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-dimX/2.0f, -dimY/2.0f, -dimZ/2.0f));
        model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));

        // Create model matrix for streamlines
        glm::mat4 streamlineModel = glm::mat4(1.0f);
        streamlineModel = glm::translate(streamlineModel, glm::vec3(-dimX / 2.0f, -dimY / 2.0f, -dimZ / 2.0f));
        streamlineModel = glm::translate(streamlineModel, glm::vec3(0.0f, 0.0f, 0.0f));

        // Set up depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);  // Allow drawing on top of equal depth values

        // Render slice and streamlines
        if (streamlineRenderer != nullptr && vectorField != nullptr) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);  // Standard depth test

            // Render background slice
            sliceShader->use();
            glBindTexture(GL_TEXTURE_3D, texture);
            sliceShader->setInt("selectedAxis", selectedAxis);
            sliceShader->setMat4("projection", projection); //TODO if this is an inefficient operation we should only set the projection matrix at the start
            sliceShader->setMat4("view", view);
            sliceShader->setMat4("model", model);


            if (selectedAxis == AXIS_Z)
            {
                float currentSliceZF = (float)currentSliceZ / ((float)dimZ - 1.0f);
                sliceShader->setFloat("currentSlice", currentSliceZF);
            }
            else if (selectedAxis == AXIS_Y)
            {
                float currentSliceYF = (float)currentSliceY / ((float)dimY - 1.0f);
                sliceShader->setFloat("currentSlice", currentSliceYF);
            }
            else if (selectedAxis == AXIS_X)
            {
                float currentSliceXF = (float)currentSliceX / ((float)dimX - 1.0f);
                sliceShader->setFloat("currentSlice", currentSliceXF);
            }

            glBindVertexArray(sliceVAO);
            glDrawElements(GL_TRIANGLES, 20, GL_UNSIGNED_INT, 0);

            // Set up depth for streamlines to appear above slice
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);

            // Render streamlines
            streamlineShader->use();
            streamlineShader->setMat4("projection", projection);
            streamlineShader->setMat4("view", view);
            streamlineShader->setMat4("model", streamlineModel);

            streamlineRenderer->render();
        }

        // ImGui rendering
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create control panel window
        ImGui::Begin("Streamline Controls");

        // Camera control section
        ImGui::Separator();
        ImGui::Text("Camera Controls");
        ImGui::TextWrapped("Move the camera using the right mouse button.");
        ImGui::TextWrapped("Zoom using the mouse scrollwheel.");

        // Data selection section
        ImGui::Separator();
        ImGui::Text("Data Selection");


        //TODO select scalar and vector data together
        if (ImGui::BeginCombo("Dataset", currentDataset))
        {
            if (ImGui::Selectable(TOY_SCALAR_PATH))
            {
                if (currentScalarFile != TOY_SCALAR_PATH || currentVectorFile != TOY_VECTOR_PATH)
                {
                    currentScalarFile = TOY_SCALAR_PATH;
                    currentVectorFile = TOY_VECTOR_PATH;
                    currentDataset = TOY_DATASET;
                    
                    switchDataSet();
                }
            }

            if (ImGui::Selectable(BRAIN_SCALAR_PATH))
            {
                if (currentScalarFile != BRAIN_SCALAR_PATH || currentVectorFile != BRAIN_VECTOR_PATH)
                {
                    currentScalarFile = BRAIN_SCALAR_PATH;
                    currentVectorFile = BRAIN_VECTOR_PATH;
                    currentDataset = BRAIN_DATASET;
                    
                    switchDataSet();
                }
            }
            ImGui::EndCombo();
        }

        // Streamline parameters section
        ImGui::Separator();
        ImGui::Text("Streamline Parameters");

        ImGui::TextWrapped("Line width");
        if (ImGui::SliderFloat("##lineWidth", &lineWidth, 1.0f, 5.0f, "%.2f"))
        {
            //todo make linewidth relative to zoom level.
            streamlineRenderer->setLineWidth(lineWidth);
        }

        // Step size slider
        ImGui::TextWrapped("Step size");
        paramsChanged |= ImGui::SliderFloat("##stepSize", &stepSize, 0.1f, 2.0f, "%.3f");
        
        // Max length slider
        ImGui::TextWrapped("Max streamline length");
        paramsChanged |= ImGui::SliderFloat("##maxLength", &maxLength, 1.0f, 100.0f, "%.1f");
        
        // Max steps slider
        ImGui::TextWrapped("Max integration steps");
        paramsChanged |= ImGui::SliderInt("##maxSteps", &maxSteps, 1, 2000);

        // Max angle slider
        ImGui::TextWrapped("Max angle between steps (degrees)");
        if (ImGui::SliderFloat("##maxAngle", &maxAngleDegrees, 1.0f, 90.0f, "%.1f"))
        {
            maxAngle = maxAngleDegrees * (std::_Pi_val / 180);
            paramsChanged = true;
        }

        paramsChanged |= ImGui::Checkbox("FlipX", &(vectorField->flipX));
        paramsChanged |= ImGui::Checkbox("FlipY", &(vectorField->flipY));
        paramsChanged |= ImGui::Checkbox("FlipZ", &(vectorField->flipZ));

        // Streamline display section
        ImGui::Separator();
        ImGui::Text("Streamline Seeding");

        ImGui::TextWrapped("Coming soon: tensor eigenvector extraction");

        // Slice position control
        ImGui::TextWrapped("View axis");
        
        viewAxisChanged |= ImGui::RadioButton("axis_x", &selectedAxis, AXIS_X); ImGui::SameLine();
        viewAxisChanged |= ImGui::RadioButton("axis_y", &selectedAxis, AXIS_Y); ImGui::SameLine();
        viewAxisChanged |= ImGui::RadioButton("axis_z", &selectedAxis, AXIS_Z);
        paramsChanged |= viewAxisChanged;
        if (viewAxisChanged)
        {
            std::cout << "hello world" << std::endl;
            updatePVMatrices();
            viewAxisChanged = false;
        }

        int maxSliceIndexX = dimX - 1;
        int maxSliceIndexY = dimY - 1;
        int maxSliceIndexZ = dimZ - 1;
        paramsChanged |= ImGui::SliderInt("Slice X", &currentSliceX, 0, maxSliceIndexX);
        paramsChanged |= ImGui::SliderInt("Slice Y", &currentSliceY, 0, maxSliceIndexY);
        paramsChanged |= ImGui::SliderInt("Slice Z", &currentSliceZ, 0, maxSliceIndexZ);

        paramsChanged |= ImGui::Checkbox("Mouse seeding", &useMouseSeeding);

        ImGui::BeginDisabled(!paramsChanged);
        if (ImGui::Button("Regenerate Streamlines")) {
            regenerateStreamLines();
        }
        ImGui::EndDisabled();

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    if (sliceVAO) {
        glDeleteVertexArrays(1, &sliceVAO);
        glDeleteBuffers(1, &sliceVBO);
        glDeleteBuffers(1, &sliceEBO);
    }

    if (texture) {
        glDeleteTextures(1, &texture);
    }

    delete vectorField;
    delete streamlineRenderer;
    delete sliceShader;
    delete streamlineShader;
    delete glyphShader;

    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (globalScalarData) {
        delete[] globalScalarData;
        globalScalarData = nullptr;
    }

    glfwTerminate();
    return 0;
}