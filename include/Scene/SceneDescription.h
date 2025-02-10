#pragma once

#include "Vultron/Types.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <fstream>
#include <cstdint>

using namespace Vultron;

namespace mk
{
    struct SceneInfoHeader
    {
        uint32_t numEntities;
    };

    struct AABB
    {
        glm::vec3 min;
        glm::vec3 max;
    };

    struct SceneEntity
    {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        AABB aabb;
        uint32_t type;

        std::string meshName;
        std::string materialName;
    };

    enum class AssetType : uint32_t
    {
        Mesh,
        PBRMaterial,

        // Keep last
        Count,
        None,
    };

    struct AssetInfoHeader
    {
        AssetType type;
    };

    struct MeshAssetInfo
    {
        std::string filename;
    };

    struct PBRMaterialAssetInfo
    {
        std::string name;
        std::string albedoMap;
        std::string normalMap;
        std::string maskMap;
    };

    struct SceneDescription
    {
        std::string filename;
        std::vector<MeshAssetInfo> meshes;
        std::vector<PBRMaterialAssetInfo> materials;
        std::vector<SceneEntity> entities;
        std::vector<glm::vec3> navigationPoints;
        std::vector<std::vector<uint32_t>> navigationAdjacency;
        VolumeData irradianceVolumeData;
        std::string irradianceData;
        std::vector<glm::vec3> probePositions;
        std::string prefilteredData;
        std::string skybox;

        static SceneDescription Load(const std::string &filename);
    };

}
