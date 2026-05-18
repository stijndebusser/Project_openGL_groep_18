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