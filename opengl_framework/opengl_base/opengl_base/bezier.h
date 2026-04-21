#pragma once
#include <vector>
#include <glm/glm.hpp>

class Bezier {
public:
    static glm::vec3 CalculatePoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

    static std::vector<glm::vec3> GenerateCurveForwardDifferencing(int steps, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

    static glm::vec3 CalculateSurfacePoint(float u, float v, const std::vector<glm::vec3>& controlPoints);

    static std::vector<float> GenerateSurfaceMesh(int uSteps, int vSteps, const std::vector<glm::vec3>& controlPoints);

    static std::vector<float> GenerateTrackMesh(int steps, float width, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
};