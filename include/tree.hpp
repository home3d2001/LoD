#pragma once

#if defined(__APPLE__)
    #include <OpenGL/glew.h>
#else
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        #include <windows.h>
    #endif
    #include <GL/glew.h>
#endif

#include "oglwrap/oglwrap.hpp"
#include "oglwrap/shapes/mesh.hpp"
#include "oglwrap/utils/camera.hpp"

#include "charmove.hpp"
#include "skybox.h"
#include "terrain.h"
#include <cstdio>
#include <ctime>

class Tree {
public:
    oglwrap::Mesh mesh;
private:
    oglwrap::Program prog;
    oglwrap::VertexShader vs;
    oglwrap::FragmentShader fs;

    oglwrap::LazyUniform<glm::mat4> projectionMatrix, cameraMatrix, modelMatrix;
    oglwrap::LazyUniform<glm::vec4> sunData;

    Skybox& skybox;

    glm::vec3 scales;
    struct TreeInfo {
        glm::vec3 pos;
        glm::mat4 mat;

        TreeInfo(const glm::vec3& pos, const glm::mat4& mat)
            : pos(pos), mat(mat) {
        }
    };

    std::vector<TreeInfo> trees;
public:
    Tree(Skybox& skybox, const Terrain& terrain)
        : mesh("models/trees/trees_0/tree_1.obj",
           aiProcessPreset_TargetRealtime_MaxQuality |
           aiProcess_FlipUVs
          )
        , vs("tree.vert")
        , fs("tree.frag")
        , projectionMatrix(prog, "u_ProjectionMatrix")
        , cameraMatrix(prog, "u_CameraMatrix")
        , modelMatrix(prog, "u_ModelMatrix")
        , sunData(prog, "SunData")
        , skybox(skybox) {

        prog << vs << fs << skybox.sky_fs;
        prog.link().use();

        mesh.setup_positions(prog | "a_Position");
        mesh.setup_texCoords(prog | "a_TexCoord");
        mesh.setup_normals(prog | "a_Normal");
        oglwrap::UniformSampler(prog, "EnvMap").set(0);

        mesh.setup_diffuse_textures(1);
        oglwrap::UniformSampler(prog, "u_DiffuseTexture").set(1);

        // Get the trees' positions.
        srand(5);
        scales = terrain.getScales();
        const int treeDist = 200;
        for(int i = -terrain.h / 2; i <= terrain.h / 2; i += treeDist) {
            for(int j = -terrain.w / 2; j <= terrain.w / 2; j += treeDist) {
                glm::ivec2 coord = glm::ivec2(i + rand()%(treeDist/2) - treeDist/4,
                                              j + rand()%(treeDist/2) - treeDist/4);
                glm::vec3 pos = scales * glm::vec3(coord.x, terrain.fetchHeight(coord), coord.y);
                glm::vec3 scale = glm::vec3(
                    0.5f + 0.5f * rand() / RAND_MAX,
                    0.5f + 0.5f * rand() / RAND_MAX,
                    0.5f + 0.5f * rand() / RAND_MAX
                );

                float rotation = 360.0f * rand() / RAND_MAX;

                glm::mat4 matrix = glm::rotate(glm::mat4(), rotation, glm::vec3(0, 1, 0));
                matrix[3] = glm::vec4(pos, 1);
                matrix = glm::scale(matrix, scale);

                trees.push_back(TreeInfo(pos, matrix));
            }
        }

    }

    void reshape(glm::mat4 projMat) {
        prog.use();
        projectionMatrix = projMat;
    }

    void render(float time, const oglwrap::Camera& cam) {
        prog.use();
        cameraMatrix.set(cam.cameraMatrix());
        sunData.set(skybox.getSunData(time));
        skybox.envMap.active(0);
        skybox.envMap.bind();

        gl( Enable(GL_BLEND) );
        gl( BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );

        auto campos = cam.getPos();
        for(size_t i = 0; i < trees.size(); i++) {
            if(glm::length(campos - trees[i].pos) < std::max(scales.x, scales.z) * 600) {
                modelMatrix.set(trees[i].mat);
                mesh.render();
            }
        }

        gl( Disable(GL_BLEND) );

        skybox.envMap.active(0);
        skybox.envMap.unbind();
    }
};
