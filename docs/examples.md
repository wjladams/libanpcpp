# Examples guide {#examples}

Runnable console demos live in the repository `examples/` directory. Build them with
`-DANPCPP_BUILD_EXAMPLES=ON`, then run the binaries under `build/examples/`.

```bash
cmake -S . -B build -DANPCPP_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/tree134
```

Each program prints model inputs, intermediate matrices, and final priorities.
Shared formatting helpers are in `examples/anp_print.hpp` (not a demo itself).

| Demo | Teaches |
|------|---------|
| [tree134](#tree134) | AHP hierarchy; hierarchy short-circuit for the limit matrix |
| [network23](#network23) | Full feedback ANP; iterative calculus limit matrix |
| [hamburger_std](#hamburger_std) | Larger SuperDecisions reference market-share network |
| [benefits_costs_subnet](#benefits_costs_subnet) | Control network with subnetworks and inverted Costs |
| [ratings_demo](#ratings_demo) | Categorical and numeric ratings columns toward Alternatives |
| export_sample_models | Writes starter `.anpstudio` models under a chosen directory (see ANP Studio `samples/`) |

---

## tree134 {#tree134}

**Source:** `examples/tree134.cpp`

A classic three-level AHP tree:

```text
Goal:         Best Car
Criteria:     Cost, Quality, Style
Alternatives: Civic, Accord, CR-V
```

The goal connects to the three criteria; each criterion connects to the shared
alternatives cluster. The scaled supermatrix is a strict hierarchy (nilpotent),
so the library resolves the limit matrix with the hierarchy formula rather than
power iteration.

**What to notice:** criteria importance wrt the goal, then local rankings of
cars under Cost / Quality / Style, and the final global priorities for the
alternatives.

```bash
./build/examples/tree134
```

---

## network23 {#network23}

**Source:** `examples/network23.cpp`

A small fully connected ANP network with feedback:

```text
Criteria:     Price, Quality
Alternatives: A, B, C
```

Criteria depend on alternatives and vice versa, with inner dependence inside
each cluster. The scaled supermatrix therefore has cycles, so the limit matrix
uses the iterative calculus method.

**What to notice:** cluster-weight comparisons between Criteria and Alternatives,
feedback judgments from each alternative into the criteria, and that results
differ from a pure hierarchy of the same local scores.

```bash
./build/examples/network23
```

---

## hamburger_std {#hamburger_std}

**Source:** `examples/hamburger_std.cpp`

SuperDecisions sample hamburger market-share ANP network (Creative Decisions
Foundation tutorial tables). Local priority blocks from the published unweighted
supermatrix and cluster-weight matrix are turned back into pairwise ratios
`a_ij = p_i / p_j`.

Clusters include Alternatives (McDonalds, BurgerKing, Wendys), Advertising,
Quality of Food, and Other.

**What to notice:** synthesized alternative priorities near the published
tutorial targets (approximately McDonald's 0.55, Burger King 0.28, Wendy's 0.17),
as a cross-check against SuperDecisions / reference behavior.

```bash
./build/examples/hamburger_std
```

---

## benefits_costs_subnet {#benefits_costs_subnet}

**Source:** `examples/benefits_costs_subnet.cpp`

A Benefits/Costs-style control model with subnetworks (BOCR-like pattern):

```text
Top:      Choose (goal) -> { Benefits, Costs }
Benefits: { Performance, Convenience } -> { Plan1, Plan2, Plan3 }
Costs:    { Money, Risk }              -> { Plan1, Plan2, Plan3 }
```

Each control node owns its own subnetwork that ranks the same alternatives.
Scores are synthesized to the top level, weighted by control-criteria
priorities. The Costs node is marked inverted so higher cost hurts the final
score.

**What to notice:** separate Benefits and Costs subnet results, then the
synthesized top-level alternative vector that mixes both (with inversion on
Costs).

```bash
./build/examples/benefits_costs_subnet
```

---

## ratings_demo {#ratings_demo}

**Source:** `examples/ratings_demo.cpp`

A small hierarchy that uses `RatingsPrioritizer` instead of pairwise under the
criteria:

```text
Goal:         Best  --pairwise-->  { Price, Quality }
Price:        categorical Hi/Med/Low ratings of { A, B, C }
Quality:      numeric scores with DivideByMax of { A, B, C }
```

**What to notice:** Price favors A (High) over C (Low); Quality favors C (100)
over A (40) after divide-by-max; equal criteria weights from Best; L1-normalized
rating columns feed the unscaled supermatrix the same way pairwise priorities do.

```bash
./build/examples/ratings_demo
```
