#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D m_texture1;

void main()
{
    FragColor = texture(m_texture1, TexCoord);
};