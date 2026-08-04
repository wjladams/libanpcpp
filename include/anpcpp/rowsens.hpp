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
  /** Used when kind is Direct. */
  double direct = 0.5;
  /** Alternative/node index made continuous when kind is Smart. */
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
 * @brief Rank-influence result for one row (or one alternative under a fixed Wrt).
 */
struct InfluenceRankEntry {
  std::string name;
  double original = 0.0;
  double rank_influence = 0.0;
};

/**
 * @brief Smart-\(p_0\) marginal influence for one row or alternative.
 */
struct InfluenceMarginalEntry {
  std::string name;
  double marginal = 0.0;
  double smart_p0 = 0.5;
};

/**
 * @brief Fixed-distance total influence for one row (pyanp influence_fixed Total).
 */
struct InfluenceTotalEntry {
  std::string name;
  double total_influence = 0.0;
  double max_alt_change = 0.0;
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
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param cont_alt Alternative/node index made continuous.
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
 */
[[nodiscard]] double smart_p0(
    const Matrix& mat,
    std::size_t row,
    std::size_t cont_alt,
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {});

/**
 * @brief Priorities after row adjust (pyanp row_adjust_priority, normalize_to_orig).
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param p Sensitivity parameter in \([0,1]\).
 * @param p0mode Resting-value mode.
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
 * @param normalize_to_orig If true, rescale so the Wrt entry matches the original.
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
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param p0 Direct resting parameter.
 * @param left_or_right &lt;0 LHS, &gt;0 RHS, 0 average.
 * @param delta Finite-difference step.
 * @param influence_nodes Nodes whose scores are reported (empty = all but @p row).
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
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
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param influence_nodes Alternative indices to score.
 * @param names Display names aligned with @p influence_nodes.
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param delta Finite-difference step.
 * @param options Limit-matrix options.
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
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param influence_nodes Alternative indices to score.
 * @param names Display names aligned with @p influence_nodes.
 * @param delta_up Upward change from @p p0.
 * @param delta_down Downward change from @p p0.
 * @param p0 Resting parameter (typically 0.5).
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
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
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param influence_nodes Alternative indices to score.
 * @param names Display names aligned with @p influence_nodes.
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param error Binary-search tolerance on \(p\).
 * @param round_to_decimal Decimals used when comparing ranks.
 * @param options Limit-matrix options.
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

/**
 * @brief Fixed-distance total influence for one row (L1 and max of abs diffs).
 *
 * Matches pyanp `influence_fixed` for a single row, then aggregates to Total /
 * Max Alt Change as in the multi-row case.
 *
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param influence_nodes Alternative indices to score.
 * @param delta Fixed upward change from @p p0.
 * @param p0 Resting parameter (typically 0.5).
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
 */
[[nodiscard]] InfluenceTotalEntry influence_total_row(
    const Matrix& mat,
    std::size_t row,
    const std::vector<std::size_t>& influence_nodes,
    double delta = 0.25,
    double p0 = 0.5,
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {});

/**
 * @brief Per-row total influence (pyanp multi-row `influence_fixed` Totals).
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param rows Row indices to score.
 * @param row_names Display names aligned with @p rows.
 * @param influence_nodes Alternative indices used for each row's diffs.
 * @param delta Fixed upward change from @p p0.
 * @param p0 Resting parameter (typically 0.5).
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
 */
[[nodiscard]] std::vector<InfluenceTotalEntry> influence_total(
    const Matrix& mat,
    const std::vector<std::size_t>& rows,
    const std::vector<std::string>& row_names,
    const std::vector<std::size_t>& influence_nodes,
    double delta = 0.25,
    double p0 = 0.5,
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {});

/**
 * @brief Perspective of one row: limit as p approaches 1 of row sensitivity.
 *
 * Evaluates row sensitivity near p = 1 (never at exactly 1, which
 * degenerates @ref row_adjust). Compares p = 1-1e-5 and p = 1-1e-6; if they
 * disagree beyond 1e-6 (L-inf), refines at p = 1-1e-7.
 *
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param row Row index (Wrt node).
 * @param p0mode Resting-value mode.
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
 * @param normalize_to_orig If true, rescale like @ref priority_after_row_adjust.
 */
[[nodiscard]] Vector perspective(
    const Matrix& mat,
    std::size_t row,
    const P0Mode& p0mode = P0Mode::Direct(0.5),
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {},
    bool normalize_to_orig = true);

/**
 * @brief Perspective matrix: column j is @ref perspective for @p rows[j].
 *
 * Empty @p rows means every row index 0 .. n-1.
 *
 * @param mat Scaled (column-stochastic) supermatrix.
 * @param rows Row indices used as columns (empty = all rows).
 * @param p0mode Resting-value mode.
 * @param cluster_nodes Cluster row indices (empty = full matrix).
 * @param options Limit-matrix options.
 * @param normalize_to_orig If true, rescale like @ref priority_after_row_adjust.
 */
[[nodiscard]] Matrix perspective_matrix(
    const Matrix& mat,
    const std::vector<std::size_t>& rows = {},
    const P0Mode& p0mode = P0Mode::Direct(0.5),
    const std::vector<std::size_t>& cluster_nodes = {},
    const LimitMatrixOptions& options = {},
    bool normalize_to_orig = true);

/** Near-1 p values for limit-as-p-approaches-1 perspective (never exactly 1). */
inline constexpr double kPerspectivePCoarse = 1.0 - 1e-6;
inline constexpr double kPerspectivePFine = 1.0 - 1e-7;
inline constexpr double kPerspectivePRefine = 1.0 - 1e-9;
/** Max L-inf gap between coarse/fine before refining further. */
inline constexpr double kPerspectiveAgreeTol = 1e-6;

}  // namespace anpcpp
