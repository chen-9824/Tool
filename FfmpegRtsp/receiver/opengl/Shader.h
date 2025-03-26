#ifndef SHADER_H
#define SHADER_H

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
class Shader
{
public:
    unsigned int ID;
    Shader(const char *vertexPath, const char *fragmentPath);
    ~Shader();

    // 使用/激活程序
    void use();
    void delete_shader();
    // uniform工具函数
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void set4F(const std::string &name, float value1, float value2, float value3, float value4) const;

private:
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif