# libanpcpp API Reference

C++20 library for **Analytic Network Process (ANP)** models, pairwise judgments,
supermatrices, limit-matrix calculations, synthesis (including subnetworks),
and versioned JSON persistence.

## Quick start

- **Namespace:** `cppanp`
- **CMake target:** `anpcpp::anpcpp`
- **Headers:** `#include <cppanp/network.hpp>` (and related headers)

```cpp
#include <cppanp/network.hpp>

cppanp::AnpNetwork net;
auto& goal = net.add_cluster("Goal");
// ...
```

## Modules

| Header | Contents |
|--------|----------|
| @ref matrix.hpp | Dense `Vector` and `Matrix` |
| @ref eigen.hpp | Principal eigenvector (power iteration) |
| @ref inconsistency.hpp | Saaty consistency index and ratio |
| @ref pairwise.hpp | Named pairwise comparison tables |
| @ref limit_matrix.hpp | Limit matrix and priority extraction |
| @ref network.hpp | `AnpNetwork`, `AnpCluster`, `AnpNode` |
| @ref synthesis.hpp | Subnetwork score synthesis |
| @ref json_io.hpp | JSON save/load (format v1) |

## Examples

Runnable console demos live in the `examples/` directory. See the
@ref examples "examples guide" for what each demo teaches and how to run it.

## Desktop application

The SuperDecisions-style Qt GUI is in the separate **cppanp** repository.
