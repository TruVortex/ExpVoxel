#include <SDL2/SDL.h>
#include <tracy/Tracy.hpp>
#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <atomic>

#include "world/grid.hpp"
#include "concurrency/work_stealing_pool.hpp"
#include "rendering/tile_renderer.hpp"

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr int RENDER_WIDTH = 960;
constexpr int RENDER_HEIGHT = 540;
constexpr int GRID_BRICKS = 8;

void generate_terrain(Grid& grid) {
    for (int x = 0; x < 64; ++x) {
        for (int z = 0; z < 64; ++z) {
            float height_val = 14.0f + 8.0f * std::sin(x * 0.15f) * std::cos(z * 0.15f) + 4.0f * std::sin((x + z) * 0.1f);
            int h = static_cast<int>(height_val);
            for (int y = 0; y <= h && y < 64; ++y) {
                if (y == h) {
                    grid.set_voxel(x, y, z, 40, 180, 60);
                } else if (y > h - 4) {
                    grid.set_voxel(x, y, z, 120, 80, 40);
                } else if (y > 2) {
                    grid.set_voxel(x, y, z, 90, 95, 100);
                } else {
                    grid.set_voxel(x, y, z, 240, 60, 20);
                }
            }
        }
    }
    auto add_sphere = [&](int cx, int cy, int cz, int r, uint8_t cr, uint8_t cg, uint8_t cb) {
        for (int x = cx - r; x <= cx + r; ++x) {
            for (int y = cy - r; y <= cy + r; ++y) {
                for (int z = cz - r; z <= cz + r; ++z) {
                    int dx = x - cx, dy = y - cy, dz = z - cz;
                    if (dx*dx + dy*dy + dz*dz <= r*r) {
                        grid.set_voxel(x, y, z, cr, cg, cb);
                    }
                }
            }
        }
    };
    add_sphere(20, 35, 20, 6, 0, 220, 255);
    add_sphere(44, 40, 44, 7, 255, 40, 200);
    add_sphere(32, 45, 32, 5, 255, 200, 30);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "ExpVoxel Showcase | [SPACE] Toggle Auto-Orbit | [WASD] Move | [R] Reset Terrain",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT
    );
    Grid grid(GRID_BRICKS, GRID_BRICKS, GRID_BRICKS);
    generate_terrain(grid);
    std::vector<uint32_t> pixels(RENDER_WIDTH * RENDER_HEIGHT, 0);
    WorkStealingPool thread_pool;
    std::atomic<bool> sim_running{true};
    std::thread mutator_thread([&grid, &sim_running] {
        float angle = 0.0f;
        int prev_laser_x = -1, prev_laser_z = -1;
        int prev_px = -1, prev_py = -1, prev_pz = -1;
        while (sim_running.load(std::memory_order_relaxed)) {
            angle += 0.04f;
            if (prev_laser_x >= 0) {
                for (int y = 0; y < 48; ++y) {
                    grid.clear_voxel_atomic(prev_laser_x, y, prev_laser_z);
                }
            }
            int laser_x = static_cast<int>(32.0f + 20.0f * std::cos(angle));
            int laser_z = static_cast<int>(32.0f + 20.0f * std::sin(angle));
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    for (int y = 0; y < 22; ++y) {
                        grid.clear_voxel_atomic(laser_x + dx, y, laser_z + dz);
                    }
                }
            }
            for (int y = 0; y < 48; ++y) {
                grid.set_voxel(laser_x, y, laser_z, 255, 30, 30);
            }
            prev_laser_x = laser_x;
            prev_laser_z = laser_z;
            int px = static_cast<int>(32.0f + 12.0f * std::sin(angle * 2.0f));
            int py = static_cast<int>(35.0f + 5.0f * std::cos(angle * 3.0f));
            int pz = static_cast<int>(32.0f + 12.0f * std::cos(angle * 2.0f));
            if (prev_px >= 0) grid.clear_voxel_atomic(prev_px, prev_py, prev_pz);
            grid.set_voxel(px, py, pz, 255, 255, 255);
            prev_px = px; prev_py = py; prev_pz = pz;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    bool running = true;
    bool auto_orbit = true;
    float time_sec = 0.0f;
    Vec3 cam_pos{32.0f, 40.0f, -40.0f};
    Vec3 target{32.0f, 18.0f, 32.0f};
    SDL_Event event;
    while (running) {
        FrameMark;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_SPACE: auto_orbit = !auto_orbit; break;
                    case SDLK_r: generate_terrain(grid); break;
                    case SDLK_w: cam_pos.z += 2.0f; break;
                    case SDLK_s: cam_pos.z -= 2.0f; break;
                    case SDLK_a: cam_pos.x -= 2.0f; break;
                    case SDLK_d: cam_pos.x += 2.0f; break;
                }
            }
        }
        time_sec += 0.016f;
        if (auto_orbit) {
            float radius = 55.0f;
            cam_pos.x = 32.0f + radius * std::sin(time_sec * 0.4f);
            cam_pos.z = 32.0f - radius * std::cos(time_sec * 0.4f);
            cam_pos.y = 35.0f + 12.0f * std::sin(time_sec * 0.2f);
        }
        Vec3 cam_forward = (target - cam_pos).normalized();
        Vec3 world_up{0.0f, 1.0f, 0.0f};
        Vec3 cam_right = Vec3{
            cam_forward.y * world_up.z - cam_forward.z * world_up.y,
            cam_forward.z * world_up.x - cam_forward.x * world_up.z,
            cam_forward.x * world_up.y - cam_forward.y * world_up.x
        }.normalized();
        Vec3 cam_up = Vec3{
            cam_right.y * cam_forward.z - cam_right.z * cam_forward.y,
            cam_right.z * cam_forward.x - cam_right.x * cam_forward.z,
            cam_right.x * cam_forward.y - cam_right.y * cam_forward.x
        }.normalized();
        float fov = 65.0f * (3.14159f / 180.0f);
        auto start_time = std::chrono::high_resolution_clock::now();
        {
            ZoneScopedN("RenderFrame");
            render_tiles_multithreaded(
                thread_pool, grid, pixels.data(),
                RENDER_WIDTH, RENDER_HEIGHT,
                cam_pos, fov,
                cam_forward, cam_right, cam_up
            );
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        double frame_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        double mrays_sec = (RENDER_WIDTH * RENDER_HEIGHT) / (frame_ms * 1000.0);
        std::string title = "ExpVoxel | Threads: " + std::to_string(thread_pool.thread_count()) +
                            " | Frame: " + std::to_string(frame_ms).substr(0, 5) + " ms | " +
                            std::to_string(mrays_sec).substr(0, 5) + " MRays/s";
        SDL_SetWindowTitle(window, title.c_str());
        SDL_UpdateTexture(texture, nullptr, pixels.data(), RENDER_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
    sim_running.store(false);
    mutator_thread.join();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}