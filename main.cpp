#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <cmath>

class Object
{
private:
    std::vector<float> position = {400, 300};
    std::vector<float> velocity = {0, 0};
    float framerate = 1 / 60;

public:
    bool Initalizing = false;
    bool Launched = false;
    std::vector<float> hue = {1.0f, 1.0f, 1.0f, 1.0f};
    float mass;
    float density = 0.08375; // kg / m^3  HYDROGEN
    float radius; // Will be calculated after mass is set

    Object(std::vector<float> initPosition, std::vector<float> initVelocity, float mass)
    {
        this->position = initPosition;
        this->velocity = initVelocity;
        this->mass = mass;
    }

    void UpdatePos()
    {
        this->position[0] += this->velocity[0] / 94;
        this->position[1] += this->velocity[1] / 94;
    }
    void Velocity(float x, float y)
    {
        this->velocity[0] *= x;
        this->velocity[1] *= y;
    }
    void SetVel(float x, float y)
    {
        this->velocity = {x, y};
    }
    std::vector<float> GetCoord()
    {
        std::vector<float> xy = {this->position[0], this->position[1]};
        return xy;
    }
    float GetRadius()
    {
        float volume = this->mass / this->density;
        return pow(((3 * volume) / (4 * 3.14159265359)), (1.0f / 3.0f));
    }
    void Draw()
    {
        float volume = this->mass / this->density;
        float radius = pow(((3 * volume) / (4 * 3.14159265359)), (1.0f / 3.0f));
        float x = this->position[0];
        float y = this->position[1];
        glColor4f(this->hue[0], this->hue[1], this->hue[2], this->hue[3]);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);
        int numSegments = 100;
        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = 2.0f * 3.14159265359f * float(i) / float(numSegments);
            float dx = radius * cosf(angle);
            float dy = radius * sinf(angle);
            glVertex2f(x + dx, y + dy);
        }
        glEnd();
    };
    void CheckBoundry(int bottom, int top, int left, int right)
    {
        float radius = pow(((3 * (this->mass / this->density)) / (4 * 3.14159265359)), (1.0f / 3.0f));
        if (this->position[1] < bottom + radius || this->position[1] > top - radius)
        {
            this->position[1] = this->position[1] < bottom + radius ? bottom + radius : top - radius;
            this->velocity[1] *= -0.8f;
        }
        if (this->position[0] < left + radius || this->position[0] > right - radius)
        {
            this->position[0] = this->position[0] < left + radius ? left + radius : right - radius;
            this->velocity[0] *= -0.8f;
        }
    }
    void CheckCollision(std::vector<Object> &objs)
    {
        for (auto &obj2 : objs)
        {
            if (&obj2 == this)
                continue;
            float dx = obj2.GetCoord()[0] - this->position[0];
            float dy = obj2.GetCoord()[1] - this->position[1];
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < this->GetRadius() + obj2.GetRadius())
            {
                obj2.Velocity(1, -1);
                this->velocity[1] *= -1;
            }
        }
    }

    void accelerate(float x, float y)
    {
        this->velocity[0] += x;
        this->velocity[1] += y;
    }
};

int main(void)
{
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(800, 600, "Grav", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Get the actual window size
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    
    // Define screen dimensions for compatibility
    int screenWidth = windowWidth;
    int screenHeight = windowHeight;
    
    float radius = 50.0f;
    int res = 100;

    std::vector<float> position = {
        400.0f, 300.0f, // Center of the circle
    };

    std::vector<Object> objs = {
        Object({screenWidth / 3.0f, screenHeight / 2.0f}, {0.0f, 150.0f}, 4000.0f),        // Initial object
        Object({screenWidth / 3.0f * 2.0f, screenHeight / 2.0f}, {0.0f, 0.0f}, 5000.0f), // Another object for interaction
    };

    // Set up the viewport
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto &obj : objs)
        {
            obj.Draw();
            obj.UpdatePos();
            // obj gravity
            for (auto &obj2 : objs)
            {
                float dx = (obj2.GetCoord()[0] - obj.GetCoord()[0]);
                float dy = (obj2.GetCoord()[1] - obj.GetCoord()[1]);
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance != 0)
                {
                    std::vector<float> direction = {(obj2.GetCoord()[0] - obj.GetCoord()[0]) / distance, (obj2.GetCoord()[1] - obj.GetCoord()[1]) / distance};

                    obj.accelerate(direction[0], direction[1]);
                }
            }
            // gravity
            // obj.accelerate(0, -9.6);
            if (obj.Initalizing)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                ypos = screenHeight - ypos;
                obj.Velocity(0, 0);
                // target line
                glColor4f(1.0f, 0.4f, 0.4f, 0.2f);
                glLineWidth(2.0f);
                glBegin(GL_LINES);

                glVertex2d(xpos, ypos);
                glVertex2d(obj.GetCoord()[0], obj.GetCoord()[1]);

                float radius = pow(((3 * (obj.mass / obj.density)) / (4 * 3.14159265359)), (1.0f / 3.0f));

                // init mass
                if (xpos < obj.GetCoord()[0] + radius && xpos > obj.GetCoord()[0] - radius && ypos < obj.GetCoord()[1] + radius && ypos > obj.GetCoord()[1] - radius)
                {
                    obj.mass *= 1.05;
                }
                glEnd();
            }
            if (obj.Launched)
            {
                obj.Launched = false;
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                ypos = screenHeight - ypos;
                float dx = (xpos - obj.GetCoord()[0]);
                float dy = (ypos - obj.GetCoord()[1]);
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance != 0)
                {
                    std::vector<float> direction = {(float(xpos) - obj.GetCoord()[0]) * distance * 0.01f, (float(ypos) - obj.GetCoord()[1]) * distance * 0.01f};
                    obj.SetVel(-direction[0], -direction[1]);
                }
            }
            obj.CheckBoundry(0, screenHeight, 0, screenWidth);
            obj.CheckCollision(objs);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}