#include "Scene/SceneDescription.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>

namespace ACS
{
    void ReadString(std::ifstream &file, std::string &str)
    {
        uint32_t length = 0;
        file.read(reinterpret_cast<char *>(&length), sizeof(length));

        if (length == 0)
        {
            str.clear();
            return;
        }

        str.resize(length);
        file.read(str.data(), length);
    }

    SceneDescription SceneDescription::Load(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << filename << std::endl;
            abort();
        }

        SceneDescription sceneDescription;
        sceneDescription.filename = filename;

        // Assets
        std::set<std::string> assetNames;

        uint32_t numMeshes = 0;
        file.read(reinterpret_cast<char *>(&numMeshes), sizeof(numMeshes));

        for (uint32_t i = 0; i < numMeshes; i++)
        {
            MeshAssetInfo meshInfo;
            ReadString(file, meshInfo.filename);
            sceneDescription.meshes.push_back(meshInfo);
            assetNames.insert(meshInfo.filename);
        }

        uint32_t numMaterials = 0;
        file.read(reinterpret_cast<char *>(&numMaterials), sizeof(numMaterials));

        for (uint32_t i = 0; i < numMaterials; i++)
        {
            PBRMaterialAssetInfo materialInfo;
            ReadString(file, materialInfo.name);
            ReadString(file, materialInfo.albedoMap);
            ReadString(file, materialInfo.normalMap);
            ReadString(file, materialInfo.maskMap);
            sceneDescription.materials.push_back(materialInfo);
            assetNames.insert(materialInfo.name);
        }

        // Scene data
        SceneInfoHeader sceneHeader;
        file.read(reinterpret_cast<char *>(&sceneHeader), sizeof(SceneInfoHeader));

        std::vector<SceneEntity> entities(sceneHeader.numEntities);
        for (uint32_t i = 0; i < sceneHeader.numEntities; i++)
        {
            SceneEntity &entity = entities[i];
            file.read(reinterpret_cast<char *>(&entity.position), sizeof(glm::vec3));
            file.read(reinterpret_cast<char *>(&entity.rotation), sizeof(glm::quat));
            file.read(reinterpret_cast<char *>(&entity.scale), sizeof(glm::vec3));
            file.read(reinterpret_cast<char *>(&entity.aabb), sizeof(AABB));
            file.read(reinterpret_cast<char *>(&entity.type), sizeof(uint32_t));

            ReadString(file, entity.meshName);
            ReadString(file, entity.materialName);

            if (entity.type != 1)
            {
                assert((assetNames.find(entity.meshName) != assetNames.end()) && "Mesh asset not found");
                assert((assetNames.find(entity.materialName) != assetNames.end()) && "Material asset not found");
            }

            sceneDescription.entities.push_back(entity);
        }

        // Check if there is a file called "filename.nav" and load it, it contains the navigation adjacency

        // Read the navigation points from the scene file
        uint32_t numSpawnPoints = 0;
        if (file.peek() != EOF)
        {
            file.read(reinterpret_cast<char *>(&numSpawnPoints), sizeof(numSpawnPoints));
            sceneDescription.navigationPoints.resize(numSpawnPoints);
            file.read(reinterpret_cast<char *>(sceneDescription.navigationPoints.data()), numSpawnPoints * sizeof(glm::vec3));
        }
        else
        {
            sceneDescription.navigationPoints = {
                glm::vec3(0.0f, 0.0f, 0.0f),
            };
        }

        if (file.peek() != EOF)
        {
            uint32_t numAdjacency = 0;
            file.read(reinterpret_cast<char *>(&numAdjacency), sizeof(numAdjacency));
            sceneDescription.navigationAdjacency.resize(numAdjacency);
            for (uint32_t i = 0; i < numAdjacency; i++)
            {
                uint32_t numPoints = 0;
                file.read(reinterpret_cast<char *>(&numPoints), sizeof(numPoints));
                sceneDescription.navigationAdjacency[i].resize(numPoints);
                file.read(reinterpret_cast<char *>(sceneDescription.navigationAdjacency[i].data()), numPoints * sizeof(uint32_t));
            }
        }
        else
        {
            sceneDescription.navigationAdjacency = {
                {},
            };
        }

        // We overwrite the navigation data if there is a file called "filename.nav"
        std::string navFilename = filename + ".nav";
        if (std::filesystem::exists(navFilename))
        {
            std::cout << "Loading navigation data from: " << navFilename << std::endl;
            std::ifstream navFile(navFilename, std::ios::binary);
            uint32_t numPoints = 0;
            navFile.read(reinterpret_cast<char *>(&numPoints), sizeof(numPoints));
            sceneDescription.navigationPoints.resize(numPoints);
            navFile.read(reinterpret_cast<char *>(sceneDescription.navigationPoints.data()), numPoints * sizeof(glm::vec3));

            uint32_t numAdjacency = 0;
            navFile.read(reinterpret_cast<char *>(&numAdjacency), sizeof(numAdjacency));
            sceneDescription.navigationAdjacency.resize(numAdjacency);
            for (uint32_t i = 0; i < numAdjacency; i++)
            {
                uint32_t numPoints = 0;
                navFile.read(reinterpret_cast<char *>(&numPoints), sizeof(numPoints));
                sceneDescription.navigationAdjacency[i].resize(numPoints);
                navFile.read(reinterpret_cast<char *>(sceneDescription.navigationAdjacency[i].data()), numPoints * sizeof(uint32_t));
            }
        }

        if (file.peek() != EOF)
        {
            file.read(reinterpret_cast<char *>(&sceneDescription.irradianceVolumeData), sizeof(sceneDescription.irradianceVolumeData));
        }
        else
        {
            sceneDescription.irradianceVolumeData = {
                .min = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                .max = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                .numCells = glm::uvec4(1, 1, 1, 0),
            };
        }

        if (file.peek() != EOF)
        {
            ReadString(file, sceneDescription.irradianceData);
        }
        else
        {
            sceneDescription.irradianceData = "";
        }

        if (file.peek() != EOF)
        {
            uint32_t numProbePositions = 0;
            file.read(reinterpret_cast<char *>(&numProbePositions), sizeof(numProbePositions));
            sceneDescription.probePositions.resize(numProbePositions);
            file.read(reinterpret_cast<char *>(sceneDescription.probePositions.data()), numProbePositions * sizeof(glm::vec3));

            for (uint32_t i = 0; i < numProbePositions; i++)
            {
                std::cout << "vec3(" << sceneDescription.probePositions[i].x << ", " << sceneDescription.probePositions[i].y << ", " << sceneDescription.probePositions[i].z << ")," << std::endl;
            }
        }
        else
        {
            sceneDescription.probePositions = {
                glm::vec3(0.0f, 0.0f, 0.0f),
            };
        }

        if (file.peek() != EOF)
        {
            ReadString(file, sceneDescription.prefilteredData);
        }
        else
        {
            sceneDescription.prefilteredData = "";
        }

        if (file.peek() != EOF)
        {
            ReadString(file, sceneDescription.skybox);
        }
        else
        {
            sceneDescription.skybox = "";
        }

        return sceneDescription;
    }

}