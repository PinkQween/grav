#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <stdio.h>
#include <cmath>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // for radians()

const int WINDOW_WIDTH = 800 * 1.5;
const int WINDOW_HEIGHT = 600 * 1.5;

// Use a scaled distance system:
// Scale all planet distances so Neptune fits around 90% of window width.
const double REAL_NEPTUNE_DISTANCE_M = 4.495e12;            // meters (4.495 billion km)
const float MAX_ORBIT_RADIUS_PIXELS = WINDOW_WIDTH * 0.45f; // max radius for Neptune orbit in pixels

// Calculate a scale factor so Neptune’s orbit fits on screen
const double DISTANCE_SCALE = REAL_NEPTUNE_DISTANCE_M / MAX_ORBIT_RADIUS_PIXELS; // meters per pixel

// Global camera variables
float cameraDistance = 1000.0f;
float cameraAngleX = 115.0f; // pitch (degrees) - top/down
float cameraAngleY = 90.0f;  // yaw (degrees) - orbit around origin
float cameraAngleZ = 0.0f;   // roll (degrees) - rotate about view axis
float cameraSpeed = 2.0f;

// Space-time grid settings
bool showGrid = true;
bool grid3D = false;                                       // false = 2D grid, true = 3D grid
const int GRID_SIZE = 100;                                  // Grid resolution
const int GRID_SIZE_3D = 20;                               // Reduced grid resolution for 3D
const float GRID_SPACING = 50.0f;                          // Distance between grid points
const float GRID_SPACING_3D = 20.0f;                       // Distance between grid points for 3D
const float GRID_EXTENT = GRID_SIZE * GRID_SPACING / 2.0f; // Half the grid size

// Celestial object types
enum CelestialType
{
    PLANET,
    STAR,
    BLACK_HOLE
};

// --- helpers for vector math (small, inline) ---
static inline void vec3_normalize(float v[3])
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-6f)
    {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}
static inline void vec3_cross(const float a[3], const float b[3], float out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}
static inline float vec3_dot(const float a[3], const float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Rotate the "up" vector (0,1,0) around the camera forward axis by rollRadians using Rodrigues' formula
static inline void computeRolledUpVector(const float forward[3], float rollRadians, float outUp[3])
{
    // original up vector
    const float up0[3] = {0.0f, 1.0f, 0.0f};
    float k[3] = {forward[0], forward[1], forward[2]};
    vec3_normalize(k);

    float cosr = cosf(rollRadians);
    float sinr = sinf(rollRadians);

    // term1 = up0 * cosr
    float term1[3] = {up0[0] * cosr, up0[1] * cosr, up0[2] * cosr};

    // term2 = (k x up0) * sinr
    float kxup[3];
    vec3_cross(k, up0, kxup);
    float term2[3] = {kxup[0] * sinr, kxup[1] * sinr, kxup[2] * sinr};

    // term3 = k * (k·up0) * (1 - cosr)
    float kdotup = vec3_dot(k, up0);
    float coef = (1.0f - cosr) * kdotup;
    float term3[3] = {k[0] * coef, k[1] * coef, k[2] * coef};

    outUp[0] = term1[0] + term2[0] + term3[0];
    outUp[1] = term1[1] + term2[1] + term3[1];
    outUp[2] = term1[2] + term2[2] + term3[2];

    // normalize just in case
    vec3_normalize(outUp);
}

// Advanced lighting calculation with much brighter lighting
float calculateLightIntensity(const std::vector<float> &lightPos, const std::vector<float> &objectPos,
                              const std::vector<std::vector<float>> &blackHoles)
{
    float dx = lightPos[0] - objectPos[0];
    float dy = lightPos[1] - objectPos[1];
    float dz = lightPos[2] - objectPos[2];
    float distanceToLight = sqrt(dx * dx + dy * dy + dz * dz);

    if (distanceToLight < 1.0f)
        return 1.0f; // At light source

    // Much brighter base intensity with slower falloff
    float baseIntensity = 0.8f + (200000.0f / (distanceToLight + 100.0f));
    baseIntensity = std::min(1.0f, baseIntensity); // Cap at 1.0

    // Check light absorption by black holes (only strong shadows very close to black holes)
    for (const auto &blackHole : blackHoles)
    {
        float bhx = blackHole[0] - objectPos[0];
        float bhy = blackHole[1] - objectPos[1];
        float bhz = blackHole[2] - objectPos[2];
        float distanceToBlackHole = sqrt(bhx * bhx + bhy * bhy + bhz * bhz);

        // Event horizon radius (simplified)
        float eventHorizonRadius = blackHole[3]; // Stored as 4th element

        // Only cast shadows if very close to black hole
        if (distanceToBlackHole < eventHorizonRadius * 2.0f)
        {
            // Check if black hole is between light and object
            float lightToBH_x = blackHole[0] - lightPos[0];
            float lightToBH_y = blackHole[1] - lightPos[1];
            float lightToBH_z = blackHole[2] - lightPos[2];
            float lightToObj_x = objectPos[0] - lightPos[0];
            float lightToObj_y = objectPos[1] - lightPos[1];
            float lightToObj_z = objectPos[2] - lightPos[2];

            float dotProduct = lightToBH_x * lightToObj_x + lightToBH_y * lightToObj_y + lightToBH_z * lightToObj_z;
            float lightToObj_mag = sqrt(lightToObj_x * lightToObj_x + lightToObj_y * lightToObj_y + lightToObj_z * lightToObj_z);
            float lightToBH_mag = sqrt(lightToBH_x * lightToBH_x + lightToBH_y * lightToBH_y + lightToBH_z * lightToBH_z);

            if (dotProduct > 0 && lightToBH_mag < lightToObj_mag)
            {
                // Black hole is between light and object - reduce shadow effect
                float shadowStrength = 1.0f - (distanceToBlackHole / (eventHorizonRadius * 2.0f));
                baseIntensity *= (1.0f - shadowStrength * 0.4f); // Reduced shadow strength
            }
        }
    }

    return std::max(0.4f, baseIntensity); // Much higher minimum ambient light
}

class CelestialObject
{
private:
    std::vector<float> position = {0, 0, 0}; // 3D position (pixels)
    std::vector<float> velocity = {0, 0, 0}; // 3D velocity (pixels per second)

public:
    std::vector<float> hue = {1.0f, 1.0f, 1.0f, 1.0f};
    double mass;             // kg
    double density = 1400.0; // kg/m^3 approx for planets
    CelestialType type;

    CelestialObject(std::vector<float> initPosition, std::vector<float> initVelocity, double mass,
                    std::vector<float> color, CelestialType objectType = PLANET)
        : position(initPosition), velocity(initVelocity), mass(mass), hue(color), type(objectType) {}

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

    std::vector<float> GetVelocity() const
    {
        return velocity;
    }

    void SetVelocity(const std::vector<float> &v)
    {
        velocity = v;
    }

    float GetRadius() const
    {
        if (type == BLACK_HOLE)
        {
            // Schwarzschild radius for black hole event horizon
            const double c = 299792458.0; // speed of light
            const double G = 6.67430e-11;
            double schwarzschildRadius = (2.0 * G * mass) / (c * c);
            return (float)(schwarzschildRadius / DISTANCE_SCALE * 1000000); // Scale for visibility
        }

        double volume = mass / density;
        double radiusMeters = pow((3.0 * volume) / (4.0 * M_PI), 1.0 / 3.0);

        const double SUN_SCALE_FACTOR = 5e6;
        const double PLANET_SCALE_FACTOR = 1e6;

        double radiusPixels;
        if (mass > 1e29 || type == STAR)
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

    void DrawAccretionDisk(float innerRadius, float outerRadius) const
    {
        glDisable(GL_LIGHTING); // Disable lighting for the glowing disk
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        int segments = 64;
        int rings = 16;

        for (int ring = 0; ring < rings; ++ring)
        {
            float r1 = innerRadius + (outerRadius - innerRadius) * ring / rings;
            float r2 = innerRadius + (outerRadius - innerRadius) * (ring + 1) / rings;

            // Color gradient from hot inner (white/yellow) to cooler outer (red/orange)
            float intensity = 1.0f - (float)ring / rings;
            glColor4f(1.0f, 0.6f + 0.4f * intensity, 0.2f * intensity, 0.3f + 0.4f * intensity);

            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= segments; ++i)
            {
                float angle = 2.0f * M_PI * i / segments;
                float x1 = r1 * cosf(angle);
                float z1 = r1 * sinf(angle);
                float x2 = r2 * cosf(angle);
                float z2 = r2 * sinf(angle);

                glVertex3f(x1, 0, z1);
                glVertex3f(x2, 0, z2);
            }
            glEnd();
        }

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    void DrawSphere(float radius, int slices, int stacks, const std::vector<float> &lightPos,
                    const std::vector<std::vector<float>> &blackHoles) const
    {
        glPushMatrix();
        glTranslatef(position[0], position[1], position[2]);

        // Calculate lighting intensity based on light sources and black hole shadows
        if (type == STAR)
        {
            // Stars emit their own light - disable lighting temporarily
            glDisable(GL_LIGHTING);
            glColor4f(hue[0], hue[1], hue[2], hue[3]);
        }
        else if (type == BLACK_HOLE)
        {
            // Black holes absorb light - draw as pure black sphere
            glDisable(GL_LIGHTING);
            glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            // Planets receive lighting
            float lightIntensity = calculateLightIntensity(lightPos, position, blackHoles);
            glColor4f(hue[0] * lightIntensity, hue[1] * lightIntensity, hue[2] * lightIntensity, hue[3]);
        }

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

        // Draw accretion disk for black holes
        if (type == BLACK_HOLE)
        {
            DrawAccretionDisk(radius * 3.0f, radius * 8.0f);
        }

        if (type == STAR || type == BLACK_HOLE)
        {
            glEnable(GL_LIGHTING);
        }

        glPopMatrix();
    }

    void Draw(const std::vector<float> &lightPos, const std::vector<std::vector<float>> &blackHoles) const
    {
        float radius = GetRadius();
        DrawSphere(radius, 20, 16, lightPos, blackHoles);
    }

    // Add acceleration to the velocity (ax,ay,az are in pixels/s^2; timestep in seconds)
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

    bool IsBlackHole() const { return type == BLACK_HOLE; }
    bool IsStar() const { return type == STAR; }
};

double orbitalVelocity(double G, double centralMass, double distanceMeters)
{
    return sqrt(G * centralMass / distanceMeters);
}

// Calculate space-time curvature at a point due to all massive objects
float calculateSpaceTimeCurvature(float x, float y, float z, const CelestialObject &sun, const std::vector<CelestialObject> &objects)
{
    float totalCurvature = 0.0f;

    // Curvature from sun
    auto sunPos = sun.GetCoord();
    float dx = x - sunPos[0];
    float dy = y - sunPos[1];
    float dz = z - sunPos[2];
    float distToSun = sqrt(dx * dx + dy * dy + dz * dz);
    if (distToSun > 1.0f)
    {
        totalCurvature += sun.mass / (distToSun * distToSun) * 1e-25f;
    }

    // Curvature from other massive objects
    for (const auto &obj : objects)
    {
        auto pos = obj.GetCoord();
        float dx2 = x - pos[0];
        float dy2 = y - pos[1];
        float dz2 = z - pos[2];
        float distToObj = sqrt(dx2 * dx2 + dy2 * dy2 + dz2 * dz2);
        if (distToObj > 1.0f && obj.mass > sun.mass * 0.01)
        {
            totalCurvature += obj.mass / (distToObj * distToObj) * 1e-25f;
        }
    }

    return totalCurvature;
}

// Keyboard callback for camera controls and grid toggle
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_G:
            showGrid = !showGrid;
            std::printf("Space-time grid: %s\n", showGrid ? "ON" : "OFF");
            break;
        case GLFW_KEY_T:
            grid3D = !grid3D;
            std::printf("Grid mode: %s\n", grid3D ? "3D" : "2D");
            break;
        }
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            cameraAngleX -= cameraSpeed;
            break; // pitch up
        case GLFW_KEY_S:
            cameraAngleX += cameraSpeed;
            break; // pitch down
        case GLFW_KEY_A:
            cameraAngleZ -= cameraSpeed;
            break; // yaw left
        case GLFW_KEY_D:
            cameraAngleZ += cameraSpeed;
            break; // yaw right
        case GLFW_KEY_LEFT:
            cameraAngleY -= cameraSpeed;
            break; // roll left
        case GLFW_KEY_RIGHT:
            cameraAngleY += cameraSpeed;
            break; // roll right
        case GLFW_KEY_Q:
            cameraDistance -= 50.0f;
            break;
        case GLFW_KEY_E:
            cameraDistance += 50.0f;
            break;
        }

        // Clamp camera distance
        if (cameraDistance < 100.0f)
            cameraDistance = 100.0f;
        if (cameraDistance > 30000000.0f)
            cameraDistance = 30000000.0f;
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

    // Set up much brighter lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Much brighter light settings
    float lightPosInit[] = {0.0f, 0.0f, 0.0f, 1.0f};  // Light at sun position (will update later)
    float lightAmbient[] = {0.6f, 0.6f, 0.6f, 1.0f};  // Bright ambient light
    float lightDiffuse[] = {1.5f, 1.5f, 1.2f, 1.0f};  // Very bright diffuse light
    float lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f}; // Bright specular highlights

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosInit);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // Set global ambient light to be quite bright
    float globalAmbient[] = {0.4f, 0.4f, 0.4f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Enable color material with brighter settings
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    const double G = 6.67430e-11;                       // gravitational constant (m^3 kg^-1 s^-2)
    const double TIME_STEP = 3600.0 * 24 * 365.24;

    double sunMass = 1.989e30;

    // Create vector of celestial objects; push the Sun first so we can reference it (index 0)
    std::vector<CelestialObject> celestialObjects;
    std::vector<std::vector<float>> blackHolePositions; // Track black hole positions for lighting

    // Create Sun as a moving STAR object (initially at origin, zero velocity)
    CelestialObject sunObj({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, sunMass, {1.0f, 1.0f, 0.0f, 1.0f}, STAR);
    float sunRadius = sunObj.GetRadius();
    celestialObjects.push_back(sunObj);

    struct CelestialInfo
    {
        double distanceKm;
        double mass;
        std::vector<float> color;
        CelestialType type;
    };

    std::vector<CelestialInfo> celestialInfos = {
        {57.9e6, 3.285e23, {0.6f, 0.6f, 0.6f, 1.0f}, PLANET},   // Mercury
        {108.2e6, 4.867e24, {1.0f, 0.5f, 0.0f, 1.0f}, PLANET},  // Venus
        {149.6e6, 5.972e24, {0.0f, 0.5f, 1.0f, 1.0f}, PLANET},  // Earth
        {227.9e6, 6.39e23, {1.0f, 0.2f, 0.2f, 1.0f}, PLANET},   // Mars
        {778.5e6, 1.898e27, {1.0f, 0.7f, 0.4f, 1.0f}, PLANET},  // Jupiter
        {1.433e9, 5.683e26, {1.0f, 1.0f, 0.7f, 1.0f}, PLANET},  // Saturn
        {2.8725e9, 8.681e25, {0.5f, 1.0f, 1.0f, 1.0f}, PLANET}, // Uranus
        {4.495e9, 1.024e26, {0.2f, 0.4f, 1.0f, 1.0f}, PLANET},  // Neptune
        // Add a black hole beyond Neptune
        // {6.0e9, sunMass * 0.5, {0.0f, 0.0f, 0.0f, 1.0f}, BLACK_HOLE} // Black hole

    };

    // Sort objects ascending by distance (optional)
    std::sort(celestialInfos.begin(), celestialInfos.end(),
              [](const CelestialInfo &a, const CelestialInfo &b)
              {
                  return a.distanceKm < b.distanceKm;
              });

    float lastOrbitRadius = sunRadius + 20.0f;
    int objectIndex = 0;
    for (const auto &c : celestialInfos)
    {
        double distanceMeters = c.distanceKm * 1000.0;
        float distancePixels = (float)(distanceMeters / DISTANCE_SCALE);

        CelestialObject tempObject({0, 0, 0}, {0, 0, 0}, c.mass, c.color, c.type);
        float objectRadiusPixels = tempObject.GetRadius();

        float minOrbitRadius = lastOrbitRadius + objectRadiusPixels + 10.0f;
        if (distancePixels < minOrbitRadius)
            distancePixels = minOrbitRadius;

        lastOrbitRadius = distancePixels;

        // Use initial orbital speed around the Sun (approximation)
        double distanceForVelocityMeters = distancePixels * DISTANCE_SCALE;
        double velMag = orbitalVelocity(G, sunMass, distanceForVelocityMeters);
        double velPixels = velMag / DISTANCE_SCALE;

        // Add slight orbital inclinations for visual interest (in radians)
        float inclination = (objectIndex * 5.0f) * M_PI / 180.0f; // 0-45 degrees
        float startAngle = objectIndex * 40.0f * M_PI / 180.0f;   // Spread objects around

        // Calculate 3D position
        float posX = distancePixels * cosf(startAngle) * cosf(inclination);
        float posY = distancePixels * sinf(startAngle) * cosf(inclination);
        float posZ = distancePixels * sinf(inclination);

        // Calculate 3D velocity (perpendicular to position vector to approximate circular orbit)
        float velX = -posY * (float)velPixels / distancePixels;
        float velY = posX * (float)velPixels / distancePixels;
        float velZ = 0.0f; // Keep orbital motion mostly in XY plane

        CelestialObject newObject({posX, posY, posZ}, {velX, velY, velZ}, c.mass, c.color, c.type);
        celestialObjects.push_back(newObject);

        objectIndex++;
    }

    std::printf("\n3D Solar System Controls:\n");
    std::printf("W/S: Pitch up/down\n");
    std::printf("A/D: Roll left/right\n");
    std::printf("Left/Right arrows: Yaw left/right (optional)\n");
    std::printf("Q/E: Zoom in/out\n");
    std::printf("G: Toggle space-time grid\n");
    std::printf("T: Toggle 2D/3D grid mode\n\n");

    // MAIN LOOP
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

        // Calculate camera position based on spherical coordinates (pitch/yaw) and distance
        float pitchRad = cameraAngleX * M_PI / 180.0f;
        float yawRad = cameraAngleY * M_PI / 180.0f;
        float rollRad = cameraAngleZ * M_PI / 180.0f;

        // spherical to cartesian (radius, pitch, yaw)
        float cameraX = cameraDistance * sinf(pitchRad) * cosf(yawRad);
        float cameraY = cameraDistance * cosf(pitchRad);
        float cameraZ = cameraDistance * sinf(pitchRad) * sinf(yawRad);

        // compute forward vector (from eye to center)
        float forward[3] = {-cameraX, -cameraY, -cameraZ};
        vec3_normalize(forward);

        // compute up vector by rotating global up around forward by rollRad
        float upVec[3];
        computeRolledUpVector(forward, rollRad, upVec);

        // Determine Sun's current position (we put Sun at index 0)
        std::vector<float> sunPos = celestialObjects[0].GetCoord();

        float camX = cameraDistance * sinf(glm::radians(cameraAngleY)) * cosf(glm::radians(cameraAngleX));
        float camY = cameraDistance * sinf(glm::radians(cameraAngleX));
        float camZ = cameraDistance * cosf(glm::radians(cameraAngleY)) * cosf(glm::radians(cameraAngleX));

        // Detach camera from sun, orbit fixed origin (0,0,0)
        gluLookAt(cameraX, cameraY, cameraZ, // eye position
                  0.0f, 0.0f, 0.0f,          // look at origin
                  upVec[0], upVec[1], upVec[2]);

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Dark space color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update light position (at sun)
        float lightPos[] = {sunPos[0], sunPos[1], sunPos[2], 1.0f};
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        // Update black hole positions for dynamic lighting
        blackHolePositions.clear();
        for (const auto &obj : celestialObjects)
        {
            if (obj.IsBlackHole())
            {
                auto pos = obj.GetCoord();
                blackHolePositions.push_back({pos[0], pos[1], pos[2], obj.GetRadius()});
            }
        }

        // Draw space-time grid if enabled
        if (showGrid)
        {
            glDisable(GL_LIGHTING);
            glColor4f(0.3f, 0.6f, 0.9f, 0.4f); // Blue/cyan grid color
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            if (grid3D)
            {
                // Draw reduced 3D grid
                glBegin(GL_LINES);
                for (int i = 0; i < GRID_SIZE_3D; ++i)
                {
                    for (int j = 0; j < GRID_SIZE_3D; ++j)
                    {
                        for (int k = 0; k < GRID_SIZE_3D; ++k)
                        {
                            float x = (i - GRID_SIZE_3D / 2) * GRID_SPACING_3D * 2.0f; // Wider spacing for 3D
                            float y = (j - GRID_SIZE_3D / 2) * GRID_SPACING_3D * 2.0f;
                            float z = (k - GRID_SIZE_3D / 2) * GRID_SPACING_3D * 2.0f;

                            // Calculate space-time curvature
                            float curvature = calculateSpaceTimeCurvature(x, y, z, celestialObjects[0], celestialObjects);
                            float displacement = curvature * 50.0f;

                            // Draw lines to adjacent grid points with curvature displacement
                            // Only draw every other line to reduce clutter
                            if (i < GRID_SIZE_3D - 1 && (i + j + k) % 2 == 0)
                            {
                                float x2 = x + GRID_SPACING_3D * 2.0f;
                                float curvature2 = calculateSpaceTimeCurvature(x2, y, z, celestialObjects[0], celestialObjects);
                                float displacement2 = curvature2 * 50.0f;

                                glVertex3f(x, y - displacement, z);
                                glVertex3f(x2, y - displacement2, z);
                            }

                            if (j < GRID_SIZE_3D - 1 && (i + j + k) % 2 == 0)
                            {
                                float y2 = y + GRID_SPACING_3D * 2.0f;
                                float curvature2 = calculateSpaceTimeCurvature(x, y2, z, celestialObjects[0], celestialObjects);
                                float displacement2 = curvature2 * 50.0f;

                                glVertex3f(x, y - displacement, z);
                                glVertex3f(x, y2 - displacement2, z);
                            }

                            // Add Z-direction lines but even more sparsely
                            if (k < GRID_SIZE_3D - 1 && (i + j + k) % 3 == 0)
                            {
                                float z2 = z + GRID_SPACING_3D * 2.0f;
                                float curvature2 = calculateSpaceTimeCurvature(x, y, z2, celestialObjects[0], celestialObjects);
                                float displacement2 = curvature2 * 500.0f;

                                glVertex3f(x, y - displacement, z);
                                glVertex3f(x, y - displacement2, z2);
                            }
                        }
                    }
                }
                glEnd();
            }
            else
            {
                // Draw 2D grid (XY plane)
                glBegin(GL_LINES);
                for (int i = 0; i < GRID_SIZE; ++i)
                {
                    for (int j = 0; j < GRID_SIZE; ++j)
                    {
                        float x = (i - GRID_SIZE / 2) * GRID_SPACING;
                        float y = (j - GRID_SIZE / 2) * GRID_SPACING;
                        float z = -200.0f; // Fixed Z plane below the solar system

                        // Calculate space-time curvature
                        float curvature = calculateSpaceTimeCurvature(x, y, 0, celestialObjects[0], celestialObjects);
                        float displacement = curvature * 500.0f;

                        // Draw lines to adjacent grid points with curvature displacement
                        if (i < GRID_SIZE - 1)
                        {
                            float x2 = x + GRID_SPACING;
                            float curvature2 = calculateSpaceTimeCurvature(x2, y, 0, celestialObjects[0], celestialObjects);
                            float displacement2 = curvature2 * 500.0f;

                            glVertex3f(x, y, z - displacement);
                            glVertex3f(x2, y, z - displacement2);
                        }

                        if (j < GRID_SIZE - 1)
                        {
                            float y2 = y + GRID_SPACING;
                            float curvature2 = calculateSpaceTimeCurvature(x, y2, 0, celestialObjects[0], celestialObjects);
                            float displacement2 = curvature2 * 500.0f;

                            glVertex3f(x, y, z - displacement);
                            glVertex3f(x, y2, z - displacement2);
                        }
                    }
                }
                glEnd();
            }

            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }

        // --------------- N-BODY PHYSICS UPDATE ---------------
        // Compute accelerations (pixels / s^2) for every object from every other object
        size_t n = celestialObjects.size();
        std::vector<std::array<float, 3>> accels(n);
        for (size_t i = 0; i < n; ++i)
        {
            accels[i] = {0.0f, 0.0f, 0.0f};
        }

        for (size_t i = 0; i < n; ++i)
        {
            auto pos_i = celestialObjects[i].GetCoord();
            for (size_t j = 0; j < n; ++j)
            {
                if (i == j)
                    continue;

                auto pos_j = celestialObjects[j].GetCoord();
                float dx = pos_j[0] - pos_i[0];
                float dy = pos_j[1] - pos_i[1];
                float dz = pos_j[2] - pos_i[2];
                double dist_pixels = sqrt(dx * dx + dy * dy + dz * dz);

                if (dist_pixels < 1e-3)
                    continue; // avoid singularity / self

                // Convert pixel distance -> meters
                double dist_meters = dist_pixels * DISTANCE_SCALE;

                // Acceleration contribution from object j: a = G * m_j / r^2 (m/s^2)
                double a_m_s2 = G * celestialObjects[j].mass / (dist_meters * dist_meters);

                // Convert acceleration to pixels/s^2 for our simulation coordinates:
                double a_pixels_s2 = a_m_s2 / DISTANCE_SCALE;

                // direction unit vector (from i -> j)
                double dir_x = dx / dist_pixels;
                double dir_y = dy / dist_pixels;
                double dir_z = dz / dist_pixels;

                accels[i][0] += (float)(dir_x * a_pixels_s2);
                accels[i][1] += (float)(dir_y * a_pixels_s2);
                accels[i][2] += (float)(dir_z * a_pixels_s2);
            }
        }

        // Apply accelerations to velocities
        for (size_t i = 0; i < n; ++i)
        {
            celestialObjects[i].accelerate(accels[i][0], accels[i][1], accels[i][2], TIME_STEP);
        }

        // Update positions and draw objects
        for (auto &object : celestialObjects)
        {
            object.UpdatePos(TIME_STEP);
            object.Draw(sunPos, blackHolePositions);
        }

        // Update black hole positions for lighting (recompute because objects moved)
        blackHolePositions.clear();
        for (const auto &obj : celestialObjects)
        {
            if (obj.IsBlackHole())
            {
                auto pos = obj.GetCoord();
                blackHolePositions.push_back({pos[0], pos[1], pos[2], obj.GetRadius()});
            }
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}