#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#pragma once

#include <iostream>
#include <string>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
class Shader;
class VideoRenderer
{
public:
    VideoRenderer(int width, int height);
    ~VideoRenderer();

    // 传入帧数据格式为RGB24
    void updateFrame(const unsigned char *data);
    void render();

private:
    int m_width, m_height;
    GLFWwindow *window;
    // Shader *m_shader;
    std::unique_ptr<Shader> m_shader;

    unsigned int m_VBO, m_VAO, m_EBO;
    unsigned int m_texture1;

    void init();
    GLFWwindow *create_window(const std::string &title);

    void createVertex();
    void createTexture();

    static void framebuffer_size_cb(GLFWwindow *window, int width, int height);
    static void process_input(GLFWwindow *window);
};

#endif