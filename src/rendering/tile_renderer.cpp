#include "rendering/tile_renderer.hpp"
#include "concurrency/work_stealing_pool.hpp"
#include "rendering/simd_dda.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <tracy/Tracy.hpp>
#include <vector>

struct TileTask {
    int tile_x, tile_y;
    int x_end, y_end;
};

void render_tiles_multithreaded(WorkStealingPool &pool, const Grid &grid,
                                uint32_t *pixels, int width, int height,
                                Vec3 cam_pos, float fov, Vec3 cam_forward,
                                Vec3 cam_right, Vec3 cam_up, int tile_size) {
    ZoneScopedN("TileRenderer");
    float aspect_ratio = static_cast<float>(width) / height;
    float tan_fov = std::tan(fov * 0.5f);
    std::vector<TileTask> tiles;
    tiles.reserve(((height + tile_size - 1) / tile_size) *
                  ((width + tile_size - 1) / tile_size));
    for (int tile_y = 0; tile_y < height; tile_y += tile_size) {
        for (int tile_x = 0; tile_x < width; tile_x += tile_size) {
            tiles.push_back({tile_x, tile_y,
                             std::min(tile_x + tile_size, width),
                             std::min(tile_y + tile_size, height)});
        }
    }
    std::atomic<size_t> tile_idx{0};
    const size_t total_tiles = tiles.size();
    const size_t threads = pool.thread_count();
    for (size_t t = 0; t < threads; ++t) {
        pool.enqueue([&, width, height, cam_pos, tan_fov, aspect_ratio,
                      cam_forward, cam_right, cam_up]() {
            alignas(32) RayPacket8 packet{};
            int pixel_indices[8];
            while (true) {
                size_t curr = tile_idx.fetch_add(1, std::memory_order_relaxed);
                if (curr >= total_tiles) {
                    break;
                }
                const auto &tile = tiles[curr];
                int packet_idx = 0;
                for (int y = tile.tile_y; y < tile.y_end; ++y) {
                    for (int x = tile.tile_x; x < tile.x_end; ++x) {
                        float u = (2.0f * (x + 0.5f) / width - 1.0f) *
                                  aspect_ratio * tan_fov;
                        float v = (1.0f - 2.0f * (y + 0.5f) / height) * tan_fov;
                        Vec3 ray_dir =
                            (cam_forward + cam_right * u + cam_up * v)
                                .normalized();
                        packet.orig_x[packet_idx] = cam_pos.x;
                        packet.orig_y[packet_idx] = cam_pos.y;
                        packet.orig_z[packet_idx] = cam_pos.z;
                        packet.dir_x[packet_idx] = ray_dir.x;
                        packet.dir_y[packet_idx] = ray_dir.y;
                        packet.dir_z[packet_idx] = ray_dir.z;
                        pixel_indices[packet_idx] = y * width + x;
                        packet_idx++;
                        if (packet_idx == 8) {
                            packet.active_mask = 0xFF;
                            auto hits = trace_dda_simd_packet(packet, grid);
                            for (int i = 0; i < 8; ++i) {
                                if (hits[i].hit) {
                                    pixels[pixel_indices[i]] =
                                        (0xFF << 24) | (hits[i].colour_r << 16) |
                                        (hits[i].colour_g << 8) |
                                        hits[i].colour_b;
                                } else {
                                    pixels[pixel_indices[i]] = 0xFF101015;
                                }
                            }
                            packet_idx = 0;
                        }
                    }
                }
                if (packet_idx > 0) {
                    packet.active_mask = (1 << packet_idx) - 1;
                    auto hits = trace_dda_simd_packet(packet, grid);
                    for (int i = 0; i < packet_idx; ++i) {
                        if (hits[i].hit) {
                            pixels[pixel_indices[i]] =
                                (0xFF << 24) | (hits[i].colour_r << 16) |
                                (hits[i].colour_g << 8) | hits[i].colour_b;
                        } else {
                            pixels[pixel_indices[i]] = 0xFF101015;
                        }
                    }
                }
            }
        });
    }
    pool.wait_all();
}