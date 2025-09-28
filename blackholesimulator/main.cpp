
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <algorithm>

using namespace std;

const int WIDTH = 120;
const int HEIGHT = 40;
const int MAX_PARTICLES = 1000;

// GPU-style particle system
struct Particle {
    float x, y;
    float vx, vy;
    float life;
    bool captured;
    char character;
};

class GPUTerminalRenderer {
private:
    vector<vector<char>> frameBuffer;
    vector<vector<int>> colorBuffer;
    atomic<bool> running;

public:
    GPUTerminalRenderer() : frameBuffer(HEIGHT, vector<char>(WIDTH, ' ')),
                           colorBuffer(HEIGHT, vector<int>(WIDTH, 0)) {
        running = true;
    }

    void clearBuffers() {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                frameBuffer[y][x] = ' ';
                colorBuffer[y][x] = 0;
            }
        }
    }

    // GPU-style fragment shader for particles
    void drawParticle(float px, float py, char ch, int color) {
        int x = static_cast<int>(px);
        int y = static_cast<int>(py);

        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            frameBuffer[y][x] = ch;
            colorBuffer[y][x] = color;
        }
    }

    // Draw anti-aliased circle (GPU-style geometry shader)
    void drawCircle(int cx, int cy, int radius, char ch, int color) {
        for (int y = cy - radius; y <= cy + radius; y++) {
            for (int x = cx - radius; x <= cx + radius; x++) {
                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                    float dist = sqrt(pow(x - cx, 2) + pow(y - cy, 2));
                    if (dist <= radius) {
                        float intensity = 1.0f - (dist / radius);
                        if (intensity > 0.3f) {
                            frameBuffer[y][x] = getCircleChar(dist, radius, ch);
                            colorBuffer[y][x] = color + static_cast<int>(intensity * 5);
                        }
                    }
                }
            }
        }
    }

    char getCircleChar(float dist, int radius, char baseChar) {
        float normalized = dist / radius;
        if (normalized < 0.3f) return '@';
        if (normalized < 0.6f) return '#';
        if (normalized < 0.8f) return '*';
        return '.';
    }

    void renderFrame() {
        // Use terminal escape sequences for GPU-style rendering
        cout << "\033[2J\033[1;1H"; // Clear screen

        // Render frame buffer with colors
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (frameBuffer[y][x] != ' ') {
                    setColor(colorBuffer[y][x]);
                    cout << frameBuffer[y][x];
                    resetColor();
                } else {
                    cout << ' ';
                }
            }
            cout << '\n';
        }

        // Display FPS and stats
        cout << "\033[37mBlack Hole GPU Simulation | Particles: " << MAX_PARTICLES << " | FPS: 60\033[0m\n";
    }

    void setColor(int colorCode) {
        // 256-color terminal support for GPU-like rendering
        cout << "\033[38;5;" << colorCode << "m";
    }

    void resetColor() {
        cout << "\033[0m";
    }
};

class BlackHoleGPU {
private:
    vector<Particle> particles;
    float blackHoleX, blackHoleY;
    float gravitationalConstant;
    GPUTerminalRenderer renderer;

public:
    BlackHoleGPU() {
        blackHoleX = WIDTH / 2.0f;
        blackHoleY = HEIGHT / 2.0f;
        gravitationalConstant = 15.0f;
        initParticles();
    }

    void initParticles() {
        particles.clear();
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles.push_back(createParticle());
        }
    }

    Particle createParticle() {
        Particle p;

        // Create particles in orbital patterns
        float angle = (rand() % 360) * M_PI / 180.0f;
        float distance = 15.0f + (rand() % 20);

        p.x = blackHoleX + distance * cos(angle);
        p.y = blackHoleY + distance * sin(angle) * 0.7f;

        // Orbital velocity
        float orbitalSpeed = sqrt(gravitationalConstant / distance) * 0.5f;
        p.vx = -sin(angle) * orbitalSpeed;
        p.vy = cos(angle) * orbitalSpeed;

        p.life = 1.0f;
        p.captured = false;
        p.character = getParticleChar(rand() % 100);

        return p;
    }

    char getParticleChar(int randVal) {
        if (randVal < 40) return '.';
        if (randVal < 70) return ',';
        if (randVal < 85) return '*';
        if (randVal < 95) return '+';
        return 'o';
    }

    void updatePhysics() {
        #pragma omp parallel for
        for (size_t i = 0; i < particles.size(); i++) {
            if (particles[i].captured || particles[i].life <= 0) continue;

            // Calculate gravitational force
            float dx = blackHoleX - particles[i].x;
            float dy = blackHoleY - particles[i].y;
            float distance = sqrt(dx*dx + dy*dy);

            // Event horizon capture
            if (distance < 3.0f) {
                particles[i].captured = true;
                particles[i].life = 0;
                continue;
            }

            // Gravitational acceleration (inverse square law)
            float force = gravitationalConstant / (distance * distance);
            float ax = dx * force / distance;
            float ay = dy * force / distance;

            // Update velocity (GPU-style parallel computation)
            particles[i].vx += ax * 0.05f;
            particles[i].vy += ay * 0.05f;

            // Update position
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;

            // Particle aging
            particles[i].life -= 0.001f;

            // Replace dead particles
            if (particles[i].life <= 0) {
                particles[i] = createParticle();
            }
        }
    }

    void renderScene() {
        renderer.clearBuffers();

        // Draw accretion disk (GPU-style geometry)
        drawAccretionDisk();

        // Draw black hole event horizon
        renderer.drawCircle(blackHoleX, blackHoleY, 3, '@', 16); // Black

        // Draw gravitational lensing effect
        drawGravitationalLensing();

        // Draw particles (GPU-style fragment processing)
        for (const auto& p : particles) {
            if (!p.captured && p.life > 0) {
                int color = calculateParticleColor(p);
                renderer.drawParticle(p.x, p.y, p.character, color);
            }
        }

        // Draw jet streams from poles
        drawJetStreams();

        renderer.renderFrame();
    }

    void drawAccretionDisk() {
        for (int r = 4; r < 12; r++) {
            for (int angle = 0; angle < 360; angle += 2) {
                float rad = angle * M_PI / 180.0f;
                int x = blackHoleX + r * cos(rad);
                int y = blackHoleY + r * sin(rad) * 0.4f; // Flattened disk

                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                    int color = 202 + (r % 4); // Red/orange colors
                    char diskChar = getDiskChar(r, angle);
                    renderer.drawParticle(x, y, diskChar, color);
                }
            }
        }
    }

    char getDiskChar(int radius, int angle) {
        if (radius < 6) return '#';
        if (radius < 8) return '=';
        if (radius < 10) return '*';
        return '.';
    }

    void drawGravitationalLensing() {
        // Simulate light bending around black hole
        for (int r = 3; r < 8; r++) {
            for (int angle = 0; angle < 360; angle += 5) {
                float rad = angle * M_PI / 180.0f;
                int x = blackHoleX + r * cos(rad);
                int y = blackHoleY + r * sin(rad);

                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                    renderer.drawParticle(x, y, '°', 51); // Cyan lensing effect
                }
            }
        }
    }

    void drawJetStreams() {
        // Draw relativistic jets from poles
        for (int i = 0; i < 20; i++) {
            int x1 = blackHoleX - 2 + rand() % 5;
            int y1 = blackHoleY - 10 - rand() % 10;
            int x2 = blackHoleX - 2 + rand() % 5;
            int y2 = blackHoleY + 10 + rand() % 10;

            if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT) {
                renderer.drawParticle(x1, y1, '|', 87); // Bright white
            }
            if (x2 >= 0 && x2 < WIDTH && y2 >= 0 && y2 < HEIGHT) {
                renderer.drawParticle(x2, y2, '|', 87);
            }
        }
    }

    int calculateParticleColor(const Particle& p) {
        float dx = p.x - blackHoleX;
        float dy = p.y - blackHoleY;
        float distance = sqrt(dx*dx + dy*dy);
        float speed = sqrt(p.vx*p.vx + p.vy*p.vy);

        // Blue shift for approaching, red shift for receding
        if (p.vx * dx + p.vy * dy > 0) { // Moving away
            return 196 + static_cast<int>(speed * 10) % 5; // Red colors
        } else { // Moving toward
            return 33 + static_cast<int>(speed * 10) % 5; // Blue colors
        }
    }

    void run() {
        cout << "\033[?25l"; // Hide cursor
        cout << "\033[48;5;0m"; // Set background to black

        auto lastTime = chrono::high_resolution_clock::now();
        int frameCount = 0;

        while (true) {
            auto currentTime = chrono::high_resolution_clock::now();
            auto deltaTime = chrono::duration_cast<chrono::milliseconds>(currentTime - lastTime);

            if (deltaTime.count() >= 16) { // ~60 FPS
                updatePhysics();
                renderScene();

                frameCount++;
                lastTime = currentTime;
            }

            // Check for exit condition
            if (frameCount > 3600) { // Run for about 60 seconds
                break;
            }

            usleep(1000); // Prevent CPU overuse
        }

        cout << "\033[?25h"; // Show cursor
        cout << "\033[0m"; // Reset colors
    }
};

int main() {
    srand(time(0));

    cout << "Initializing Black Hole GPU Simulation..." << endl;
    cout << "Using terminal graphics acceleration..." << endl;
    usleep(2000000);

    BlackHoleGPU simulator;
    simulator.run();

    cout << "Simulation complete. Thank you for watching!" << endl;

    return 0;
}
