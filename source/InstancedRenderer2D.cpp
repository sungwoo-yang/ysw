#include "InstancedRenderer2D.hpp"
#include "Path.hpp"
#include <fstream>
#include <numeric>
#include <sstream>

InstancedRenderer2D::InstancedRenderer2D(unsigned max_sprites)
{
    maxInstances = max_sprites;
    instanceData.reserve(max_sprites);
}

void InstancedRenderer2D::Init()
{
    // get max texture units
    // get glsl code and update the fragment shader
    // create the shader
    // set the binding values for textures array

    GLint max_tex_units = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_tex_units);
    textureSlots.resize(std::min(max_tex_units, 64));

    // load shaders
    const std::filesystem::path vertex_file = assets::locate_asset("Assets/shaders/instance.vert");
    std::ifstream               vert_stream(vertex_file);
    std::stringstream           vert_text_stream;
    vert_text_stream << vert_stream.rdbuf();
    const std::string vertex_glsl = vert_text_stream.str();


    const std::filesystem::path fragment_file = assets::locate_asset("Assets/shaders/instance.frag");
    std::ifstream               frag_stream(fragment_file);
    std::stringstream           frag_text_stream;
    frag_text_stream << frag_stream.rdbuf();
    std::string       frag_glsl     = frag_text_stream.str();
    const size_t      first_newline = frag_glsl.find('\n');
    const std::string define_line   = "\n#define MAX_TEXTURE_SLOTS " + std::to_string(textureSlots.size());
    frag_glsl.insert(first_newline, define_line);
    shader = OpenGL::CreateShader(std::string_view{ vertex_glsl }, std::string_view{ frag_glsl });

    glUseProgram(shader.Shader);

    std::vector<int> sampler_binding_values(textureSlots.size());
    std::iota(sampler_binding_values.begin(), sampler_binding_values.end(), 0);
    const GLint location = glGetUniformLocation(shader.Shader, "uTextures");
    glUniform1iv(location, static_cast<GLsizei>(textureSlots.size()), sampler_binding_values.data());
    glUseProgram(0);

    // create our fixed buffer data
    //  create index buffer data
    // create our instanced buffer
    //  create VAO

    constexpr float fixed_sprite_vertices[] = { // bottom left
                                                -0.5f, -0.5f, 0.0f, 0.0f,
                                                // bottom right
                                                0.5f, -0.5f, 1.0f, 0.0f,
                                                // top right
                                                0.5f, 0.5f, 1.0f, 1.0f,
                                                // top left
                                                -0.5f, 0.5f, 0.0f, 1.0f
    };
    constexpr unsigned char indicies[] = { 0, 1, 2, 0, 2, 3 };

    glGenBuffers(1, &fixedVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, fixedVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fixed_sprite_vertices), fixed_sprite_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenBuffers(1, &instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadInstance) * maxInstances, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, fixedVertexBuffer);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
    glVertexAttribDivisor(0, 0);

    // Texture coordinate attribute (location 1)
    glEnableVertexAttribArray(1);
    const ptrdiff_t texcoord_offset = 2 * sizeof(float);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(texcoord_offset));
    glVertexAttribDivisor(1, 0);

    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);

    glEnableVertexAttribArray(2);
    const ptrdiff_t modelrow0_offset = offsetof(QuadInstance, transformrow0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(modelrow0_offset));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    const ptrdiff_t modelrow1_offset = offsetof(QuadInstance, transformrow1);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(modelrow1_offset));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    const ptrdiff_t tint_offset = offsetof(QuadInstance, tint);
    glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadInstance), reinterpret_cast<void*>(tint_offset));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    const ptrdiff_t tex_scale_offset = offsetof(QuadInstance, texScale);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(tex_scale_offset));
    glVertexAttribDivisor(5, 1);

    glEnableVertexAttribArray(6);
    const ptrdiff_t tex_offset_offset = offsetof(QuadInstance, texOffset);
    glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), reinterpret_cast<void*>(tex_offset_offset));
    glVertexAttribDivisor(6, 1);

    glEnableVertexAttribArray(7);
    const ptrdiff_t tex_index_offset = offsetof(QuadInstance, textureIndex);
    glVertexAttribIPointer(7, 1, GL_INT, sizeof(QuadInstance), reinterpret_cast<void*>(tex_index_offset));
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void InstancedRenderer2D::Shutdown()
{
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &fixedVertexBuffer);
    glDeleteBuffers(1, &instanceBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteProgram(shader.Shader);

    vertexArrayObject = 0;
    fixedVertexBuffer = 0;
    instanceBuffer    = 0;
    indexBuffer       = 0;
    shader.Shader     = 0;
}

void InstancedRenderer2D::BeginScene(std::span<const float, 9> ndc_matrix)
{
    glUseProgram(shader.Shader);
    glUniformMatrix3fv(shader.UniformLocations.at("uToNDC"), 1, GL_FALSE, ndc_matrix.data());
    glUseProgram(0);

    startBatch();
}

void InstancedRenderer2D::EndScene()
{
    flush();
}

void InstancedRenderer2D::DrawQuad(std::span<const float, 9> transform, OpenGL::Handle texture, std::span<const float, 4> texture_coords_lbrt, std::span<const float, 4> tint_color)
{
    if (instanceData.size() >= maxInstances)
    {
        flush();
    }

    // find texture slot
    int  tex_index = 0;
    bool found     = false;
    for (size_t i = 0; i < activeTexuresSize; ++i)
    {
        if (textureSlots[i] == texture)
        {
            found     = true;
            tex_index = static_cast<int>(i);
        }
    }
    if (!found)
    {
        if (activeTexuresSize >= textureSlots.size())
        {
            flush();
        }
        tex_index                       = static_cast<int>(activeTexuresSize);
        textureSlots[activeTexuresSize] = texture;
        ++activeTexuresSize;
    }

    const float left   = texture_coords_lbrt[0];
    const float bottom = texture_coords_lbrt[1];
    const float right  = texture_coords_lbrt[2];
    const float top    = texture_coords_lbrt[3];

    QuadInstance instance;

    instance.textureIndex = tex_index;

    instance.texScale[0]  = right - left;
    instance.texScale[1]  = top - bottom;
    instance.texOffset[0] = left;
    instance.texOffset[1] = bottom;

    instance.transformrow0[0] = transform[0];
    instance.transformrow0[1] = transform[3];
    instance.transformrow0[2] = transform[6];

    instance.transformrow1[0] = transform[1];
    instance.transformrow1[1] = transform[4];
    instance.transformrow1[2] = transform[7];

    instance.tint[0] = static_cast<unsigned char>(tint_color[0] * 255.0f);
    instance.tint[1] = static_cast<unsigned char>(tint_color[1] * 255.0f);
    instance.tint[2] = static_cast<unsigned char>(tint_color[2] * 255.0f);
    instance.tint[3] = static_cast<unsigned char>(tint_color[3] * 255.0f);

    instanceData.push_back(instance);
}

void InstancedRenderer2D::startBatch()
{
    instanceData.clear();
    activeTexuresSize = 0;
}

void InstancedRenderer2D::flush()
{
    if (instanceData.empty()) [[unlikely]]
        return;

    // update the instance buffer data
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(QuadInstance) * instanceData.size(), instanceData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // set texure samplers
    for (size_t i = 0; i < activeTexuresSize; ++i)
    {
        glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + i));
        glBindTexture(GL_TEXTURE_2D, textureSlots[i]);
    }

    glUseProgram(shader.Shader);
    glBindVertexArray(vertexArrayObject);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr, static_cast<GLsizei>(instanceData.size()));

    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    startBatch();
}
