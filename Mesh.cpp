#include "Mesh.hpp"

namespace gps {

    Mesh::Mesh(std::vector<Vertex> vertices,
        std::vector<GLuint> indices,
        std::vector<Texture> textures,
        glm::vec3 materialDiffuse)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->materialDiffuse = materialDiffuse;

        hasDiffuseTexture = false;
        for (auto& t : textures) {
            if (t.type == "diffuseTexture") {
                hasDiffuseTexture = true;
                break;
            }
        }

        setupMesh();
    }

    Buffers Mesh::getBuffers() {
        return buffers;
    }

    void Mesh::Draw(gps::Shader shader)
    {
        shader.useShaderProgram();

        // material uniforms
        glUniform3fv(
            glGetUniformLocation(shader.shaderProgram, "materialDiffuse"),
            1,
            &materialDiffuse[0]
        );
        glUniform1i(
            glGetUniformLocation(shader.shaderProgram, "hasDiffuseTexture"),
            hasDiffuseTexture ? 1 : 0
        );

        // bind textures
        for (GLuint i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glUniform1i(
                glGetUniformLocation(shader.shaderProgram, textures[i].type.c_str()),
                i
            );
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(buffers.VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        for (GLuint i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void Mesh::setupMesh()
    {
        glGenVertexArrays(1, &buffers.VAO);
        glGenBuffers(1, &buffers.VBO);
        glGenBuffers(1, &buffers.EBO);

        glBindVertexArray(buffers.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, buffers.VBO);
        glBufferData(GL_ARRAY_BUFFER,
            vertices.size() * sizeof(Vertex),
            &vertices[0],
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(GLuint),
            &indices[0],
            GL_STATIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (void*)0);

        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // texcoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
}
