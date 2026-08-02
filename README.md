# ExpVoxel

ExpVoxel is a low-latency CPU voxel raytracing project written in modern C++20. It combines **Data-Oriented Design**, **AVX2 SIMD Ray-Packet Traversal**, a **Lock-Free Chase-Lev Work-Stealing Scheduler**, and **Epoch-Based Reclamation** for concurrent world mutations.

---

## Architectural Highlights

ExpVoxel leverages a data-oriented memory layout where the 3D grid is decomposed into $8\times8\times8$ cache-line aligned bricks (`alignas(64)`). Custom thread-local linear arena allocators eliminate heap allocations along the hot rendering path, while bitwise shifts replace division and modulo operations to minimize CPU cycle overhead. Traversal uses a Two-Level DDA algorithm that evaluates empty $8\times8\times8$ bricks in a single operation, bypassing unpopulated spatial regions instantly.

To maximize instruction-level parallelism, rays are first evaluated against the world's axis-aligned bounding box (AABB) using a fast slab intersection test, rejecting out-of-bounds rays in zero DDA steps. Traversal executes via AVX2 SIMD intrinsics (`__m256`) processing 8 rays simultaneously in a Structure of Arrays (SoA) layout. Work distribution across CPU cores is managed by a custom single-producer multi-consumer Chase-Lev work-stealing scheduler built with fine-grained C++20 atomic memory orderings (`memory_order_acquire`, `release`, `seq_cst`) for zero-allocation tile dispatch.

Dynamic scene modifications operate asynchronously via lock-free atomic voxel occupancy masks (`std::atomic<uint64_t>`). Background simulation threads mutate the world state while render threads trace rays without mutex locks or thread stalls. Memory safety during concurrent deletions is guaranteed via Epoch-Based Reclamation.

---

## Performance & Thread Scaling

Benchmarked at 720p resolution ($1280 \times 720$):

| Threads | Avg Frame Time | Throughput | Speedup vs 1T |
| :---: | :---: | :---: | :---: |
| 1 | 96.47 ms | 9.55 MRays/s | **1.00x** |
| 2 | 47.89 ms | 19.24 MRays/s | **2.01x** |
| 4 | 24.36 ms | 37.84 MRays/s | **3.96x** |
| 8 | 15.43 ms | 59.74 MRays/s | **6.25x** |
| 16 | 13.08 ms | 70.47 MRays/s | **7.38x** |

---

## Build & Test Instructions

### Prerequisites
* C++20 Compliant Compiler (GCC 12+, Clang 15+, MSVC 2022)
* CMake 3.20+ & Ninja
* SDL2 Library

### Build Project
```bash
cmake -B build -G Ninja
ninja -C build
```

### Run Unit Tests
```bash
ctest --test-dir build --output-on-failure
```

### Run Benchmark Suite
```bash
./build/bench_engine
```

### Run Application
```bash
./build/expvoxel_runner
```
Launches the real-time application rendering a $64\times64\times64$ voxel landscape demo:

* **`W`**: Move camera position forward along the Z-axis (towards the terrain).
* **`S`**: Move camera position backward along the Z-axis (away from the terrain).
* **`A`**: Move camera position left along the X-axis.
* **`D`**: Move camera position right along the X-axis.
* **`SPACE`**: Toggle automatic orbiting camera mode on or off.
* **`R`**: Regenerate the procedural terrain landscape and floating spheres.
* **`ESC`**: Close and exit the application.
