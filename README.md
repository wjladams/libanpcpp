# libanpcpp (CMake package: anpcpp)

C++20 library for Analytic Network Process (ANP) models, pairwise judgments,
supermatrices, limit-matrix calculations, synthesis (including subnetworks),
and versioned JSON persistence.

The public C++ API lives in namespace `cppanp` (headers under `include/cppanp/`).
The CMake target is `anpcpp::anpcpp`. There is no Qt dependency.

Numerical behavior is cross-checked against concepts from
[pyanp](https://pyanp.org/) (reference only, not a dependency).

## Features

- Dense `Matrix` / `Vector`
- Principal eigen (power iteration) and Saaty CI/CR
- `PairwiseJudgments` for named comparison tables
- `AnpNetwork` / `AnpCluster` / `AnpNode` with pairwise wiring
- Unscaled / cluster-weighted / scaled supermatrices
- Limit matrix (calculus method, with hierarchy short-circuit)
- Recursive subnetworks with additive / multiplicative / custom synthesis
- Versioned JSON save/load (`cppanp` format v1)

## Build and test

Requires a C++20 compiler and CMake 3.20+. Network access is needed on the
first configure so CMake can download [nlohmann/json](https://github.com/nlohmann/json)
and (for tests) [GoogleTest](https://github.com/google/googletest).

```bash
cmake -S . -B build -DANPCPP_BUILD_TESTS=ON -DANPCPP_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Disable tests or examples with `-DANPCPP_BUILD_TESTS=OFF` /
`-DANPCPP_BUILD_EXAMPLES=OFF`. When this project is pulled in as a
subdirectory / FetchContent dependency, tests and examples default to **OFF**.

### Examples

Runnable demos land in `build/examples/`:

- `tree134` – goal / 3 criteria / 3 alternatives AHP hierarchy
- `network23` – fully connected 2-cluster ANP network with feedback
- `hamburger_std` – SuperDecisions hamburger market-share network
- `benefits_costs_subnet` – Benefits/Costs control network with subnetworks

```bash
./build/examples/tree134
```

## Use from another CMake project

### FetchContent (recommended)

```cmake
include(FetchContent)
FetchContent_Declare(anpcpp
  GIT_REPOSITORY https://github.com/<org>/libanpcpp.git
  GIT_TAG v0.1.0)
FetchContent_MakeAvailable(anpcpp)
target_link_libraries(myapp PRIVATE anpcpp::anpcpp)
```

For local development against a sibling checkout:

```cmake
FetchContent_Declare(anpcpp
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libanpcpp")
FetchContent_MakeAvailable(anpcpp)
```

### Install + find_package

```bash
cmake -S . -B build -DANPCPP_BUILD_TESTS=OFF -DANPCPP_BUILD_EXAMPLES=OFF
cmake --build build
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(anpcpp 0.1 REQUIRED)
target_link_libraries(myapp PRIVATE anpcpp::anpcpp)
```

Set `CMAKE_PREFIX_PATH` to the install prefix if needed.

## Desktop GUI

The SuperDecisions-style Qt application lives in a separate repository,
`cppanp`, which depends on this library via FetchContent (or a sibling
`../libanpcpp` checkout during development).

## Layout

```text
include/cppanp/   Public headers
src/              Implementation
tests/            GoogleTest suite
examples/         Console ANP demos
docs/             Developer notes
cmake/            Package config templates
```
