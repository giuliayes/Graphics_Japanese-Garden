#ifndef Mesh_hpp
#define Mesh_hpp

#if defined (__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <glm/glm.hpp>
#include "Shader.hpp"
#include <string>
#include <vector>

namespace gps {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

    struct Texture {
        GLuint id;
        std::string type; // diffuseTexture, specularTexture
        std::string path;
    };

    struct Buffers {
        GLuint VAO;
        GLuint VBO;
        GLuint EBO;
    };

    class Mesh {
    public:
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        // NEW: material support
        glm::vec3 materialDiffuse;
        bool hasDiffuseTexture;

        Mesh(std::vector<Vertex> vertices,
            std::vector<GLuint> indices,
            std::vector<Texture> textures,
            glm::vec3 materialDiffuse = glm::vec3(1.0f));

        Buffers getBuffers();
        void Draw(gps::Shader shader);

    private:
        Buffers buffers;
        void setupMesh();
    };
}

#endif /* Mesh_hpp */
