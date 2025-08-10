#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
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

// Global camera variables
float cameraDistance = 1000.0f;
float cameraAngleX = 30.0f;
float cameraAngleY = 0.0f;
float cameraSpeed = 2.0f;

class Object
{
private:
    std::vector<float> position = {0, 0, 0};  // 3D position
    std::vector<float> velocity = {0, 0, 0};  // 3D velocity

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
        position[2] += velocity[2] * timestep;
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

    void DrawSphere(float radius, int slices, int stacks) const
    {
        glPushMatrix();
        glTranslatef(position[0], position[1], position[2]);
        glColor4f(hue[0], hue[1], hue[2], hue[3]);
        
        // Draw sphere using triangles
        for (int i = 0; i < stacks; ++i)
        {
            float lat1 = M_PI * (-0.5f + (float)i / stacks);
            float lat2 = M_PI * (-0.5f + (float)(i + 1) / stacks);
            
            glBegin(GL_TRIANGLE_STRIP);
            for (int j = 0; j <= slices; ++j)
            {
                float lng = 2 * M_PI * (float)j / slices;
                
                float x1 = cosf(lat1) * cosf(lng);
                float y1 = sinf(lat1);
                float z1 = cosf(lat1) * sinf(lng);
                
                float x2 = cosf(lat2) * cosf(lng);
                float y2 = sinf(lat2);
                float z2 = cosf(lat2) * sinf(lng);
                
                glVertex3f(x1 * radius, y1 * radius, z1 * radius);
                glVertex3f(x2 * radius, y2 * radius, z2 * radius);
            }
            glEnd();
        }
        
        glPopMatrix();
    }

    void Draw() const
    {
        float radius = GetRadius();
        DrawSphere(radius, 20, 16);
    }

    void accelerate(double ax, double ay, double az, double timestep)
    {
        velocity[0] += (float)(ax * timestep);
        velocity[1] += (float)(ay * timestep);
        velocity[2] += (float)(az * timestep);
    }

    // Overloaded for backward compatibility with 2D acceleration
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

// Keyboard callback for camera controls
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
            case GLFW_KEY_W: cameraAngleX += cameraSpeed; break;
            case GLFW_KEY_S: cameraAngleX -= cameraSpeed; break;
            case GLFW_KEY_A: cameraAngleY -= cameraSpeed; break;
            case GLFW_KEY_D: cameraAngleY += cameraSpeed; break;
            case GLFW_KEY_Q: cameraDistance -= 50.0f; break;
            case GLFW_KEY_E: cameraDistance += 50.0f; break;
        }
        
        // Clamp camera distance
        if (cameraDistance < 100.0f) cameraDistance = 100.0f;
        if (cameraDistance > 3000.0f) cameraDistance = 3000.0f;
    }
}

int main(void)
{
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "3D Solar System Simulation", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // Enable back face culling for better performance
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Set up lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    float lightPos[] = {0.0f, 0.0f, 0.0f, 1.0f}; // Light at sun position
    float lightColor[] = {1.0f, 1.0f, 0.8f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    
    // Enable color material
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    const double G = 6.67430e-11;         // gravitational constant
    const double TIME_STEP = 3600.0 * 24 * 365.24 / 12; // 1 average month per timestep (seconds)

    double sunMass = 1.989e30;

    // Fixed sun position at center of 3D space
    const float sunX = 0.0f;
    const float sunY = 0.0f;
    const float sunZ = 0.0f;

    // Create Sun fixed at center
    Object sun({sunX, sunY, sunZ}, {0.0f, 0.0f, 0.0f}, sunMass, {1.0f, 1.0f, 0.0f, 1.0f});
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

    int planetIndex = 0;
    for (const auto &p : planetInfos)
    {
        double distanceMeters = p.distanceKm * 1000.0;
        float distancePixels = (float)(distanceMeters / DISTANCE_SCALE);

        Object tempPlanet({0, 0, 0}, {0, 0, 0}, p.mass, p.color);
        float planetRadiusPixels = tempPlanet.GetRadius();

        float minOrbitRadius = lastOrbitRadius + planetRadiusPixels + 10.0f;
        if (distancePixels < minOrbitRadius)
            distancePixels = minOrbitRadius;

        lastOrbitRadius = distancePixels;

        double distanceForVelocityMeters = distancePixels * DISTANCE_SCALE;
        double velMag = orbitalVelocity(G, sunMass, distanceForVelocityMeters);
        double velPixels = velMag / DISTANCE_SCALE;

        // Add slight orbital inclinations for visual interest (in radians)
        float inclination = (planetIndex * 5.0f) * M_PI / 180.0f; // 0-35 degrees
        float startAngle = planetIndex * 45.0f * M_PI / 180.0f;   // Spread planets around
        
        // Calculate 3D position
        float posX = distancePixels * cosf(startAngle) * cosf(inclination);
        float posY = distancePixels * sinf(startAngle) * cosf(inclination);
        float posZ = distancePixels * sinf(inclination);

        // Calculate 3D velocity (perpendicular to position vector)
        float velX = -posY * (float)velPixels / distancePixels;
        float velY = posX * (float)velPixels / distancePixels;
        float velZ = 0.0f; // Keep orbital motion mostly in XY plane

        planets.push_back(Object({posX, posY, posZ}, {velX, velY, velZ}, p.mass, p.color));
        planetIndex++;
    }

    std::printf("\n3D Solar System Controls:\n");
    std::printf("WASD: Rotate camera\n");
    std::printf("Q/E: Zoom in/out\n\n");
    
    while (!glfwWindowShouldClose(window))
    {
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        glViewport(0, 0, windowWidth, windowHeight);

        // Set up 3D perspective projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)windowWidth / windowHeight, 1.0, 10000.0);

        // Set up 3D camera with modelview matrix
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Calculate camera position based on spherical coordinates
        float cameraX = cameraDistance * sinf(cameraAngleX * M_PI / 180.0f) * cosf(cameraAngleY * M_PI / 180.0f);
        float cameraY = cameraDistance * cosf(cameraAngleX * M_PI / 180.0f);
        float cameraZ = cameraDistance * sinf(cameraAngleX * M_PI / 180.0f) * sinf(cameraAngleY * M_PI / 180.0f);
        
        gluLookAt(cameraX, cameraY, cameraZ,  // Camera position
                  0, 0, 0,                    // Look at origin (sun)
                  0, 1, 0);                   // Up vector

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Dark space color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update light position (at sun)
        float lightPos[] = {sunX, sunY, sunZ, 1.0f};
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        // Draw Sun fixed at center
        sun.Draw();

        // Update gravitational acceleration & position of planets (3D)
        for (auto &planet : planets)
        {
            auto pos = planet.GetCoord();

            float dx = sunX - pos[0];
            float dy = sunY - pos[1];
            float dz = sunZ - pos[2];
            float dist_pixels = sqrt(dx * dx + dy * dy + dz * dz);

            if (dist_pixels < 1.0f)
                continue;

            double dist_meters = dist_pixels * DISTANCE_SCALE;

            // Calculate acceleration magnitude (m/s²)
            double acc = G * sunMass / (dist_meters * dist_meters);
            // Convert acceleration to pixels per second²
            float acc_pixels = (float)(acc / DISTANCE_SCALE);

            // Normalize direction vector for 3D
            float dir_x = dx / dist_pixels;
            float dir_y = dy / dist_pixels;
            float dir_z = dz / dist_pixels;

            float ax = dir_x * acc_pixels;
            float ay = dir_y * acc_pixels;
            float az = dir_z * acc_pixels;

            planet.accelerate(ax, ay, az, TIME_STEP);
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
