# libanpcpp (CMake package: anpcpp)

C++20 library for Analytic Network Process (ANP) models, pairwise judgments,
supermatrices, limit-matrix calculations, synthesis (including subnetworks),
and versioned JSON persistence.

The public C++ API lives in namespace `anpcpp` (headers under `include/anpcpp/`).
The CMake target is `anpcpp::anpcpp`. There is no Qt dependency.

Numerical behavior is cross-checked against concepts from
[pyanp](https://pyanp.org/) (reference only, not a dependency).

## Features

- Dense `Matrix` / `Vector`
- Principal eigen (power iteration) and Saaty CI/CR
- `PairwiseJudgments` for named comparison tables
- `RatingsPrioritizer` for categorical or numeric rating columns (with declarative score interpreters)
- `AnpNetwork` / `AnpCluster` / `AnpNode` with pairwise or ratings wiring per dest cluster
- Unscaled / cluster-weighted / scaled supermatrices
- Limit matrix (calculus, new hierarchy, sinks; hierarchy short-circuit)
- Recursive subnetworks with additive / multiplicative / custom synthesis
- Versioned JSON save/load (`anpcpp` format v1; `node_prioritizers` with pairwise/ratings)

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

Runnable demos land in `build/examples/`. A guided walkthrough of each demo is
in [`docs/examples.md`](docs/examples.md) (published at
[https://bamath.org/libanpcpp/api/examples.html](https://bamath.org/libanpcpp/api/examples.html)).

- `tree134` – goal / 3 criteria / 3 alternatives AHP hierarchy
- `network23` – fully connected 2-cluster ANP network with feedback
- `hamburger_std` – SuperDecisions hamburger market-share network
- `benefits_costs_subnet` – Benefits/Costs control network with subnetworks
- `ratings_demo` – categorical + numeric `RatingsPrioritizer` columns toward Alternatives

```bash
./build/examples/tree134
```

## Use from another CMake project

### FetchContent (recommended)

```cmake
include(FetchContent)
FetchContent_Declare(anpcpp
  GIT_REPOSITORY https://github.com/wjadams/libanpcpp.git
  GIT_TAG v0.3.0)
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
find_package(anpcpp 0.3 REQUIRED)
target_link_libraries(myapp PRIVATE anpcpp::anpcpp)
```

Set `CMAKE_PREFIX_PATH` to the install prefix if needed.

## API documentation

**Site:** [https://bamath.org/libanpcpp/](https://bamath.org/libanpcpp/) (README
landing page, published from `main` via GitHub Actions).

**API reference:** [https://bamath.org/libanpcpp/api/](https://bamath.org/libanpcpp/api/)

**Local build** requires [Doxygen](https://www.doxygen.org/) (e.g.
`sudo apt install doxygen` on Ubuntu). With the default
`ANPCPP_BUILD_DOCS=ON` (top-level builds), `cmake --build build` runs the
same Doxygen pass as CI (`WARN_AS_ERROR`). Docs-only:

```bash
cmake -S . -B build -DANPCPP_BUILD_DOCS=ON -DANPCPP_BUILD_TESTS=OFF -DANPCPP_BUILD_EXAMPLES=OFF
cmake --build build --target anpcpp_docs
```

Open **[build/docs/html/index.html](build/docs/html/index.html)** in a browser.

To preview the full site locally (README landing + API under `api/`):

```bash
cmake -S . -B build -DANPCPP_BUILD_DOCS=ON -DANPCPP_BUILD_TESTS=OFF -DANPCPP_BUILD_EXAMPLES=OFF
cmake --build build --target anpcpp_docs
cp -a build/docs/html docs/site/api
cp README.md docs/site/README.content.md
cd docs/site && bundle install && bundle exec jekyll serve --baseurl /libanpcpp
```

## Desktop GUI

The SuperDecisions-style Qt application lives in a separate repository,
[ANP Studio](https://github.com/wjladams/anp-studio) (`anp-studio`), which
depends on this library via FetchContent (or a sibling `../libanpcpp`
checkout during development).

## Layout

```text
include/anpcpp/   Public headers
src/              Implementation
tests/            GoogleTest suite
examples/         Console ANP demos
docs/             Doxygen config and developer notes
cmake/            Package config templates
```

## License

MIT License. See [LICENSE](LICENSE).
