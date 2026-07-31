#include "anpcpp/limit_matrix.hpp"

#include "anpcpp/eigen.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace anpcpp {
namespace {

[[nodiscard]] double matrix_max_abs(const Matrix& mat) {
  double best = 0.0;
  for (std::size_t i = 0; i < mat.rows(); ++i) {
    for (std::size_t j = 0; j < mat.cols(); ++j) {
      best = std::max(best, std::abs(mat(i, j)));
    }
  }
  return best;
}

[[nodiscard]] Vector column_maxes(const Matrix& mat) {
  Vector m(mat.cols(), 0.0);
  for (std::size_t j = 0; j < mat.cols(); ++j) {
    double best = 0.0;
    for (std::size_t i = 0; i < mat.rows(); ++i) {
      best = std::max(best, std::abs(mat(i, j)));
    }
    m[j] = best == 0.0 ? 1.0 : best;
  }
  return m;
}

// mat^N with N a power of 2 >= power, optionally rescaling by 1/max after each square.
[[nodiscard]] Matrix mat_pow2(const Matrix& mat,
                              std::size_t power,
                              bool rescale) {
  Matrix last = mat;
  Matrix next = mat;
  std::size_t count = 1;
  while (count <= power) {
    next = last * last;
    if (rescale) {
      const double mmax = matrix_max_abs(next);
      if (mmax != 0.0) {
        next *= (1.0 / mmax);
      }
    }
    std::swap(last, next);
    count *= 2;
  }
  return last;
}

[[nodiscard]] std::size_t calculus_start_power(const Matrix& mat) {
  const std::size_t n = mat.rows();
  if (n == 0) {
    return 1;
  }
  // Pick a starting exponent large enough that tiny entries have decayed.
  // Entries below 1/(20n) are treated as "small"; their average sets the scale.
  const double epsilon = 1.0 / (20.0 * static_cast<double>(n));
  std::vector<double> small_entries;
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      const double val = std::abs(mat(i, j));
      if (val < epsilon && val != 0.0) {
        small_entries.push_back(val);
      }
    }
  }
  if (small_entries.empty()) {
    return 20 * n * n + 10;
  }
  double sum = 0.0;
  for (double v : small_entries) {
    sum += v;
  }
  const double avg = sum / static_cast<double>(small_entries.size());
  const double A = 1.0 / avg;
  return static_cast<std::size_t>(A) * n * n;
}

// Max absolute difference after per-column scaling (handles differently
// scaled powers of the same limit matrix during convergence checks).
[[nodiscard]] double normalize_cols_dist(const Matrix& mat1,
                                         const Matrix& mat2) {
  const Vector div1 = column_maxes(mat1);
  const Vector div2 = column_maxes(mat2);
  double worst = 0.0;
  for (std::size_t i = 0; i < mat1.rows(); ++i) {
    for (std::size_t j = 0; j < mat1.cols(); ++j) {
      const double a = mat1(i, j) / div1[j];
      const double b = mat2(i, j) / div2[j];
      worst = std::max(worst, std::abs(a - b));
    }
  }
  return worst;
}

[[nodiscard]] bool column_is_zero(const Matrix& mat, std::size_t col) {
  for (std::size_t row = 0; row < mat.rows(); ++row) {
    if (mat(row, col) != 0.0) {
      return false;
    }
  }
  return true;
}

// pyanp zero_cols: indices of zero (or non-zero) columns.
[[nodiscard]] std::vector<std::size_t> zero_cols(const Matrix& mat,
                                                 bool non_zero) {
  std::vector<std::size_t> zeros;
  std::vector<std::size_t> nonzeros;
  for (std::size_t col = 0; col < mat.cols(); ++col) {
    if (column_is_zero(mat, col)) {
      zeros.push_back(col);
    } else {
      nonzeros.push_back(col);
    }
  }
  return non_zero ? nonzeros : zeros;
}

// Complement of a sorted-or-unsorted index list in [0, n).
[[nodiscard]] std::vector<std::size_t> complement_indices(
    std::size_t n, const std::vector<std::size_t>& chosen) {
  std::vector<bool> picked(n, false);
  for (std::size_t i : chosen) {
    if (i < n) {
      picked[i] = true;
    }
  }
  std::vector<std::size_t> rest;
  rest.reserve(n - chosen.size());
  for (std::size_t i = 0; i < n; ++i) {
    if (!picked[i]) {
      rest.push_back(i);
    }
  }
  return rest;
}

// pyanp two_two_breakdown: split into [A B; C D] with A indexed by upper.
struct TwoTwoBreakdown {
  Matrix A;
  Matrix B;
  Matrix C;
  Matrix D;
  std::vector<std::size_t> upper;
  std::vector<std::size_t> lower;
};

[[nodiscard]] TwoTwoBreakdown two_two_breakdown(
    const Matrix& mat, const std::vector<std::size_t>& upper_right_indices) {
  const std::size_t total_n = mat.rows();
  TwoTwoBreakdown out;
  out.upper = upper_right_indices;
  out.lower = complement_indices(total_n, upper_right_indices);
  const std::size_t upper_n = out.upper.size();
  const std::size_t lower_n = out.lower.size();
  out.A = Matrix(upper_n, upper_n, 0.0);
  out.B = Matrix(upper_n, lower_n, 0.0);
  out.C = Matrix(lower_n, upper_n, 0.0);
  out.D = Matrix(lower_n, lower_n, 0.0);
  for (std::size_t i = 0; i < upper_n; ++i) {
    const std::size_t row = out.upper[i];
    for (std::size_t j = 0; j < upper_n; ++j) {
      out.A(i, j) = mat(row, out.upper[j]);
    }
    for (std::size_t j = 0; j < lower_n; ++j) {
      out.B(i, j) = mat(row, out.lower[j]);
    }
  }
  for (std::size_t i = 0; i < lower_n; ++i) {
    const std::size_t row = out.lower[i];
    for (std::size_t j = 0; j < upper_n; ++j) {
      out.C(i, j) = mat(row, out.upper[j]);
    }
    for (std::size_t j = 0; j < lower_n; ++j) {
      out.D(i, j) = mat(row, out.lower[j]);
    }
  }
  return out;
}

}  // namespace

Matrix column_normalize(const Matrix& mat) {
  Matrix out = mat;
  column_normalize_inplace(out);
  return out;
}

void column_normalize_inplace(Matrix& mat) {
  for (std::size_t j = 0; j < mat.cols(); ++j) {
    double total = 0.0;
    for (std::size_t i = 0; i < mat.rows(); ++i) {
      total += mat(i, j);
    }
    if (total == 0.0) {
      continue;
    }
    for (std::size_t i = 0; i < mat.rows(); ++i) {
      mat(i, j) /= total;
    }
  }
}

Matrix hierarchy_formula(const Matrix& mat) {
  if (mat.rows() != mat.cols()) {
    throw DimensionError("hierarchy_formula requires a square matrix");
  }
  const std::size_t size = mat.rows();
  if (size == 0) {
    return Matrix{};
  }

  // A hierarchy has W^{n+1} = 0 (no cyclic influence). If not, bail out.
  const Matrix big = mat_pow2(mat, size + 1, false);
  if (matrix_max_abs(big) != 0.0) {
    return Matrix{};  // not a hierarchy
  }

  // Closed form: L = W + W² + … + W^{n-1}, column-normalized.
  Matrix summ = mat;
  Matrix thispow = mat;
  for (std::size_t i = 0; i + 2 < size; ++i) {
    const Matrix nextpow = thispow * mat;
    summ += nextpow;
    thispow = nextpow;
  }
  return column_normalize(summ);
}

std::vector<std::size_t> hierarchy_nodes(const Matrix& mat) {
  if (mat.rows() != mat.cols()) {
    throw DimensionError("hierarchy_nodes requires a square matrix");
  }
  if (mat.empty()) {
    return {};
  }
  const Matrix powered = mat_pow2(mat, mat.rows(), false);
  return zero_cols(powered, false);
}

Matrix calculus_limit(const Matrix& mat, const LimitMatrixOptions& options) {
  if (mat.rows() != mat.cols()) {
    throw DimensionError("calculus_limit requires a square matrix");
  }
  const std::size_t size = mat.rows();
  if (size == 0) {
    return Matrix{};
  }

  std::size_t start_pow = options.start_pow;
  if (start_pow == 0) {
    start_pow = calculus_start_power(mat);
  }

  // Jump to W^start_pow via repeated squaring; rescale after each square to
  // avoid overflow on large exponents.
  Matrix start = mat_pow2(mat, start_pow, true);
  if (options.use_hierarchy_formula && matrix_max_abs(start) == 0.0) {
    // W^start may underflow to zero for hierarchical matrices; try W^n and
    // fall back to the closed-form hierarchy sum when applicable.
    start = mat_pow2(mat, size, false);
    if (matrix_max_abs(start) == 0.0) {
      const Matrix hier = hierarchy_formula(mat);
      if (!hier.empty()) {
        return hier;
      }
    }
  }

  // Sliding window of successive powers W^k … W^{k+n-1}; stop when two
  // consecutive powers agree column-wise within tolerance.
  std::vector<Matrix> pows;
  pows.reserve(size);
  pows.push_back(start);
  for (std::size_t i = 0; i + 1 < size; ++i) {
    pows.push_back(mat * pows.back());
    const double diff =
        normalize_cols_dist(pows[pows.size() - 1], pows[pows.size() - 2]);
    if (diff < options.error) {
      return column_normalize(pows.back());
    }
  }

  // Advance the window: drop the oldest power, append W * newest.
  for (std::size_t count = 0; count < options.max_iters; ++count) {
    Matrix nextp = pows.back() * mat;
    for (std::size_t i = 0; i + 1 < pows.size(); ++i) {
      pows[i] = std::move(pows[i + 1]);
    }
    pows.back() = nextp;

    bool converged = false;
    for (std::size_t i = 0; i + 1 < pows.size(); ++i) {
      if (normalize_cols_dist(pows[i], pows.back()) < options.error) {
        converged = true;
        break;
      }
    }
    if (converged) {
      return column_normalize(pows.back());
    }
  }

  throw ConvergenceError(
      "calculus_limit: did not converge within the iteration limit");
}

Matrix limit_sinks(const Matrix& mat, const LimitMatrixOptions& options) {
  if (mat.rows() != mat.cols()) {
    throw DimensionError("limit_sinks requires a square matrix");
  }
  const std::size_t n = mat.rows();
  if (n == 0) {
    return Matrix{};
  }

  const std::vector<std::size_t> nonsinks = zero_cols(mat, true);
  const std::vector<std::size_t> sinks = zero_cols(mat, false);
  if (nonsinks.size() == n) {
    // No sinks — fall back to calculus.
    return calculus_limit(mat, options);
  }
  if (nonsinks.empty()) {
    return Matrix(n, n, 0.0);
  }

  // pyanp: (B, z1, A, z2) = two_two_breakdown(mat, nonsinks)
  // B = nonsink×nonsink, A = sink×nonsink.
  const TwoTwoBreakdown parts = two_two_breakdown(mat, nonsinks);
  Matrix limitB = calculus_limit(parts.A, options);
  if (!options.straight_normalizer) {
    limitB = column_normalize(limitB);
  }
  const Matrix axblimit = parts.C * limitB;

  Matrix rval(n, n, 0.0);
  for (std::size_t i = 0; i < nonsinks.size(); ++i) {
    const std::size_t orig_row = nonsinks[i];
    for (std::size_t j = 0; j < nonsinks.size(); ++j) {
      rval(orig_row, nonsinks[j]) = limitB(i, j);
    }
  }
  for (std::size_t i = 0; i < sinks.size(); ++i) {
    const std::size_t orig_row = sinks[i];
    for (std::size_t j = 0; j < nonsinks.size(); ++j) {
      rval(orig_row, nonsinks[j]) = axblimit(i, j);
    }
  }
  if (options.straight_normalizer) {
    column_normalize_inplace(rval);
  }
  return rval;
}

Matrix limit_newhierarchy(const Matrix& mat,
                          const LimitMatrixOptions& options) {
  if (mat.rows() != mat.cols()) {
    throw DimensionError("limit_newhierarchy requires a square matrix");
  }
  const std::size_t n = mat.rows();
  if (n == 0) {
    return Matrix{};
  }

  const std::vector<std::size_t> hier_nodes = hierarchy_nodes(mat);
  const std::vector<std::size_t> net_nodes = complement_indices(n, hier_nodes);
  if (net_nodes.size() == n) {
    return calculus_limit(mat, options);
  }
  if (hier_nodes.size() == n) {
    const Matrix hier = hierarchy_formula(mat);
    if (!hier.empty()) {
      return hier;
    }
    return calculus_limit(mat, options);
  }

  // pyanp: (B, z1, A, C) = two_two_breakdown(mat, net_nodes)
  // B = net×net, A = hier×net, C = hier×hier.
  const TwoTwoBreakdown parts = two_two_breakdown(mat, net_nodes);
  const Matrix& B = parts.A;
  const Matrix& A = parts.C;
  const Matrix& C = parts.D;

  const Matrix limitB = calculus_limit(B, options);
  const Matrix limitC = calculus_limit(C, options);

  Matrix lower_left = A * limitB + C * A;
  column_normalize_inplace(lower_left);

  if (options.with_limit) {
    Matrix laststep = lower_left;
    Matrix nextstep = lower_left;
    double diff = 1.0;
    for (std::size_t count = 0;
         diff > options.error && count < options.max_count; ++count) {
      // Matches pyanp: A·limit(B) + limit(C)·A (fixed point after one step).
      nextstep = A * limitB + limitC * A;
      diff = normalize_cols_dist(laststep, nextstep);
      laststep = nextstep;
    }
    lower_left = nextstep;
  }

  Matrix rval(n, n, 0.0);
  for (std::size_t i = 0; i < net_nodes.size(); ++i) {
    const std::size_t orig_row = net_nodes[i];
    for (std::size_t j = 0; j < net_nodes.size(); ++j) {
      rval(orig_row, net_nodes[j]) = limitB(i, j);
    }
  }
  for (std::size_t i = 0; i < hier_nodes.size(); ++i) {
    const std::size_t orig_row = hier_nodes[i];
    for (std::size_t j = 0; j < net_nodes.size(); ++j) {
      rval(orig_row, net_nodes[j]) = lower_left(i, j);
    }
    for (std::size_t j = 0; j < hier_nodes.size(); ++j) {
      rval(orig_row, hier_nodes[j]) = C(i, j);
    }
  }
  return column_normalize(rval);
}

Matrix compute_limit_matrix(const Matrix& mat,
                            const LimitMatrixOptions& options) {
  switch (options.method) {
    case LimitMatrixMethod::NewHierarchy:
      return limit_newhierarchy(mat, options);
    case LimitMatrixMethod::Sinks:
      return limit_sinks(mat, options);
    case LimitMatrixMethod::Calculus:
    default:
      return calculus_limit(mat, options);
  }
}

Vector priority_from_limit(const Matrix& limit_matrix) {
  Vector rval = limit_matrix.row_sums();
  const double total = rval.sum();
  if (total != 0.0) {
    rval.normalize();
  }
  return rval;
}

}  // namespace anpcpp
