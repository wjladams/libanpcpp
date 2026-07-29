/**
 * @file eigen.hpp
 * @brief Principal eigenvector computation for pairwise matrices.
 */

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

#include "cppanp/matrix.hpp"

namespace cppanp {

/**
 * @brief Thrown when power iteration fails to converge within max_iterations.
 */
class ConvergenceError : public std::runtime_error {
public:
  /**
   * @param message Description of the failure.
   */
  explicit ConvergenceError(const std::string& message)
      : std::runtime_error(message) {}
};

/**
 * @brief Applies Harker's fix for incomplete pairwise matrices.
 *
 * Each row diagonal becomes @c 1 + (number of zero off-diagonal entries in
 * that row), enabling power iteration on incomplete comparisons.
 *
 * @param mat Square pairwise matrix.
 * @return Adjusted matrix.
 */
[[nodiscard]] Matrix harker_fix(const Matrix& mat);

/**
 * @brief Options controlling principal eigen computation.
 */
struct EigenOptions {
  /** Stop when max-norm change between iterates is <= error. */
  double error = 1e-10;
  /** Apply @ref harker_fix before iteration when true. */
  bool use_harker = false;
  /** Maximum power-iteration steps (pyanp has no cap). */
  std::size_t max_iterations = 10000;
};

/**
 * @brief Result of @ref principal_eigen.
 */
struct EigenResult {
  /** Priority vector (components sum to 1). */
  Vector vector;
  /** Dominant eigenvalue estimate. */
  double value = 0.0;
  /** Number of iterations performed. */
  std::size_t iterations = 0;
  /** False if max_iterations reached before convergence. */
  bool converged = false;
};

/**
 * @brief Power iteration with sum-normalization (pyanp pri_eigen compatible).
 *
 * Non-convergence is reported via @c converged == false rather than throwing.
 *
 * @param mat Square matrix.
 * @param options Iteration and Harker options.
 * @return Eigenvector, eigenvalue estimate, and convergence metadata.
 */
[[nodiscard]] EigenResult principal_eigen(const Matrix& mat,
                                          const EigenOptions& options = {});

/**
 * @brief Returns the principal eigenvector only.
 * @throws ConvergenceError if iteration does not converge.
 */
[[nodiscard]] Vector principal_eigenvector(const Matrix& mat,
                                           const EigenOptions& options = {});

/**
 * @brief Returns the principal eigenvalue only.
 * @throws ConvergenceError if iteration does not converge.
 */
[[nodiscard]] double principal_eigenvalue(const Matrix& mat,
                                          const EigenOptions& options = {});

}  // namespace cppanp
