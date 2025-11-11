/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "ImmediateRenderer2D.hpp"
#include "Path.hpp"
#include <GL/glew.h>
#include <array>

void ImmediateRenderer2D::Init()
{
    // Load shaders
    const std::filesystem::path vertex_file   = assets::locate_asset("Assets/shaders/basic.vert");
    const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/basic.frag");
    shader                                    = OpenGL::CreateShader(vertex_file, fragment_file);

    // Define quad vertex format
    struct vertex
    {
        float x, y; // position
        float s, t; // texture coordinates
    };

    // Quad vertices (centered at origin, from -0.5 to +0.5)
    const vertex vertices[] = {
        { -0.5f, -0.5f, 0.0f, 0.0f }, // bottom-left
        { +0.5f, -0.5f, 1.0f, 0.0f }, // bottom-right
        { +0.5f, +0.5f, 1.0f, 1.0f }, // top-right
        { -0.5f, +0.5f, 0.0f, 1.0f }, // top-left
    };

    // Triangle indices (two triangles form a quad)
    const unsigned short indices[] = {
        0, 1, 2, // first triangle
        0, 2, 3  // second triangle
    };

    indices_count = static_cast<GLsizei>(std::ssize(indices));

    // Create vertex buffer
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create index buffer
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create vertex array object
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
    glVertexAttribDivisor(0, 0);

    // Texture coordinate attribute (location 1)
    glEnableVertexAttribArray(1);
    const ptrdiff_t texcoord_offset = 2 * sizeof(float);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), reinterpret_cast<void*>(texcoord_offset));
    glVertexAttribDivisor(1, 0);

    // Unbind VAO and buffers
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void ImmediateRenderer2D::Shutdown()
{
    glDeleteVertexArrays(1, &vertex_array_object);
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &index_buffer);
    glDeleteProgram(shader.Shader);
}

void ImmediateRenderer2D::BeginScene(std::span<const float, 9> ndc_matrix)
{
    glUseProgram(shader.Shader);
    glUniformMatrix3fv(shader.UniformLocations.at("uToNDC"), 1, GL_FALSE, ndc_matrix.data());
    glUseProgram(0);
}

void ImmediateRenderer2D::EndScene()
{
}

void ImmediateRenderer2D::DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color)
{
    glUseProgram(shader.Shader);
    glBindVertexArray(vertex_array_object);
    // Set uniforms
    glUniformMatrix3fv(shader.UniformLocations.at("uModel"), 1, GL_FALSE, transform.data());

    // Convert texture_coords_lbrt (left, bottom, right, top) to texture coordinate transform matrix
    const float left   = texture_coords_lbrt[0];
    const float bottom = texture_coords_lbrt[1];
    const float right  = texture_coords_lbrt[2];
    const float top    = texture_coords_lbrt[3];

    const float u_scale  = right - left; // width in texture space
    const float v_scale  = top - bottom; // height in texture space
    const float u_offset = left;         // starting U coordinate
    const float v_offset = bottom;       // starting V coordinate

    std::array<float, 9> texcoord_transform{
        u_scale,  0.0f,     0.0f, // column 0: scale U
        0.0f,     v_scale,  0.0f, // column 1: scale V
        u_offset, v_offset, 1.0f  // column 2: offset UV
    };

    glUniformMatrix3fv(shader.UniformLocations.at("uTexCoordTransform"), 1, GL_FALSE, texcoord_transform.data());
    glUniform4f(shader.UniformLocations.at("uTint"), tint_color[0], tint_color[1], tint_color[2], tint_color[3]);

    // Bind texture and draw
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}
