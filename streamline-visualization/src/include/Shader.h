#pragma once
#ifndef SHADER_H
#define SHADER_H

#include "../extra/glad.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(_POSIX_VERSION)
#include <unistd.h>
#endif

/**
 * @class Shader
 * @brief OpenGL shader program wrapper
 *
 * This class handles the loading, compilation, and management of OpenGL shader programs.
 * It provides utilities for setting uniform values and managing shader state.
 *
 * Based on https://learnopengl.com/Getting-started/Shaders
 */
class Shader
{
public:
    /** @brief Shader program ID in OpenGL */
    unsigned int ID;

    /**
     * @brief Create a shader program from vertex and fragment shader files
     * @param vertexPath Path to the vertex shader source file
     * @param fragmentPath Path to the fragment shader source file
     */
    Shader(const char* vertexPath, const char* fragmentPath)
    {
       // /*
        // Retrieve shader source code from file paths
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // Enable exceptions for file handling
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            // Open shader files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // Read file contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // Close file handlers
            vShaderFile.close();
            fShaderFile.close();

            // Convert streams into strings
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
            std::cout << strerror(errno) << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // Compile shaders
        unsigned int vertex, fragment;

        // Vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Log shader compilation completion
        std::cout << "Shader compilation completed. Shader ID: " << ID << std::endl;

        // Delete the individual shaders as they're now linked into the program
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        //*/
        /*
        //retrieve vertex/fragment code from file path
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            //open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            //read buffers
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            //close file handlers
            vShaderFile.close();
            fShaderFile.close();

            //convert streams into strings
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        //compile shaders
        unsigned int vertex, fragment;

        //vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        //fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        //shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        //delete the shaders now since they've been linked into the program
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        */
    }

    /**
     * @brief Activate the shader program
     */
    void use()
    {
        glUseProgram(ID);
    }

    /**
     * @brief Set a boolean uniform value
     * @param name Name of the uniform
     * @param value Boolean value to set
     */
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    /**
     * @brief Set an integer uniform value
     * @param name Name of the uniform
     * @param value Integer value to set
     */
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    /**
     * @brief Set a float uniform value
     * @param name Name of the uniform
     * @param value Float value to set
     */
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    /**
     * @brief Set a 4x4 matrix uniform value
     * @param name Name of the uniform
     * @param mat Matrix value to set
     */
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    /**
     * @brief Check for compilation or linking errors
     * @param shader Shader or program ID to check
     * @param type Type of check ("VERTEX", "FRAGMENT", or "PROGRAM")
     */
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];

        if (type != "PROGRAM")
        {
            // Check shader compilation
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            // Check program linking
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

#endif // !SHADER_H