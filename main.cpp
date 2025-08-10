#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <stdio.h>
#include <cmath>
#include <vector>
#include <iostream>

const int WINDOW_WIDTH = 800 * 1.5;
const int WINDOW_HEIGHT = 600 * 1.5;

// Use a scaled distance system:
// Scale all planet distances so Neptune fits around 90% of window width.
const double REAL_NEPTUNE_DISTANCE_M = 4.495e12;            // meters (4.495 billion km)
const float MAX_ORBIT_RADIUS_PIXELS = WINDOW_WIDTH * 0.45f; // max radius for Neptune orbit in pixels

// Calculate a scale factor so Neptune’s orbit fits on screen
// Removed *400 for realistic scaling
const double DISTANCE_SCALE = REAL_NEPTUNE_DISTANCE_M / MAX_ORBIT_RADIUS_PIXELS; // meters per pixel

class Object
{
private:
    std::vector<float> position = {400, 300};
    std::vector<float> velocity = {0, 0};

public:
    std::vector<float> hue = {1.0f, 1.0f, 1.0f, 1.0f};
    double mass;
    double density = 1400.0; // kg/m^3 approx for planets

    Object(std::vector<float> initPosition, std::vector<float> initVelocity, double mass, std::vector<float> color)
        : position(initPosition), velocity(initVelocity), mass(mass), hue(color) {}

    void UpdatePos(double timestep)
    {
        position[0] += velocity[0] * timestep;
        position[1] += velocity[1] * timestep;
    }

    std::vector<float> GetCoord() const
    {
        return position;
    }

    float GetRadius() const
    {
        double volume = mass / density;
        double radiusMeters = pow((3.0 * volume) / (4.0 * M_PI), 1.0 / 3.0);

        const double SUN_SCALE_FACTOR = 5e6;
        const double PLANET_SCALE_FACTOR = 1e6;

        double radiusPixels;
        if (mass > 1e29)
        {
            radiusPixels = radiusMeters / SUN_SCALE_FACTOR;
            const double MAX_SUN_RADIUS = 250.0;
            if (radiusPixels > MAX_SUN_RADIUS)
                radiusPixels = MAX_SUN_RADIUS;
        }
        else
        {
            radiusPixels = radiusMeters / PLANET_SCALE_FACTOR;
            const double MIN_PLANET_RADIUS = 6.0;
            if (radiusPixels < MIN_PLANET_RADIUS)
                radiusPixels = MIN_PLANET_RADIUS;
        }

        return (float)radiusPixels;
    }

    void Draw() const
    {
        float radius = GetRadius();
        float x = position[0];
        float y = position[1];
        glColor4f(hue[0], hue[1], hue[2], hue[3]);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);
        int numSegments = 40;
        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = 2.0f * 3.14159265359f * i / numSegments;
            float dx = radius * cosf(angle);
            float dy = radius * sinf(angle);
            glVertex2f(x + dx, y + dy);
        }
        glEnd();
    }

    void accelerate(double ax, double ay, double timestep)
    {
        velocity[0] += (float)(ax * timestep);
        velocity[1] += (float)(ay * timestep);
    }
};

double orbitalVelocity(double G, double centralMass, double distanceMeters)
{
    return sqrt(G * centralMass / distanceMeters);
}

int main(void)
{
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Solar System Simulation", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);

    const double G = 6.67430e-11;         // gravitational constant
    const double TIME_STEP = 3600.0 * 24 * 365.24 / 12; // 1 average month per timestep (seconds)

    double sunMass = 1.989e30;

    // Fixed sun position at center of window
    const float sunX = WINDOW_WIDTH / 2.0f;
    const float sunY = WINDOW_HEIGHT / 2.0f;

    // Create Sun fixed at center
    Object sun({sunX, sunY}, {0.0f, 0.0f}, sunMass, {1.0f, 1.0f, 0.0f, 1.0f});
    float sunRadius = sun.GetRadius();

    struct PlanetInfo
    {
        double distanceKm;
        double mass;
        std::vector<float> color;
    };

    std::vector<PlanetInfo> planetInfos = {
        {57.9e6, 3.285e23, {0.6f, 0.6f, 0.6f, 1.0f}},   // Mercury
        {108.2e6, 4.867e24, {1.0f, 0.5f, 0.0f, 1.0f}},  // Venus
        {149.6e6, 5.972e24, {0.0f, 0.5f, 1.0f, 1.0f}},  // Earth
        {227.9e6, 6.39e23, {1.0f, 0.2f, 0.2f, 1.0f}},   // Mars
        {778.5e6, 1.898e27, {1.0f, 0.7f, 0.4f, 1.0f}},  // Jupiter
        {1.433e9, 5.683e26, {1.0f, 1.0f, 0.7f, 1.0f}},  // Saturn
        {2.8725e9, 8.681e25, {0.5f, 1.0f, 1.0f, 1.0f}}, // Uranus
        {4.495e9, 1.024e26, {0.2f, 0.4f, 1.0f, 1.0f}}   // Neptune
    };

    // Sort planets ascending by distance (optional)
    std::sort(planetInfos.begin(), planetInfos.end(),
              [](const PlanetInfo &a, const PlanetInfo &b)
              {
                  return a.distanceKm < b.distanceKm;
              });

    float lastOrbitRadius = sunRadius + 20.0f;
    std::vector<Object> planets;

    for (const auto &p : planetInfos)
    {
        double distanceMeters = p.distanceKm * 1000.0;
        float distancePixels = (float)(distanceMeters / DISTANCE_SCALE);

        Object tempPlanet({0, 0}, {0, 0}, p.mass, p.color);
        float planetRadiusPixels = tempPlanet.GetRadius();

        float minOrbitRadius = lastOrbitRadius + planetRadiusPixels + 10.0f;
        if (distancePixels < minOrbitRadius)
            distancePixels = minOrbitRadius;

        lastOrbitRadius = distancePixels;

        double distanceForVelocityMeters = distancePixels * DISTANCE_SCALE;
        double velMag = orbitalVelocity(G, sunMass, distanceForVelocityMeters);
        // No artificial velocity scale here for realism:
        double velPixels = velMag / DISTANCE_SCALE;

        float posX = sunX + distancePixels;
        float posY = sunY;

        // velocity perpendicular to radius vector (circular orbit)
        float rx = posX - sunX;
        float ry = posY - sunY;
        float rLen = sqrt(rx * rx + ry * ry);
        rx /= rLen;
        ry /= rLen;

        float velX = -ry * (float)velPixels;
        float velY = rx * (float)velPixels;

        planets.push_back(Object({posX, posY}, {velX, velY}, p.mass, p.color));
    }

    while (!glfwWindowShouldClose(window))
    {
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        glViewport(0, 0, windowWidth, windowHeight);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowWidth, windowHeight, 0, -1, 1);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw Sun fixed at center
        sun.Draw();

        // Update gravitational acceleration & position of planets
        for (auto &planet : planets)
        {
            auto pos = planet.GetCoord();

            float dx = sunX - pos[0];
            float dy = sunY - pos[1];
            float dist_pixels = sqrt(dx * dx + dy * dy);

            if (dist_pixels < 1.0f)
                continue;

            double dist_meters = dist_pixels * DISTANCE_SCALE;

            // Calculate acceleration magnitude (m/s²)
            double acc = G * sunMass / (dist_meters * dist_meters);
            // Convert acceleration to pixels per second²
            float acc_pixels = (float)(acc / DISTANCE_SCALE);

            float dir_x = dx / dist_pixels;
            float dir_y = dy / dist_pixels;

            float ax = dir_x * acc_pixels;
            float ay = dir_y * acc_pixels;

            planet.accelerate(ax, ay, TIME_STEP);
        }

        for (auto &planet : planets)
        {
            planet.UpdatePos(TIME_STEP);
            planet.Draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
