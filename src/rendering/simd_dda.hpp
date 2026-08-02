#pragma once
#include "core/math.hpp"
#include "world/grid.hpp"
#include <array>

std::array<HitResult, 8> trace_dda_simd_packet(const RayPacket8& packet, const Grid& grid);