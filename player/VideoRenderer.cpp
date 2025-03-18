#include "VideoRenderer.h"

#include "Shader.h"

VideoRenderer::VideoRenderer(int width, int height) : m_width(width), m_height(height)
{
    init();

    m_shader = std::make_unique<Shader>("../shader/player.vs", "../shader/player.fs");

    createVertex();

    createTexture();
}

VideoRenderer::~VideoRenderer()
{
    glDeleteTextures(1, &m_texture1);
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
}

void VideoRenderer::updateFrame(const unsigned char *data)
{
    // 使用子图更新（整个纹理）
    glBindTexture(GL_TEXTURE_2D, m_texture1);
    // 效率更高, 不需要重新分配显存
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, data);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

void VideoRenderer::render()
{
    // 清屏，显示指定背景色
    // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    // 清理画面
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture1);

    m_shader->use();

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void VideoRenderer::init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = create_window("PlayerOpenGl");
}

GLFWwindow *VideoRenderer::create_window(const std::string &title)
{
    // 创建窗口
    window = glfwCreateWindow(m_width, m_height, title.c_str(), nullptr, nullptr);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return window;
    }
    // 设置当前线程渲染窗口
    glfwMakeContextCurrent(window);
    // 设置窗口大小变化时的回调，一般用于调整视口大小
    glfwSetFramebufferSizeCallback(window, framebuffer_size_cb);

    //
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return window;
    }
    return window;
}

void VideoRenderer::createVertex()
{

    // 顶点数组对象
    float vertices[] = {
        //---- 位置 ----   - 纹理坐标 -
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // 右上
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // 右下
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 左下
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // 左上
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 设置顶点属性指针, 如何解析顶点数据， 任何随后的顶点属性调用都会储存在这个VAO中
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), (void *)0);
    glEnableVertexAttribArray(0);

    // 纹理属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), (void *)(3 * sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void VideoRenderer::createTexture()
{
    // 创建纹理对象
    glGenTextures(1, &m_texture1);
    glBindTexture(GL_TEXTURE_2D, m_texture1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_shader->setInt("m_texture1", 0);
}

void VideoRenderer::framebuffer_size_cb(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void VideoRenderer::process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        std::cout << "press KEY_ESCAPE, Window Should Close" << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
}
