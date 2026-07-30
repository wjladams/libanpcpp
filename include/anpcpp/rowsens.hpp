/**
 * @file rowsens.hpp
 * @brief ANP row sensitivity and influence (ported from pyanp.rowsens).
 */

#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "anpcpp/limit_matrix.hpp"
#include "anpcpp/matrix.hpp"

namespace anpcpp {

/**
 * @brief How resting parameter \(p_0\) is chosen for row sensitivity.
 */
enum class P0ModeKind {
  /** Fixed \(p_0\) (typically 0.5). */
  Direct,
  /** Smart \(p_0\) making scores continuous for one alternative index. */
  Smart,
  /** Use each column's original row weight as \(p_0\). */
  OriginalWeight
};

/**
 * @brief \(p_0\) selection for @ref row_adjust.
 */
struct P0Mode {
  P0ModeKind kind = P0ModeKind::Direct;
  /** Used when @ref kind is Direct. */
  double direct = 0.5;
  /** Alternative/node index made continuous when @ref kind is Smart. */
  std::size_t smart_alt = 0;

  /** @return Direct mode with the given \(p_0\). */
  static P0Mode Direct(double p0) {
    P0Mode m;
    m.kind = P0ModeKind::Direct;
    m.direct = p0;
    return m;
  }
  /** @return Smart mode for continuity of @p alt_index. */
  static P0Mode Smart(std::size_t alt_index) {
    P0Mode m;
    m.kind = P0ModeKind::Smart;
    m.smart_alt = alt_index;
    return m;
  }
  /** @return Per-column original-weight \(p_0\). */
  static P0Mode Original() {
    P0Mode m;
    m.kind = P0ModeKind::OriginalWeight;
    return m;
  }
};

/**
 * @brief One alternative's raw (fixed-distance) influence result.
 */
struct InfluenceRawEntry {
  std::string name;
  double original = 0.0;
  double up_score = 0.0;
  double up_diff = 0.0;
  double down_score = 0.0;
  double down_diff = 0.0;
};

/**
 * @brief One alternative's rank-influence result.
 */
struct InfluenceRankEntry {
  std::string name;
  double original = 0.0;
  double rank_influence = 0.0;
};

/**
 * @brief One alternative's smart-\(p_0\) marginal influence.
 */
struct InfluenceMarginalEntry {
  std::string name;
  double marginal = 0.0;
  double smart_p0 = 0.5;
};

/**
 * @brief Adjusts row @p row of scaled supermatrix @p mat to parameter @p p.
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param p Sensitivity parameter in \([0,1]\).
 * @param p0mode Resting-value mode.
 * @param cluster_nodes Indices in the Wrt node's cluster (including @p row);
 *        empty means the full matrix (all rows).
 * @return Adjusted matrix copy.
 */
[[nodiscard]] Matrix row_adjust(const Matrix& mat,
                                std::size_t row,
                                double p,
                                const P0Mode& p0mode = P0Mode::Direct(0.5),
                                const std::vector<std::size_t>& cluster_nodes = {});

/**
 * @brief Smart resting \(p_0\) making score of @p cont_alt continuous at \(p_0\).
 */
[[nodiscard]] double smart_p0(
    const Matrix& mat,
    std::size_t row,
    std::size_t cont_alt,
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {});

/**
 * @brief Priorities after row adjust (pyanp row_adjust_priority, normalize_to_orig).
 */
[[nodiscard]] Vector priority_after_row_adjust(
    const Matrix& mat,
    std::size_t row,
    double p,
    const P0Mode& p0mode = P0Mode::Direct(0.5),
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {},
    bool normalize_to_orig = true);

/**
 * @brief Marginal influence (finite difference) at a direct \(p_0\).
 * @param left_or_right &lt;0 LHS, &gt;0 RHS, 0 average.
 */
[[nodiscard]] Vector influence_marginal(
    const Matrix& mat,
    std::size_t row,
    double p0,
    int left_or_right = 0,
    double delta = 1e-6,
    const std::vector<std::size_t>& influence_nodes = {},
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {});

/**
 * @brief Per-alternative smart-\(p_0\) marginal influence (single value each).
 */
[[nodiscard]] std::vector<InfluenceMarginalEntry> influence_marginal_smart(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    const std::vector<std::string>& names,
    const std::vector<std::size_t>& cluster_nodes = {},
    double delta = 1e-6,
    const LimitMatrixOptions& options = {});

/**
 * @brief Raw fixed-distance influence table (original / up / down scores).
 */
[[nodiscard]] std::vector<InfluenceRawEntry> influence_raw(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    const std::vector<std::string>& names,
    double delta_up = 0.1,
    double delta_down = 0.1,
    double p0 = 0.5,
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {});

/**
 * @brief Rank influence score per alternative (max of upper/lower searches).
 */
[[nodiscard]] std::vector<InfluenceRankEntry> influence_rank(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    const std::vector<std::string>& names,
    const std::vector<std::size_t>& cluster_nodes = {},
    double error = 1e-5,
    int round_to_decimal = 5,
    const LimitMatrixOptions& options = {});

}  // namespace anpcpp
