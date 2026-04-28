#pragma once
#include <vector>
#include <glm/glm.hpp>

class Bezier {
public:
    struct LookupEntry {
        float t;
        float distance;
    };

    static glm::vec3 CalculatePoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

    static glm::vec3 CalculateLookingDirection(
        float t,
        glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3
    );

    static std::vector<glm::vec3> GenerateCurveForwardDifferencing(int steps, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

    static glm::vec3 CalculateSurfacePoint(float u, float v, const std::vector<glm::vec3>& controlPoints);

    static std::vector<float> GenerateSurfaceMesh(int uSteps, int vSteps, const std::vector<glm::vec3>& controlPoints);

    static std::vector<float> GenerateTrackMesh(int steps, float width, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

   static std::vector<LookupEntry> GenerateDistanceLookupTable(
        int samples,
        glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3
    );


    static float GetTimeAtSpecificDistance(
        float distance,
        const std::vector<LookupEntry>& lookupTable
    );

    // lookuptable: 100 wmaples -> key tijd, value distance, kan tijd hebben die ertussen valt en interpoleren -> kan tijd meegeven om te zien hoe ver op track zit

    // raaklein zie slides begeir


};