#include "anpcpp/rowsens.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace anpcpp {
namespace {

std::pair<bool, double> p_to_scalar(double p, double p0) {
  if (p < p0) {
    return {false, p / p0};
  }
  return {true, (1.0 - p) / (1.0 - p0)};
}

std::vector<std::size_t> default_cluster(std::size_t n,
                                         const std::vector<std::size_t>& cluster) {
  if (!cluster.empty()) return cluster;
  std::vector<std::size_t> all(n);
  for (std::size_t i = 0; i < n; ++i) all[i] = i;
  return all;
}

std::vector<std::size_t> default_influence(
    std::size_t n,
    std::size_t row,
    const std::vector<std::size_t>& influence) {
  if (!influence.empty()) return influence;
  std::vector<std::size_t> out;
  out.reserve(n > 0 ? n - 1 : 0);
  for (std::size_t i = 0; i < n; ++i) {
    if (i != row) out.push_back(i);
  }
  return out;
}

double cluster_abs_sum(const Matrix& mat,
                       std::size_t col,
                       const std::vector<std::size_t>& cluster) {
  double total = 0.0;
  for (std::size_t r : cluster) {
    total += std::abs(mat(r, col));
  }
  return total;
}

void scale_cluster_column(Matrix& mat,
                          std::size_t col,
                          const std::vector<std::size_t>& cluster,
                          double factor) {
  for (std::size_t r : cluster) {
    mat(r, col) *= factor;
  }
}

Vector extract_nodes(const Vector& pri,
                     const std::vector<std::size_t>& idxs) {
  Vector out(idxs.size());
  for (std::size_t i = 0; i < idxs.size(); ++i) {
    out[i] = pri[idxs[i]];
  }
  return out;
}

void normalize_l1_inplace(Vector& v) {
  double s = 0.0;
  for (std::size_t i = 0; i < v.size(); ++i) s += std::abs(v[i]);
  if (s == 0.0) return;
  for (std::size_t i = 0; i < v.size(); ++i) v[i] /= s;
}

Vector ranks_average(const Vector& values) {
  // Average ranks for ties (scipy.stats.rankdata default method='average').
  const std::size_t n = values.size();
  std::vector<std::size_t> order(n);
  for (std::size_t i = 0; i < n; ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return values[a] < values[b];
  });
  Vector ranks(n, 0.0);
  std::size_t i = 0;
  while (i < n) {
    std::size_t j = i + 1;
    while (j < n && values[order[j]] == values[order[i]]) ++j;
    const double avg = 0.5 * static_cast<double>(i + j + 1);
    for (std::size_t k = i; k < j; ++k) {
      ranks[order[k]] = avg;
    }
    i = j;
  }
  return ranks;
}

bool rank_change(const Vector& a,
                 const Vector& b,
                 const std::vector<std::size_t>& places,
                 int round_to_decimal) {
  const double scale = std::pow(10.0, round_to_decimal);
  Vector ra(places.size());
  Vector rb(places.size());
  for (std::size_t i = 0; i < places.size(); ++i) {
    ra[i] = std::round(a[places[i]] * scale) / scale;
    rb[i] = std::round(b[places[i]] * scale) / scale;
  }
  const Vector rka = ranks_average(ra);
  const Vector rkb = ranks_average(rb);
  for (std::size_t i = 0; i < places.size(); ++i) {
    if (rka[i] != rkb[i]) return true;
  }
  return false;
}

double calc_p0(const Matrix& mat,
               std::size_t row,
               const std::vector<std::size_t>& cluster,
               double orig,
               const P0Mode& p0mode,
               const LimitMatrixOptions& options) {
  if (p0mode.kind == P0ModeKind::Direct) {
    return p0mode.direct;
  }
  if (p0mode.kind == P0ModeKind::Smart) {
    const Vector left =
        influence_marginal(mat, row, 0.5, -1, 1e-6, {}, cluster, options);
    const Vector right =
        influence_marginal(mat, row, 0.5, +1, 1e-6, {}, cluster, options);
    const double lval = left[p0mode.smart_alt];
    const double rval = right[p0mode.smart_alt];
    const double denom = lval + rval;
    if (denom == 0.0) return 0.5;
    return lval / denom;
  }
  return orig;
}

}  // namespace

Matrix row_adjust(const Matrix& mat,
                  std::size_t row,
                  double p,
                  const P0Mode& p0mode,
                  const std::vector<std::size_t>& cluster_nodes) {
  if (mat.rows() != mat.cols()) {
    throw DimensionError("row_adjust requires a square matrix");
  }
  const std::size_t n = mat.rows();
  if (row >= n) {
    throw std::out_of_range("row_adjust row out of range");
  }
  Matrix out = mat;
  const auto cluster = default_cluster(n, cluster_nodes);
  if (std::find(cluster.begin(), cluster.end(), row) == cluster.end()) {
    throw std::invalid_argument("row was not in cluster_nodes");
  }

  LimitMatrixOptions opts;  // used only for Smart p0 recursion
  for (std::size_t c = 0; c < n; ++c) {
    const double total = cluster_abs_sum(out, c, cluster);
    if (total != 0.0) {
      scale_cluster_column(out, c, cluster, 1.0 / total);
    }
    const double orig = out(row, c);
    if (orig != 0.0 && orig != 1.0) {
      const double p0 = calc_p0(out, row, cluster, orig, p0mode, opts);
      const auto [scale_up, scalar] = p_to_scalar(p, p0);
      if (!scale_up) {
        out(row, c) *= scalar;
        for (std::size_t r : cluster) {
          if (r == row) continue;
          out(r, c) *= (1.0 - out(row, c)) / (1.0 - orig);
        }
      } else {
        out(row, c) = 1.0 - scalar * (1.0 - out(row, c));
        for (std::size_t r : cluster) {
          if (r == row) continue;
          out(r, c) *= scalar;
        }
      }
    }
    if (total != 0.0) {
      scale_cluster_column(out, c, cluster, total);
    }
  }
  return out;
}

double smart_p0(const Matrix& mat,
                std::size_t row,
                std::size_t cont_alt,
                const std::vector<std::size_t>& cluster_nodes,
                const LimitMatrixOptions& options) {
  return calc_p0(mat, row, default_cluster(mat.rows(), cluster_nodes), 0.5,
                 P0Mode::Smart(cont_alt), options);
}

Vector priority_after_row_adjust(const Matrix& mat,
                                 std::size_t row,
                                 double p,
                                 const P0Mode& p0mode,
                                 const std::vector<std::size_t>& cluster_nodes,
                                 const LimitMatrixOptions& options,
                                 bool normalize_to_orig) {
  double old_sum = 1.0;
  double old_val = 0.0;
  if (normalize_to_orig) {
    const Vector old_pri = priority_from_limit(compute_limit_matrix(mat, options));
    old_val = old_pri[row];
    old_sum = 0.0;
    for (std::size_t i = 0; i < old_pri.size(); ++i) {
      if (i != row) old_sum += old_pri[i];
    }
  }
  const Matrix adjusted = row_adjust(mat, row, p, p0mode, cluster_nodes);
  Vector new_pri = priority_from_limit(compute_limit_matrix(adjusted, options));
  if (normalize_to_orig) {
    const double row_pri = new_pri[row];
    new_pri[row] = 0.0;
    double s = 0.0;
    for (std::size_t i = 0; i < new_pri.size(); ++i) s += new_pri[i];
    if (s != 0.0) {
      for (std::size_t i = 0; i < new_pri.size(); ++i) {
        new_pri[i] *= old_sum / s;
      }
    }
    new_pri[row] = row_pri;
    (void)old_val;
  }
  return new_pri;
}

Vector influence_marginal(const Matrix& mat,
                          std::size_t row,
                          double p0,
                          int left_or_right,
                          double delta,
                          const std::vector<std::size_t>& influence_nodes,
                          const std::vector<std::size_t>& cluster_nodes,
                          const LimitMatrixOptions& options) {
  const std::size_t n = mat.rows();
  const auto nodes = influence_nodes.empty()
                         ? [&] {
                             std::vector<std::size_t> all(n);
                             for (std::size_t i = 0; i < n; ++i) all[i] = i;
                             return all;
                           }()
                         : influence_nodes;

  Vector orig_pri =
      extract_nodes(priority_from_limit(compute_limit_matrix(mat, options)), nodes);
  normalize_l1_inplace(orig_pri);

  Vector left_deriv;
  Vector right_deriv;
  const P0Mode mode = P0Mode::Direct(p0);

  if (left_or_right <= 0) {
    const Matrix adj =
        row_adjust(mat, row, p0 - delta, mode, cluster_nodes);
    Vector pri =
        extract_nodes(priority_from_limit(compute_limit_matrix(adj, options)), nodes);
    normalize_l1_inplace(pri);
    left_deriv = Vector(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      left_deriv[i] = (pri[i] - orig_pri[i]) / -delta;
    }
    if (left_or_right < 0) {
      // Expand to full n-vector for smart_p0 indexing convenience.
      Vector full(n, 0.0);
      for (std::size_t i = 0; i < nodes.size(); ++i) {
        full[nodes[i]] = left_deriv[i];
      }
      if (influence_nodes.empty()) return full;
      return left_deriv;
    }
  }
  if (left_or_right >= 0) {
    const Matrix adj =
        row_adjust(mat, row, p0 + delta, mode, cluster_nodes);
    Vector pri =
        extract_nodes(priority_from_limit(compute_limit_matrix(adj, options)), nodes);
    normalize_l1_inplace(pri);
    right_deriv = Vector(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      right_deriv[i] = (pri[i] - orig_pri[i]) / delta;
    }
    if (left_or_right > 0) {
      Vector full(n, 0.0);
      for (std::size_t i = 0; i < nodes.size(); ++i) {
        full[nodes[i]] = right_deriv[i];
      }
      if (influence_nodes.empty()) return full;
      return right_deriv;
    }
  }

  Vector avg(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    avg[i] = 0.5 * (left_deriv[i] + right_deriv[i]);
  }
  if (influence_nodes.empty()) {
    Vector full(n, 0.0);
    for (std::size_t i = 0; i < nodes.size(); ++i) full[nodes[i]] = avg[i];
    return full;
  }
  return avg;
}

std::vector<InfluenceMarginalEntry> influence_marginal_smart(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    const std::vector<std::string>& names,
    const std::vector<std::size_t>& cluster_nodes,
    double delta,
    const LimitMatrixOptions& options) {
  const auto nodes = default_influence(mat.rows(), row, influence_nodes);
  if (names.size() != nodes.size()) {
    throw std::invalid_argument("influence name count mismatch");
  }
  std::vector<InfluenceMarginalEntry> out;
  out.reserve(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const std::size_t alt = nodes[i];
    const double p0 = smart_p0(mat, row, alt, cluster_nodes, options);
    // At smart p0, left ≈ right; average is the continuous derivative.
    const Vector marg =
        influence_marginal(mat, row, p0, 0, delta, {alt}, cluster_nodes, options);
    InfluenceMarginalEntry e;
    e.name = names[i];
    e.smart_p0 = p0;
    e.marginal = marg.empty() ? 0.0 : marg[0];
    out.push_back(std::move(e));
  }
  return out;
}

std::vector<InfluenceRawEntry> influence_raw(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    const std::vector<std::string>& names,
    double delta_up,
    double delta_down,
    double p0,
    const std::vector<std::size_t>& cluster_nodes,
    const LimitMatrixOptions& options) {
  const auto nodes = default_influence(mat.rows(), row, influence_nodes);
  if (names.size() != nodes.size()) {
    throw std::invalid_argument("influence name count mismatch");
  }
  const P0Mode mode = P0Mode::Direct(p0);
  const Vector orig_full =
      priority_after_row_adjust(mat, row, p0, mode, cluster_nodes, options);
  const double p_up = std::min(1.0, p0 + delta_up);
  const double p_down = std::max(0.0, p0 - delta_down);
  const Vector up_full =
      priority_after_row_adjust(mat, row, p_up, mode, cluster_nodes, options);
  const Vector down_full =
      priority_after_row_adjust(mat, row, p_down, mode, cluster_nodes, options);

  // Renormalize influence-node scores to sum to 1 for reporting.
  auto slice_norm = [&](const Vector& full) {
    Vector v = extract_nodes(full, nodes);
    normalize_l1_inplace(v);
    return v;
  };
  const Vector orig = slice_norm(orig_full);
  const Vector up = slice_norm(up_full);
  const Vector down = slice_norm(down_full);

  std::vector<InfluenceRawEntry> out;
  out.reserve(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    InfluenceRawEntry e;
    e.name = names[i];
    e.original = orig[i];
    e.up_score = up[i];
    e.up_diff = up[i] - orig[i];
    e.down_score = down[i];
    e.down_diff = down[i] - orig[i];
    out.push_back(std::move(e));
  }
  return out;
}

std::vector<InfluenceRankEntry> influence_rank(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    const std::vector<std::string>& names,
    const std::vector<std::size_t>& cluster_nodes,
    double error,
    int round_to_decimal,
    const LimitMatrixOptions& options) {
  const auto nodes = default_influence(mat.rows(), row, influence_nodes);
  if (names.size() != nodes.size()) {
    throw std::invalid_argument("influence name count mismatch");
  }
  const P0Mode mode = P0Mode::Direct(0.5);
  const Vector orig_full =
      priority_after_row_adjust(mat, row, 0.5, mode, cluster_nodes, options);
  Vector orig = extract_nodes(orig_full, nodes);
  normalize_l1_inplace(orig);

  // Search for rank change among influence nodes as a group, then score each
  // alt by the max of upper/lower distance-to-change (pyanp both-sides max).
  auto search_upper = [&]() -> double {
    double lower = 0.5;
    double upper = 0.99999;
    Vector lower_pri =
        priority_after_row_adjust(mat, row, lower, mode, cluster_nodes, options);
    Vector upper_pri =
        priority_after_row_adjust(mat, row, upper, mode, cluster_nodes, options);
    if (!rank_change(lower_pri, upper_pri, nodes, round_to_decimal)) {
      return 1.0;
    }
    while ((upper - lower) > error) {
      const double mid = 0.5 * (upper + lower);
      Vector mid_pri =
          priority_after_row_adjust(mat, row, mid, mode, cluster_nodes, options);
      if (rank_change(lower_pri, mid_pri, nodes, round_to_decimal)) {
        upper = mid;
        upper_pri = std::move(mid_pri);
      } else if (rank_change(mid_pri, upper_pri, nodes, round_to_decimal)) {
        lower = mid;
        lower_pri = std::move(mid_pri);
      } else {
        break;
      }
    }
    return (1.0 - upper) / (1.0 - 0.5);
  };

  auto search_lower = [&]() -> double {
    double lower = 0.00001;
    double upper = 0.5;
    Vector lower_pri =
        priority_after_row_adjust(mat, row, lower, mode, cluster_nodes, options);
    Vector upper_pri =
        priority_after_row_adjust(mat, row, upper, mode, cluster_nodes, options);
    if (!rank_change(lower_pri, upper_pri, nodes, round_to_decimal)) {
      return 0.0;
    }
    while ((upper - lower) > error) {
      const double mid = 0.5 * (upper + lower);
      Vector mid_pri =
          priority_after_row_adjust(mat, row, mid, mode, cluster_nodes, options);
      if (rank_change(lower_pri, mid_pri, nodes, round_to_decimal)) {
        upper = mid;
        upper_pri = std::move(mid_pri);
      } else if (rank_change(mid_pri, upper_pri, nodes, round_to_decimal)) {
        lower = mid;
        lower_pri = std::move(mid_pri);
      } else {
        break;
      }
    }
    return lower / 0.5;
  };

  // Per-alt rank influence: search rank change that involves that alt.
  std::vector<InfluenceRankEntry> out;
  out.reserve(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const std::vector<std::size_t> one = {nodes[i]};
    auto search_one_upper = [&]() -> double {
      double lower = 0.5;
      double upper = 0.99999;
      Vector lower_pri = priority_after_row_adjust(mat, row, lower, mode,
                                                   cluster_nodes, options);
      Vector upper_pri = priority_after_row_adjust(mat, row, upper, mode,
                                                   cluster_nodes, options);
      if (!rank_change(lower_pri, upper_pri, nodes, round_to_decimal) ||
          !rank_change(lower_pri, upper_pri, one, round_to_decimal)) {
        // Fall back: any group change score from shared search.
        return search_upper();
      }
      while ((upper - lower) > error) {
        const double mid = 0.5 * (upper + lower);
        Vector mid_pri = priority_after_row_adjust(mat, row, mid, mode,
                                                   cluster_nodes, options);
        if (rank_change(lower_pri, mid_pri, one, round_to_decimal)) {
          upper = mid;
          upper_pri = std::move(mid_pri);
        } else if (rank_change(mid_pri, upper_pri, one, round_to_decimal)) {
          lower = mid;
          lower_pri = std::move(mid_pri);
        } else {
          break;
        }
      }
      return (1.0 - upper) / 0.5;
    };
    auto search_one_lower = [&]() -> double {
      double lower = 0.00001;
      double upper = 0.5;
      Vector lower_pri = priority_after_row_adjust(mat, row, lower, mode,
                                                   cluster_nodes, options);
      Vector upper_pri = priority_after_row_adjust(mat, row, upper, mode,
                                                   cluster_nodes, options);
      if (!rank_change(lower_pri, upper_pri, one, round_to_decimal)) {
        return search_lower();
      }
      while ((upper - lower) > error) {
        const double mid = 0.5 * (upper + lower);
        Vector mid_pri = priority_after_row_adjust(mat, row, mid, mode,
                                                   cluster_nodes, options);
        if (rank_change(lower_pri, mid_pri, one, round_to_decimal)) {
          upper = mid;
          upper_pri = std::move(mid_pri);
        } else if (rank_change(mid_pri, upper_pri, one, round_to_decimal)) {
          lower = mid;
          lower_pri = std::move(mid_pri);
        } else {
          break;
        }
      }
      return lower / 0.5;
    };

    InfluenceRankEntry e;
    e.name = names[i];
    e.original = orig[i];
    e.rank_influence = std::max(search_one_upper(), search_one_lower());
    out.push_back(std::move(e));
  }
  return out;
}

InfluenceTotalEntry influence_total_row(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    double delta,
    double p0,
    const std::vector<std::size_t>& cluster_nodes,
    const LimitMatrixOptions& options) {
  const auto nodes = default_influence(mat.rows(), row, influence_nodes);
  const P0Mode mode = P0Mode::Direct(p0);
  const Vector old_full =
      priority_after_row_adjust(mat, row, p0, mode, cluster_nodes, options);
  const double p_new = std::min(1.0, p0 + delta);
  const Vector new_full =
      priority_after_row_adjust(mat, row, p_new, mode, cluster_nodes, options);

  Vector old_v = extract_nodes(old_full, nodes);
  Vector new_v = extract_nodes(new_full, nodes);
  normalize_l1_inplace(old_v);
  normalize_l1_inplace(new_v);

  InfluenceTotalEntry e;
  e.total_influence = 0.0;
  e.max_alt_change = 0.0;
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const double d = std::abs(new_v[i] - old_v[i]);
    e.total_influence += d;
    if (d > e.max_alt_change) e.max_alt_change = d;
  }
  return e;
}

std::vector<InfluenceTotalEntry> influence_total(
    const Matrix& mat,
    const std::vector<std::size_t>& rows,
    const std::vector<std::string>& row_names,
    const std::vector<std::size_t>& influence_nodes,
    double delta,
    double p0,
    const std::vector<std::size_t>& cluster_nodes,
    const LimitMatrixOptions& options) {
  if (rows.size() != row_names.size()) {
    throw std::invalid_argument("influence_total row name count mismatch");
  }
  std::vector<InfluenceTotalEntry> out;
  out.reserve(rows.size());
  for (std::size_t i = 0; i < rows.size(); ++i) {
    InfluenceTotalEntry e = influence_total_row(
        mat, rows[i], influence_nodes, delta, p0, cluster_nodes, options);
    e.name = row_names[i];
    out.push_back(std::move(e));
  }
  return out;
}

}  // namespace anpcpp
