#include <glad/glad.h>
#include <iostream>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "DataReader.h"

void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int render(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    //make sure we use the core profile
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello World", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Initialize GLAD */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to load GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);

    Shader shader1("VertexShader1.vs", "FragShader1.fs");

    /*float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f
    };*/


    //read in the data
    //std::vector<std::vector<std::vector<float>>> data;
    float* data;
    short dimX, dimY, dimZ;
    readData(data, dimX, dimY, dimZ);

    //std::cout << dimX <<" "<< dimY <<" "<< dimZ << std::endl;

    //printSlice(data, 0, dimX, dimY);

    unsigned int texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_3D, texture);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, dimX, dimY, dimZ, 0, GL_LUMINANCE, GL_FLOAT, data);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, dimX, dimY, dimZ, 0, GL_RED, GL_FLOAT, data);

    free(data);

    float vertices[] = {
        //positions            //colors             //texture coords
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f
    };
    unsigned int indices[] = {
        0,1,2,
        1,2,3
    };

    //create elements buffer
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    //create vertex buffer
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    //create vertex array
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    //bind vertex array and buffer
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT), (void*)(6 * sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(2);


    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        //clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);

        //render stuff
        glBindTexture(GL_TEXTURE_3D, texture);

        shader1.use();
        //glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        
        glBindVertexArray(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main(void)
{
    /*std::vector<std::vector<std::vector<float>>> data;
    readData(data);
    for (int x = 0; x < data.size(); x++)
    {
        for (int y = 0; y < data[0].size(); y++)
        {
            for (int z = 0; z < data[0][0].size(); z++)
            {
                printf("%f ", data[x][y][z]);
            }
            printf("\n");
        }
        printf("\n");
    }*/

    render();
    return 0;
}