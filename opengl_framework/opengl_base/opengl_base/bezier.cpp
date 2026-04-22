#include "bezier.h"

glm::vec3 Bezier::CalculatePoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1.0f - t;
    return u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3;
}

std::vector<glm::vec3> Bezier::GenerateCurveForwardDifferencing(int steps, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    std::vector<glm::vec3> vertices;
    vertices.reserve(steps + 1);

    glm::vec3 a = -p0 + 3.0f * p1 - 3.0f * p2 + p3;
    glm::vec3 b = 3.0f * p0 - 6.0f * p1 + 3.0f * p2;
    glm::vec3 c = -3.0f * p0 + 3.0f * p1;
    glm::vec3 d = p0;

    float delta = 1.0f / (float)steps;
    float delta2 = delta * delta;
    float delta3 = delta * delta * delta;

    glm::vec3 f = d;                                  
    glm::vec3 df = a * delta3 + b * delta2 + c * delta;
    glm::vec3 ddf = 6.0f * a * delta3 + 2.0f * b * delta2; 
    glm::vec3 dddf = 6.0f * a * delta3;           

    vertices.push_back(f);
    for (int i = 0; i < steps; i++) {
        f += df;
        df += ddf;
        ddf += dddf;
        vertices.push_back(f);
    }

    return vertices;
}

glm::vec3 Bezier::CalculateSurfacePoint(float u, float v, const std::vector<glm::vec3>& controlPoints) {
    glm::vec3 p0 = CalculatePoint(u, controlPoints[0], controlPoints[1], controlPoints[2], controlPoints[3]);
    glm::vec3 p1 = CalculatePoint(u, controlPoints[4], controlPoints[5], controlPoints[6], controlPoints[7]);
    glm::vec3 p2 = CalculatePoint(u, controlPoints[8], controlPoints[9], controlPoints[10], controlPoints[11]);
    glm::vec3 p3 = CalculatePoint(u, controlPoints[12], controlPoints[13], controlPoints[14], controlPoints[15]);

    return CalculatePoint(v, p0, p1, p2, p3);
}

std::vector<float> Bezier::GenerateSurfaceMesh(int uSteps, int vSteps, const std::vector<glm::vec3>& controlPoints) {
    std::vector<float> vertices;

    for (int i = 0; i < uSteps; i++) {
        for (int j = 0; j < vSteps; j++) {

            float u1 = (float)i / uSteps;
            float v1 = (float)j / vSteps;
            float u2 = (float)(i + 1) / uSteps;
            float v2 = (float)(j + 1) / vSteps;

            glm::vec3 p00 = CalculateSurfacePoint(u1, v1, controlPoints);
            glm::vec3 p10 = CalculateSurfacePoint(u2, v1, controlPoints);
            glm::vec3 p01 = CalculateSurfacePoint(u1, v2, controlPoints);
            glm::vec3 p11 = CalculateSurfacePoint(u2, v2, controlPoints);

            vertices.insert(vertices.end(), { p00.x, p00.y, p00.z,  u1, v1 });
            vertices.insert(vertices.end(), { p10.x, p10.y, p10.z,  u2, v1 });
            vertices.insert(vertices.end(), { p01.x, p01.y, p01.z,  u1, v2 });

            vertices.insert(vertices.end(), { p10.x, p10.y, p10.z,  u2, v1 });
            vertices.insert(vertices.end(), { p11.x, p11.y, p11.z,  u2, v2 });
            vertices.insert(vertices.end(), { p01.x, p01.y, p01.z,  u1, v2 });
        }
    }
    return vertices;
}

glm::vec3 CalculateDerivative(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1.0f - t;
    return 3.0f * u * u * (p1 - p0) + 6.0f * u * t * (p2 - p1) + 3.0f * t * t * (p3 - p2);
}

std::vector<float> Bezier::GenerateTrackMesh(int steps, float width, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    std::vector<float> vertices;

    std::vector<glm::vec3> curvePoints = GenerateCurveForwardDifferencing(steps, p0, p1, p2, p3);

    glm::vec3 up(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < steps; i++) {
        glm::vec3 pos1 = curvePoints[i];
        glm::vec3 pos2 = curvePoints[i + 1];

        glm::vec3 dir1 = glm::normalize(pos2 - pos1);

        glm::vec3 dir2 = dir1;
        if (i + 2 <= steps) {
            dir2 = glm::normalize(curvePoints[i + 2] - pos2);
        }

        glm::vec3 right1 = glm::normalize(glm::cross(dir1, up)) * (width * 0.5f);
        glm::vec3 right2 = glm::normalize(glm::cross(dir2, up)) * (width * 0.5f);

        glm::vec3 p1Left = pos1 - right1;
        glm::vec3 p1Right = pos1 + right1;
        glm::vec3 p2Left = pos2 - right2;
        glm::vec3 p2Right = pos2 + right2;

        float v1 = (float)i;
        float v2 = (float)(i + 1);

        vertices.insert(vertices.end(), { p1Left.x, p1Left.y, p1Left.z, 0.0f, v1 });
        vertices.insert(vertices.end(), { p1Right.x, p1Right.y, p1Right.z, 1.0f, v1 });
        vertices.insert(vertices.end(), { p2Left.x, p2Left.y, p2Left.z, 0.0f, v2 });

        vertices.insert(vertices.end(), { p1Right.x, p1Right.y, p1Right.z, 1.0f, v1 });
        vertices.insert(vertices.end(), { p2Right.x, p2Right.y, p2Right.z, 1.0f, v2 });
        vertices.insert(vertices.end(), { p2Left.x, p2Left.y, p2Left.z, 0.0f, v2 });
    }
    return vertices;
}


std::vector<Bezier::LookupEntry> Bezier::GenerateDistanceLookupTable(
    int samples,
    glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3
) {
    std::vector<Bezier::LookupEntry> lookupTable;

    if (samples < 1) {
        samples = 1;
    }

    lookupTable.reserve(samples + 1);

    glm::vec3 previousPoint = CalculatePoint(0.0f, p0, p1, p2, p3);
    float totalDistance = 0.0f;

    lookupTable.push_back({ 0.0f, 0.0f });

    for (int i = 1; i <= samples; i++) {
        float t = (float)i / (float)samples;
        glm::vec3 currentPoint = CalculatePoint(t, p0, p1, p2, p3);

        totalDistance += glm::length(currentPoint - previousPoint);
        lookupTable.push_back({ t, totalDistance });

        previousPoint = currentPoint;
    }

    return lookupTable;
}

float Bezier::GetTimeAtSpecificDistance(
    float distance,
    const std::vector<Bezier::LookupEntry>& lookupTable
) {
    if (lookupTable.empty()) {
        return 0.0f;
    }

    if (distance <= lookupTable.front().distance) {
        return lookupTable.front().t;
    }

    if (distance >= lookupTable.back().distance) {
        return lookupTable.back().t;
    }

    for (size_t i = 0; i < lookupTable.size() - 1; i++) {
        const Bezier::LookupEntry& a = lookupTable[i];
        const Bezier::LookupEntry& b = lookupTable[i + 1];

        if (distance >= a.distance && distance <= b.distance) {
            float localFactor = (distance - a.distance) / (b.distance - a.distance);
            return a.t + localFactor * (b.t - a.t);
        }
    }

    return lookupTable.back().t;
}


glm::vec3 Bezier::CalculateLookingDirection(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1.0f - t;

    glm::vec3 tangent = 3.0f * u * u * (p1 - p0) + 6.0f * u * t * (p2 - p1) + 3.0f * t * t * (p3 - p2); // tangent = raaklijn

    if (glm::length(tangent) < 0.0001f) {  // voor delen met 0 te vermijden
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }

    return glm::normalize(tangent);
}