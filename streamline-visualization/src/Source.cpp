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
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>


#include <string>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

#include "include/Shader.h"
#include "include/DataReader.h"
#include "include/VectorField.h"
#include "include/StreamlineTracer.h"
#include "include/StreamlineRenderer.h"

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

//Paths
const char* BRAIN_SCALAR_PATH = "data/brain-map.nii";
const char* BRAIN_VECTOR_PATH = "data/brain-vectors.nii";
const char* BRAIN_TENSORS_PATH = "data/brain-tensors.nii";
const char* TOY_SCALAR_PATH = "data/toy-map.nii";
const char* TOY_VECTOR_PATH = "data/toy-evec.nii";

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

// Data variables
float* globalScalarData = nullptr;
int scalarDimX = 0, scalarDimY = 0, scalarDimZ = 0;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Visualization settings
int sliceVisualizationMode = 1;  // Default to anatomical view
bool showVectorFieldOverlay = false;
bool firstLoad = true;
std::string currentScalarFile = BRAIN_SCALAR_PATH;
std::string currentVectorFile = BRAIN_VECTOR_PATH;

// Streamline parameters
int seedDensity = 5;
float stepSize = 0.5f;
float minMagnitude = 0.001f;
float maxLength = 50.0f;
int maxSteps = 1000;
float lineWidth = 2.0f;
bool needReload = false;
int sliceAxis = 2;  // 0=X, 1=Y, 2=Z (Z by default)
float sliceAlpha = 0.5f;  // Default opacity for the slice

// Global objects
VectorField* vectorField = nullptr;
StreamlineRenderer* streamlineRenderer = nullptr;
Shader* sliceShader = nullptr;
Shader* streamlineShader = nullptr;
Shader* glyphShader = nullptr;
short dimX = 0, dimY = 0, dimZ = 0;
unsigned int texture = 0;
unsigned int sliceVAO = 0, sliceVBO = 0, sliceEBO = 0;
int currentSlice = 0;


// Interactive seeding
bool enableMouseSeeding = false;
std::vector<std::vector<Point3D>> manualStreamlines;
float manualSeedLineWidth = 3.0f;

// Orbit camera control
bool orbitMode = false;
glm::vec3 orbitCenter;
float orbitDistance = 0.0f;
bool isDragging = false;
float orbitYaw = 0.0f;
float orbitPitch = 0.0f;
double lastOrbitX = 0.0f;
double lastOrbitY = 0.0f;

// Camera mode
bool cameraMode = false;

// Seeding modes
enum SeedingMode {
    GRID_SEEDING = 0,
    UNIFIED_BRAIN_SEEDING = 1,
    TOY_DATASET_SEEDING = 2
};

int seedingMode = UNIFIED_BRAIN_SEEDING;  // Default to grid seeding

// Interactive mode flag for UI
bool interactiveMode = false;




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
void loadData();
void updateSlice();
void seedStreamlinesAtPoint(float x, float y, float z);
float sampleScalarData(float x, float y, float z);
std::vector<Point3D> generateAnatomicalSeeds(float* scalarData, VectorField* vectorField,
                                           float intensityThreshold, int seedDensity,
                                           int sliceAxis, int slicePos, int range);

/**
 * @brief Load data from files and prepare for visualization
 *
 * This function handles loading scalar and vector data, initializing OpenGL resources,
 * generating streamlines, and preparing visualization parameters.
 */
void loadData() {
    // Reset reload flag at the beginning to prevent recursive calls
    needReload = false;

    // Log start of data loading
    std::cout << "Starting data loading process..." << std::endl;

    // Identify dataset type based on filename
    bool isToyDataset = (currentScalarFile.find("toy") != std::string::npos);
    bool isBrainDataset = (currentScalarFile.find("brain") != std::string::npos);

    // Only set default values if they haven't been set already
    if (firstLoad) {
        // Initialize with defaults only the first time
        if (isToyDataset) {
            stepSize = 0.2f;
            minMagnitude = 0.005f;
            maxLength = 25.0f;
        } else if (isBrainDataset) {
            seedDensity = 12;
            stepSize = 0.5f;
            minMagnitude = 0.001f;
            maxLength = 60.0f;
            maxSteps = 1500;
        }
        firstLoad = false;
    }

    // Clean up old resources
    if (vectorField) {
        delete vectorField;
        vectorField = nullptr;
    }

    if (streamlineRenderer) {
        delete streamlineRenderer;
        streamlineRenderer = nullptr;
    }

    // Delete previous texture if it exists
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }

    // Clean up previous scalar data if it exists
    if (globalScalarData) {
        delete[] globalScalarData;
        globalScalarData = nullptr;
    }

    // Load scalar data
    float* scalarData = nullptr;
    if (readData(currentScalarFile.c_str(), scalarData, dimX, dimY, dimZ) != EXIT_SUCCESS) {
        std::cerr << "Failed to read scalar data from " << currentScalarFile << std::endl;
        return;
    }

    // Store dimensions for global access
    scalarDimX = dimX;
    scalarDimY = dimY;
    scalarDimZ = dimZ;

    // Create a copy of the scalar data for global access
    globalScalarData = new float[dimX * dimY * dimZ];
    std::memcpy(globalScalarData, scalarData, dimX * dimY * dimZ * sizeof(float));

    std::cout << "Loaded scalar data: " << dimX << "x" << dimY << "x" << dimZ << std::endl;

    // Ensure slice rendering buffers are properly initialized
    if (sliceVAO == 0) {
        glGenVertexArrays(1, &sliceVAO);
        glGenBuffers(1, &sliceVBO);
        glGenBuffers(1, &sliceEBO);
        std::cout << "Initialized slice rendering buffers" << std::endl;
    }

    // Setup or update the 3D texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    float borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, dimX, dimY, dimZ, 0, GL_RED, GL_FLOAT, scalarData);

    // Free original scalar data since we now have a copy
    delete[] scalarData;
    scalarData = nullptr;

    // Load vector field data
    try {
        vectorField = new VectorField(currentVectorFile.c_str());
        std::cout << "Loaded vector field successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading vector field: " << e.what() << std::endl;
        return;
    }

    // Set appropriate visualization mode and slice for brain data
    if (isBrainDataset) {
        // For brain data, choose a slice showing good anatomical features
        currentSlice = dimZ / 2 + 5; // Adjust to find a good slice
        sliceVisualizationMode = 1;  // Set to anatomical view by default
    } else {
        // For toy dataset or other data
        currentSlice = dimZ / 2;
    }

    // Always use Z-axis for slicing
    sliceAxis = 2;  // 0=X, 1=Y, 2=Z (Z by default)

    // Update the slice
    updateSlice();

    // Initialize shader parameters
    if (sliceShader) {
        sliceShader->use();
        sliceShader->setFloat("alpha", sliceAlpha);
        sliceShader->setInt("visualizationMode", sliceVisualizationMode);
        std::cout << "Set shader visualization mode to: " << sliceVisualizationMode << std::endl;
    } else {
        std::cerr << "Warning: Slice shader is null" << std::endl;
    }

    // Adjust camera position based on data dimensions
    cameraPos = glm::vec3(dimX/2.0f, dimY/2.0f, 2.0f * std::max(std::max(dimX, dimY), dimZ));

    // Only generate streamlines if vector field was loaded successfully
    if (vectorField) {
        try {
            // Create streamline tracer
            StreamlineTracer tracer(vectorField, stepSize, maxSteps, minMagnitude, maxLength);

            // Generate seed points based on selected seeding mode
            std::vector<Point3D> seeds;

            if (isToyDataset || seedingMode == TOY_DATASET_SEEDING) {
                // Use specialized toy dataset seeding for toy data
                seeds = tracer.generateToyDatasetSeeds(seedDensity);
                std::cout << "Using specialized toy dataset seeding with " << seeds.size() << " seeds" << std::endl;
            } else if (seedingMode == UNIFIED_BRAIN_SEEDING && isBrainDataset) {
                // Use unified brain seeding for brain data
                seeds = tracer.generateUnifiedBrainSeeds(seedDensity);

                // Fallback if needed
                if (seeds.size() < 100) {
                    std::cout << "Few unified brain seeds found, falling back to grid seeding" << std::endl;
                    seeds = tracer.generateSeedGrid(seedDensity, seedDensity, seedDensity);
                }

                std::cout << "Using unified brain seeding with " << seeds.size() << " seeds" << std::endl;
            } else {
                // Use grid seeding otherwise
                seeds = tracer.generateSeedGrid(seedDensity, seedDensity, seedDensity);
                std::cout << "Using grid seeding with " << seeds.size() << " seeds" << std::endl;
            }

            // Trace streamlines from all seed points
            std::vector<std::vector<Point3D>> streamlines;
            if (!seeds.empty()) {
                streamlines = tracer.traceAllStreamlines(seeds);
                std::cout << "Generated " << streamlines.size() << " streamlines" << std::endl;
            } else {
                std::cout << "No seeds generated, skipping streamline tracing" << std::endl;
            }


            // Add manual streamlines if they exist
            if (!manualStreamlines.empty()) {
                streamlines.insert(streamlines.end(), manualStreamlines.begin(), manualStreamlines.end());
                std::cout << "Added " << manualStreamlines.size() << " manual streamlines" << std::endl;
            }

            // Create streamline renderer with proper coloring mode
            if (streamlineShader) {
                streamlineRenderer = new StreamlineRenderer(streamlineShader, lineWidth);
                streamlineRenderer->prepareStreamlines(streamlines, isToyDataset);
                std::cout << "Streamline renderer initialized with " << streamlines.size() << " streamlines" << std::endl;
            } else {
                std::cerr << "Warning: Streamline shader is null" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error generating streamlines: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Warning: Vector field is null, skipping streamline generation" << std::endl;
    }

    // Loading is complete
    std::cout << "Data loading complete" << std::endl;
    needReload = false;
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

    // Calculate camera movement speed based on delta time for smooth movement
    float cameraSpeed = 5.0f * deltaTime;

    // Increase speed when in camera mode for faster navigation
    if (cameraMode) {
        cameraSpeed *= 2.0f;
    }

    // WASD keys for horizontal movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Move forward in camera direction
        cameraPos += cameraSpeed * cameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        // Move backward from camera direction
        cameraPos -= cameraSpeed * cameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        // Strafe left (perpendicular to camera direction)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        // Strafe right (perpendicular to camera direction)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }

    // Vertical movement controls
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        // Move up
        cameraPos += cameraUp * cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        // Move down
        cameraPos -= cameraUp * cameraSpeed;
        }
}

/**
 * @brief Update the slice geometry and texture coordinates
 *
 * This function updates the OpenGL buffers for the slice plane based on the current slice settings.
 * It creates a textured quad representing a slice through the 3D volume at the current position.
 */
void updateSlice() {
    // Validate current slice index against maximum bounds
    int maxSlice = dimZ - 1;
    currentSlice = std::min(std::max(0, currentSlice), maxSlice);

    // Calculate normalized texture coordinate for the slice
    float sliceCoord = (float)currentSlice / (float)(maxSlice);

    // Set up slice for Z-axis only
    float zPos = currentSlice;

    // Define slice vertices (4 vertices, each with position, color, and texture coordinates)
    float sliceVertices[36]; // 4 vertices × 9 values per vertex

    // Format for each vertex: x, y, z, r, g, b, tex_s, tex_t, tex_r

    // Bottom-left vertex
    sliceVertices[0] = 0.0f;         sliceVertices[1] = 0.0f;   sliceVertices[2] = zPos;   // Position
    sliceVertices[3] = 0.3f;         sliceVertices[4] = 0.3f;   sliceVertices[5] = 0.3f;   // Color
    sliceVertices[6] = 0.0f;         sliceVertices[7] = 0.0f;   sliceVertices[8] = sliceCoord; // Texcoords

    // Top-left vertex
    sliceVertices[9] = 0.0f;         sliceVertices[10] = dimY;  sliceVertices[11] = zPos;   // Position
    sliceVertices[12] = 0.3f;        sliceVertices[13] = 0.3f;  sliceVertices[14] = 0.3f;   // Color
    sliceVertices[15] = 0.0f;        sliceVertices[16] = 1.0f;  sliceVertices[17] = sliceCoord; // Texcoords

    // Bottom-right vertex
    sliceVertices[18] = dimX;        sliceVertices[19] = 0.0f;  sliceVertices[20] = zPos;   // Position
    sliceVertices[21] = 0.3f;        sliceVertices[22] = 0.3f;  sliceVertices[23] = 0.3f;   // Color
    sliceVertices[24] = 1.0f;        sliceVertices[25] = 0.0f;  sliceVertices[26] = sliceCoord; // Texcoords

    // Top-right vertex
    sliceVertices[27] = dimX;        sliceVertices[28] = dimY;  sliceVertices[29] = zPos;   // Position
    sliceVertices[30] = 0.3f;        sliceVertices[31] = 0.3f;  sliceVertices[32] = 0.3f;   // Color
    sliceVertices[33] = 1.0f;        sliceVertices[34] = 1.0f;  sliceVertices[35] = sliceCoord; // Texcoords

    // Update the vertex buffer with slice geometry
    glBindVertexArray(sliceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sliceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sliceVertices), sliceVertices, GL_STATIC_DRAW);

    // Set up vertex attributes for position (3), color (3), and texture coordinates (3)
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Define triangle indices for rendering the quad as two triangles
    unsigned int sliceIndices[] = {
        0, 1, 2,  // First triangle (bottom-left, top-left, bottom-right)
        1, 3, 2   // Second triangle (top-left, top-right, bottom-right)
    };

    // Update the element buffer with the indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sliceEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sliceIndices), sliceIndices, GL_STATIC_DRAW);
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
    // Handle orbit camera rotation
    if (orbitMode && isDragging) {
        float xoffset = xpos - lastOrbitX;
        float yoffset = lastOrbitY - ypos;
        lastOrbitX = xpos;
        lastOrbitY = ypos;

        const float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        orbitYaw += xoffset;
        orbitPitch += yoffset;

        // Clamp pitch to avoid flipping
        if (orbitPitch > 89.0f)
            orbitPitch = 89.0f;
        if (orbitPitch < -89.0f)
            orbitPitch = -89.0f;

        // Calculate new camera position based on orbit angles
        float horizontalDistance = orbitDistance * cos(glm::radians(orbitPitch));
        float verticalDistance = orbitDistance * sin(glm::radians(orbitPitch));

        float offsetX = horizontalDistance * sin(glm::radians(orbitYaw));
        float offsetZ = horizontalDistance * cos(glm::radians(orbitYaw));

        // Update camera position to orbit around center
        cameraPos.x = orbitCenter.x + offsetX;
        cameraPos.y = orbitCenter.y + verticalDistance;
        cameraPos.z = orbitCenter.z + offsetZ;

        // Update camera front vector to look at center
        cameraFront = glm::normalize(orbitCenter - cameraPos);

        // Update yaw and pitch for when we exit orbit mode
        yaw = orbitYaw;
        pitch = orbitPitch;

        return;
    }

    // Standard camera control (non-orbit)
    bool moveCamera = cameraMode || (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
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

    // Apply sensitivity adjustment
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Update camera angles
    yaw += xoffset;
    pitch += yoffset;

    // Clamp pitch to prevent flipping
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Calculate new camera direction
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
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
    // Check if vector field exists
    if (!vectorField) return;

    // Clamp coordinates to valid range
    x = std::max(0.0f, std::min(x, (float)dimX - 1.0f));
    y = std::max(0.0f, std::min(y, (float)dimY - 1.0f));
    z = std::max(0.0f, std::min(z, (float)dimZ - 1.0f));

    std::cout << "Seeding streamline at (" << x << ", " << y << ", " << z << ")" << std::endl;

    // Create a seed point
    Point3D seed(x, y, z);

    // Create a streamline tracer with current parameters
    StreamlineTracer tracer(vectorField, stepSize, maxSteps, minMagnitude, maxLength);

    // Trace streamline in both directions from the seed
    std::vector<Point3D> newStreamline = tracer.traceStreamline(seed);

    // Only add if streamline has sufficient points
    if (newStreamline.size() > 2) {
        // Store in manual streamlines collection for persistence
        manualStreamlines.push_back(newStreamline);

        // Prepare container for just this new streamline
        std::vector<std::vector<Point3D>> singleStreamline;
        singleStreamline.push_back(newStreamline);

        // Add to existing renderer for immediate visualization
        if (streamlineRenderer) {
            // Save current line width
            float oldWidth = lineWidth;

            // Use special width for manually seeded streamlines
            glLineWidth(manualSeedLineWidth);

            // Add to renderer
            streamlineRenderer->addStreamlines(singleStreamline);

            // Restore original line width
            glLineWidth(oldWidth);
        }

        std::cout << "Added a new streamline with " << newStreamline.size() << " points" << std::endl;
    } else {
        std::cout << "Generated streamline was too short (" << newStreamline.size() << " points)" << std::endl;
    }
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
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    (void)mods;  // Unused parameter

    // Handle left mouse button
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Start orbit rotation if in orbit mode
            if (orbitMode) {
                isDragging = true;

                // Get cursor position
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                lastOrbitX = xpos;
                lastOrbitY = ypos;

                // Calculate orbit center and distance if not already set
                if (orbitDistance == 0.0f) {
                    // Set orbit center to center of data volume
                    orbitCenter = glm::vec3(dimX/2.0f, dimY/2.0f, dimZ/2.0f);

                    // Calculate distance from camera to center
                    orbitDistance = glm::length(cameraPos - orbitCenter);

                    // Calculate initial yaw and pitch based on current camera position
                    orbitYaw = yaw;
                    orbitPitch = pitch;
                }
            }
            // For interactive streamline seeding
            else if (enableMouseSeeding) {
                // Get cursor position
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);

                // Get window size
                int width, height;
                glfwGetWindowSize(window, &width, &height);

                // Convert to normalized device coordinates (-1 to 1)
                float ndcX = (2.0f * xpos / width) - 1.0f;
                float ndcY = 1.0f - (2.0f * ypos / height);

                // Create clip space positions (near and far planes)
                glm::vec4 clipPosNear = glm::vec4(ndcX, ndcY, -1.0f, 1.0f); // Near plane
                glm::vec4 clipPosFar = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);   // Far plane

                // Convert to world space
                glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / height, 0.1f, 1000.0f);
                glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
                glm::mat4 invProjView = glm::inverse(projection * view);

                glm::vec4 worldPosNear = invProjView * clipPosNear;
                worldPosNear /= worldPosNear.w;

                glm::vec4 worldPosFar = invProjView * clipPosFar;
                worldPosFar /= worldPosFar.w;

                // Calculate ray for intersection test
                glm::vec3 rayOrigin = cameraPos;
                glm::vec3 rayDir = glm::normalize(glm::vec3(worldPosFar - worldPosNear));

                // Debug output
                std::cout << "Ray origin: (" << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << ")" << std::endl;
                std::cout << "Ray direction: (" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << ")" << std::endl;

                // Calculate intersection with current slice plane
                bool hit = false;
                glm::vec3 hitPos;

                // Calculate intersection based on the active slice axis
                if (sliceAxis == 0) { // X-axis slice
                    float t = (currentSlice - rayOrigin.x) / rayDir.x;
                    if (t > 0) { // Check if intersection is in front of camera
                        hitPos = rayOrigin + rayDir * t;
                        if (hitPos.y >= 0 && hitPos.y < dimY && hitPos.z >= 0 && hitPos.z < dimZ) {
                            hit = true;
                            seedStreamlinesAtPoint(currentSlice, hitPos.y, hitPos.z);
                        }
                    }
                }
                else if (sliceAxis == 1) { // Y-axis slice
                    float t = (currentSlice - rayOrigin.y) / rayDir.y;
                    if (t > 0) {
                        hitPos = rayOrigin + rayDir * t;
                        if (hitPos.x >= 0 && hitPos.x < dimX && hitPos.z >= 0 && hitPos.z < dimZ) {
                            hit = true;
                            seedStreamlinesAtPoint(hitPos.x, currentSlice, hitPos.z);
                        }
                    }
                }
                else { // Z-axis slice
                    float t = (currentSlice - rayOrigin.z) / rayDir.z;
                    if (t > 0) {
                        hitPos = rayOrigin + rayDir * t;
                        if (hitPos.x >= 0 && hitPos.x < dimX && hitPos.y >= 0 && hitPos.y < dimY) {
                            hit = true;
                            seedStreamlinesAtPoint(hitPos.x, hitPos.y, currentSlice);
                        }
                    }
                }

                // Log the result of the intersection test
                if (hit) {
                    std::cout << "Hit slice at position: (" << hitPos.x << ", " << hitPos.y << ", " << hitPos.z << ")" << std::endl;
                } else {
                    std::cout << "No intersection with current slice" << std::endl;
                }
            }
        }
        else if (action == GLFW_RELEASE) {
            isDragging = false;
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
    // Unused parameters
    (void)scancode;
    (void)mods;

    // Toggle camera control mode with Tab key
    if (keyCode == GLFW_KEY_TAB && actionType == GLFW_PRESS) {
        cameraMode = !cameraMode;
        if (cameraMode) {
            // When in camera mode, hide the cursor and capture mouse movement
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            // When not in camera mode, show the cursor for UI interaction
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
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

    // Adjust field of view (zoom) based on scroll wheel
    fov -= (float)yoffset * 2.0f;

    // Clamp FOV to reasonable limits
    if (fov < 1.0f)
        fov = 1.0f;  // Maximum zoom level
    if (fov > 45.0f)
        fov = 45.0f; // Minimum zoom level
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
    glEnable(GL_DEPTH_TEST);
    glLineWidth(lineWidth);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create shaders
    sliceShader = new Shader("shaders/vertexShader1.vs", "shaders/FragShader1.fs");
    std::cout << "Slice shader loading attempted. Shader ID: " << sliceShader->ID << std::endl;

    streamlineShader = new Shader("shaders/streamlineVertex.vs", "shaders/streamlineFragment.fs");
    glyphShader = new Shader("shaders/glyphVertex.vs", "shaders/glyphFragment.fs");

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    // Set data file paths relative to the build directory
    currentScalarFile = "data/brain-map.nii";
    currentVectorFile = "data/brain-vectors.nii";

    std::cout << "Updated Scalar File Path: " << currentScalarFile << std::endl;
    std::cout << "Updated Vector File Path: " << currentVectorFile << std::endl;

    // Initial data loading
    loadData();

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);

        // Clear the screen
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create transformation matrices
        glm::mat4 projection = glm::perspective(glm::radians(fov),
                                              (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                              0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-dimX/2.0f, -dimY/2.0f, -dimZ/2.0f));
        model = glm::scale(model, glm::vec3(1.0f)); // Adjust scale if needed

        // Create offset model matrix for streamlines to avoid z-fighting
        glm::mat4 streamlineModel = model;
        streamlineModel = glm::translate(streamlineModel, glm::vec3(0.0f, 0.0f, 0.01f));

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
            sliceShader->setMat4("projection", projection);
            sliceShader->setMat4("view", view);
            sliceShader->setMat4("model", model);
            sliceShader->setFloat("alpha", sliceAlpha);
            sliceShader->setInt("visualizationMode", sliceVisualizationMode);

            glBindVertexArray(sliceVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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

        // Reset button
        if (ImGui::Button("Reset Camera")) {
            cameraPos = glm::vec3(dimX/2.0f, dimY/2.0f, 2.0f * std::max(std::max(dimX, dimY), dimZ));
            cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
            yaw = -90.0f;
            pitch = 0.0f;
        }

        // Orbit mode toggle
        ImGui::Checkbox("Orbit Camera Mode", &orbitMode);

        // Data selection section
        ImGui::Separator();
        ImGui::Text("Data Selection");

        // Dropdown for scalar data file
        if (ImGui::BeginCombo("Scalar Data", currentScalarFile.c_str())) {
            if (ImGui::Selectable(TOY_SCALAR_PATH)) {
                currentScalarFile = TOY_SCALAR_PATH;
                needReload = true;
            }
            if (ImGui::Selectable(BRAIN_SCALAR_PATH)) {
                currentScalarFile = BRAIN_SCALAR_PATH;
                needReload = true;
            }
            ImGui::EndCombo();
        }

        // Dropdown for vector data file
        if (ImGui::BeginCombo("Vector Data", currentVectorFile.c_str())) {
            if (ImGui::Selectable(TOY_VECTOR_PATH)) {
                currentVectorFile = TOY_VECTOR_PATH;
                needReload = true;
            }
            if (ImGui::Selectable(BRAIN_VECTOR_PATH)) {
                currentVectorFile = BRAIN_VECTOR_PATH;
                needReload = true;
            }
            ImGui::EndCombo();
        }

        // Streamline parameters section
        ImGui::Separator();
        ImGui::Text("Streamline Parameters");

        // Add specific flags to ensure the values change when the sliders move
        bool paramsChanged = false;

        // Seed density slider
        paramsChanged |= ImGui::SliderInt("Seed Density", &seedDensity, 1, 20);

        // Step size slider
        paramsChanged |= ImGui::SliderFloat("Step Size", &stepSize, 0.1f, 2.0f, "%.3f");

        // Min magnitude slider
        paramsChanged |= ImGui::SliderFloat("Min Magnitude", &minMagnitude, 0.0001f, 0.1f, "%.4f");

        // Max length slider
        paramsChanged |= ImGui::SliderFloat("Max Length", &maxLength, 10.0f, 100.0f, "%.1f");

        // Max steps slider
        paramsChanged |= ImGui::SliderInt("Max Steps", &maxSteps, 100, 2000);

        // Line width slider
        paramsChanged |= ImGui::SliderFloat("Line Width", &lineWidth, 0.5f, 5.0f, "%.2f");

        // If any parameter was changed, update line width immediately
        if (paramsChanged) {
            // Apply line width change immediately
            glLineWidth(lineWidth);
            if (streamlineRenderer) {
                streamlineRenderer->setLineWidth(lineWidth);
            }

            // Set flag to indicate changes that need reloading
            needReload = true;
        }

        // Add a clear indicator that parameters have changed and need reload
        if (needReload) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                               "Parameters changed - click Regenerate to apply");
        }

        // Streamline display section
        ImGui::Separator();
        ImGui::Text("Streamline Seeding");

        // Seeding method options
        ImGui::Text("Seeding Method:");
        bool seedingChanged = false;

        seedingChanged |= ImGui::RadioButton("Grid Seeding", &seedingMode, GRID_SEEDING);
        seedingChanged |= ImGui::RadioButton("Unified Brain Seeding", &seedingMode, UNIFIED_BRAIN_SEEDING);
        seedingChanged |= ImGui::RadioButton("Toy Dataset Seeding", &seedingMode, TOY_DATASET_SEEDING);

        if (seedingChanged) {
            needReload = true;
        }

        if (ImGui::Button("Regenerate Streamlines")) {
            if (vectorField && streamlineRenderer) {
                StreamlineTracer tracer(vectorField, stepSize, maxSteps, minMagnitude, maxLength);

                std::vector<Point3D> seeds;
                bool isBrainDataset = (currentScalarFile.find("brain") != std::string::npos);

                if (seedingMode == UNIFIED_BRAIN_SEEDING && isBrainDataset) {
                    seeds = tracer.generateUnifiedBrainSeeds(seedDensity);
                    if (seeds.size() < 100) {
                        seeds = tracer.generateSeedGrid(seedDensity, seedDensity, seedDensity);
                    }
                } else if (seedingMode == TOY_DATASET_SEEDING) {
                    seeds = tracer.generateToyDatasetSeeds(seedDensity);
                } else {
                    seeds = tracer.generateSeedGrid(seedDensity, seedDensity, seedDensity);
                }

                std::vector<std::vector<Point3D>> newStreamlines = tracer.traceAllStreamlines(seeds);
                newStreamlines.insert(newStreamlines.end(), manualStreamlines.begin(), manualStreamlines.end());
                streamlineRenderer->prepareStreamlines(newStreamlines);
            }
        }

        // Slice position control
        int maxSliceIndex = (sliceAxis == 0) ? dimX-1 : ((sliceAxis == 1) ? dimY-1 : dimZ-1);
        if (ImGui::SliderInt("Slice", &currentSlice, 0, maxSliceIndex)) {
            updateSlice();
        }

        // Slice opacity control
        if (ImGui::SliderFloat("Slice Opacity", &sliceAlpha, 0.0f, 1.0f)) {
            sliceShader->use();
            sliceShader->setFloat("alpha", sliceAlpha);
        }

        // Interactive seeding section
        ImGui::Separator();
        ImGui::Text("Interactive Seeding");
        if (ImGui::Checkbox("Interactive Mode", &interactiveMode)) {
            if (interactiveMode) {
                sliceAlpha = 0.7f;
                sliceShader->use();
                sliceShader->setFloat("alpha", sliceAlpha);
            }
        }

        if (interactiveMode) {
            ImGui::Checkbox("Enable Mouse Seeding", &enableMouseSeeding);
            ImGui::SliderFloat("Seeded Line Width", &manualSeedLineWidth, 1.0f, 5.0f);

            if (ImGui::Button("Clear Seed Streamlines")) {
                manualStreamlines.clear();
                needReload = true;
            }
        }

        // Reload button
        ImGui::Separator();
        if (ImGui::Button("Reload Data") || needReload) {
            loadData();
        }

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