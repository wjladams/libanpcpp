/**
 * @file limit_matrix.hpp
 * @brief Limit matrix and priority extraction from supermatrices.
 */

#pragma once

#include <cstddef>
#include <vector>

#include "anpcpp/matrix.hpp"

namespace anpcpp {

/**
 * @brief SuperDecisions / pyanp limit-matrix algorithm.
 */
enum class LimitMatrixMethod {
  /** Calculus Type (default SuperDecisions / pyanp). */
  Calculus,
  /** New Hierarchy decomposition (pyanp limit_newhierarchy). */
  NewHierarchy,
  /** Limit with sinks decomposition (pyanp limit_sinks). */
  Sinks,
};

/**
 * @brief Options for limit-matrix calculation.
 */
struct LimitMatrixOptions {
  /** Which limit-matrix algorithm to use. */
  LimitMatrixMethod method = LimitMatrixMethod::Calculus;
  /** Convergence tolerance for calculus / new-hierarchy iteration. */
  double error = 1e-10;
  /** Maximum calculus iterations. */
  std::size_t max_iters = 5000;
  /** Use hierarchy shortcut when the matrix is a strict hierarchy. */
  bool use_hierarchy_formula = true;
  /** Starting power; 0 means auto-detect (pyanp default). */
  std::size_t start_pow = 0;
  /**
   * New Hierarchy only: if true, replace the lower-left corner with
   * A·limit(B) + limit(C)·A (pyanp with_limit).
   */
  bool with_limit = false;
  /** New Hierarchy only: max iterations for the with_limit refinement. */
  std::size_t max_count = 1000;
  /**
   * Sinks only: if true, column-normalize the assembled result; if false,
   * normalize the nonsink block only (pyanp straight_normalizer).
   */
  bool straight_normalizer = true;
};

/**
 * @brief Column-normalizes each column by its sum (pyanp normalize).
 * @return Normalized copy; zero columns remain zero.
 */
[[nodiscard]] Matrix column_normalize(const Matrix& mat);

/** @brief In-place @ref column_normalize. @param mat Matrix to normalize. */
void column_normalize_inplace(Matrix& mat);

/**
 * @brief Hierarchy limit formula: normalize(sum of W^k for k=1..n-1).
 * @return Empty matrix if @p mat is not a hierarchy.
 */
[[nodiscard]] Matrix hierarchy_formula(const Matrix& mat);

/**
 * @brief Indices of hierarchical nodes (zero columns of W^n); pyanp hierarchy_nodes.
 */
[[nodiscard]] std::vector<std::size_t> hierarchy_nodes(const Matrix& mat);

/**
 * @brief SuperDecisions / pyanp calculus limit matrix.
 * @param mat Column-stochastic supermatrix.
 * @param options Iteration and hierarchy options.
 */
[[nodiscard]] Matrix calculus_limit(const Matrix& mat,
                                    const LimitMatrixOptions& options = {});

/**
 * @brief Limit-with-sinks calculation (pyanp limit_sinks).
 *
 * Splits sinks (zero columns) from nonsinks, applies calculus on the nonsink
 * block, and reassembles.
 */
[[nodiscard]] Matrix limit_sinks(const Matrix& mat,
                                 const LimitMatrixOptions& options = {});

/**
 * @brief New Hierarchy limit calculation (pyanp limit_newhierarchy).
 *
 * Splits hierarchical vs network nodes, limits each block, and reassembles.
 */
[[nodiscard]] Matrix limit_newhierarchy(const Matrix& mat,
                                        const LimitMatrixOptions& options = {});

/**
 * @brief Dispatch to the algorithm selected by @p options.method.
 */
[[nodiscard]] Matrix compute_limit_matrix(
    const Matrix& mat, const LimitMatrixOptions& options = {});

/**
 * @brief Row sums of the limit matrix, L1-normalized (pyanp priority_from_limit).
 */
[[nodiscard]] Vector priority_from_limit(const Matrix& limit_matrix);

}  // namespace anpcpp
