#pragma once
#include "core/math.hpp"
#include "world/grid.hpp"

HitResult trace_dda_scalar(const Ray& ray, const Grid& grid);