/**
 * @file limit_matrix.hpp
 * @brief Limit matrix and priority extraction from supermatrices.
 */

#pragma once

#include <cstddef>

#include "anpcpp/matrix.hpp"

namespace anpcpp {

/**
 * @brief Options for limit-matrix calculation.
 */
struct LimitMatrixOptions {
  /** Convergence tolerance for calculus iteration. */
  double error = 1e-10;
  /** Maximum calculus iterations. */
  std::size_t max_iters = 5000;
  /** Use hierarchy shortcut when the matrix is a strict hierarchy. */
  bool use_hierarchy_formula = true;
  /** Starting power; 0 means auto-detect (pyanp default). */
  std::size_t start_pow = 0;
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
 * @brief SuperDecisions / pyanp calculus limit matrix.
 * @param mat Column-stochastic supermatrix.
 * @param options Iteration and hierarchy options.
 */
[[nodiscard]] Matrix calculus_limit(const Matrix& mat,
                                    const LimitMatrixOptions& options = {});

/**
 * @brief Row sums of the limit matrix, L1-normalized (pyanp priority_from_limit).
 */
[[nodiscard]] Vector priority_from_limit(const Matrix& limit_matrix);

}  // namespace anpcpp
