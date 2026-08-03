# ExpVoxel

ExpVoxel is a low-latency CPU voxel raytracing project that explores modern systems programming techniques for high-performance rendering. Rather than focusing solely on rendering images, the project investigates how these techniques interact among themselves to create a scalable, performant application.

In particular, ExpVoxel was motivated as an *exp*loration of:

* Cache-aware data structures
* SIMD programming
* Lock-free work stealing
* Thread-local memory allocation
* Epoch-based memory reclamation
* Multicore task scheduling

---

## Implementation Features

ExpVoxel uses a memory layout where the 3D grid is decomposed into $8\times8\times8$ cache-line aligned bricks. Thread-local linear arena allocators eliminate heap allocations along the rendering path, while traversal uses a Two-Level DDA algorithm that evaluates empty bricks in a single operation, bypassing unpopulated spatial regions instantly.

To maximize instruction-level parallelism, rays are first evaluated against the world's axis-aligned bounding box using a fast slab intersection test, rejecting out-of-bounds rays in zero DDA steps. Traversal then executes via AVX2 intrinsics; ExpVoxel processes rays simultaneously by leveraging an SoA layout. Work distribution across CPU cores is managed by a lock-free single-producer multiple-consumer work-stealing scheduler (Chase-Lev) built with C++20 atomic orderings for zero-allocation tile dispatch.

Finally, dynamic scene modifications operate asynchronously via lock-free atomic occupancy masks; background simulation threads mutate the world state while render threads trace rays without mutex locks or thread stalls. In particular, memory safety during concurrent deletions is guaranteed via Epoch-Based Reclamation.

---

## Benchmarks

Benchmarked at 720p resolution on a `Ryzen 5 9600X`:

| Threads | Avg Frame Time | Throughput | Speedup vs 1T |
| :---: | :---: | :---: | :---: |
| 1 | 96.47 ms | 9.55 MRays/s | **1.00x** |
| 2 | 47.89 ms | 19.24 MRays/s | **2.01x** |
| 4 | 24.36 ms | 37.84 MRays/s | **3.96x** |
| 8 | 15.43 ms | 59.74 MRays/s | **6.25x** |
| 16 | 13.08 ms | 70.47 MRays/s | **7.38x** |

---

## Usage

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

### Run Benchmarks
```bash
./build/bench_engine
```

### Run Application
```bash
./build/expvoxel_runner
```
Launches the interactive application, rendering a $64\times64\times64$ voxel landscape demo:

* **`W`**: Move camera position forward along the Z-axis (towards the terrain).
* **`S`**: Move camera position backward along the Z-axis (away from the terrain).
* **`A`**: Move camera position left along the X-axis.
* **`D`**: Move camera position right along the X-axis.
* **`SPACE`**: Toggle automatic orbiting camera mode on or off.
* **`R`**: Regenerate the procedural terrain landscape and floating spheres.
* **`ESC`**: Close and exit the application.
