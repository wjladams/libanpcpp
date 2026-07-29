/**
 * @file inconsistency.hpp
 * @brief Saaty consistency index and ratio for pairwise matrices.
 */

#pragma once

#include <cstddef>

#include "anpcpp/eigen.hpp"
#include "anpcpp/matrix.hpp"

namespace anpcpp {

/**
 * @brief Saaty random index (RI) for matrix order @p n.
 *
 * @p n = 1,2: 0 (CR defined as 0). @p n = 3..15: Saaty table. @p n > 15:
 * Alonso-Lamata approximation RI ≈ 1.98 * (n - 2) / n.
 */
[[nodiscard]] double random_index(std::size_t n);

/**
 * @brief Consistency index CI = (lambda_max - n) / (n - 1).
 * @return 0 for n < 2.
 */
[[nodiscard]] double consistency_index(double lambda_max, std::size_t n);

/**
 * @brief Consistency ratio CR = CI / RI.
 * @return 0 when RI == 0 (n <= 2).
 */
[[nodiscard]] double consistency_ratio(double lambda_max, std::size_t n);

/**
 * @brief Options for @ref consistency.
 */
struct ConsistencyOptions {
  /** Eigen iteration options. */
  EigenOptions eigen;
  /** Apply Harker fix before eigen (pyanp incon_std default). */
  bool use_harker = true;
};

/**
 * @brief Full consistency analysis result.
 */
struct ConsistencyResult {
  double lambda_max = 0.0;
  double consistency_index = 0.0;
  double consistency_ratio = 0.0;
  double random_index = 0.0;
  /** Normalized priority vector. */
  Vector priority;
  std::size_t iterations = 0;
  bool converged = false;
};

/**
 * @brief Computes Saaty CI and CR for a pairwise comparison matrix.
 * @param mat Square comparison matrix.
 * @param options Harker and eigen options.
 */
[[nodiscard]] ConsistencyResult consistency(const Matrix& mat,
                                            ConsistencyOptions options = {});

/**
 * @brief Consistency index from a matrix (throws on non-convergence).
 * @param mat Square comparison matrix.
 * @param options Harker and eigen options.
 */
[[nodiscard]] double consistency_index(const Matrix& mat,
                                       ConsistencyOptions options = {});

/**
 * @brief Consistency ratio from a matrix (throws on non-convergence).
 * @param mat Square comparison matrix.
 * @param options Harker and eigen options.
 */
[[nodiscard]] double consistency_ratio(const Matrix& mat,
                                       ConsistencyOptions options = {});

}  // namespace anpcpp
