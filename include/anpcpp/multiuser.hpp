/**
 * @file multiuser.hpp
 * @brief Multi-user judgment participants, groups, scope, and aggregation.
 */

#pragma once

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "anpcpp/pairwise.hpp"
#include "anpcpp/ratings.hpp"

namespace anpcpp {

/** @brief A judge on a model (model-scoped, not an app account). */
struct JudgmentParticipant {
  std::string id;
  std::string name;
  std::string email;
};

/** @brief Named subset of participants for group-scope analysis. */
struct JudgmentGroup {
  std::string id;
  std::string name;
  std::vector<std::string> member_ids;
};

/** @brief Whose judgments feed the effective calc slot. */
enum class JudgmentScopeKind {
  /** Geometric / arithmetic average over all participants with data. */
  Average,
  /** Single participant. */
  Participant,
  /** Named group members only. */
  Group,
};

/**
 * @brief Document session: which judgments are active for editing/calc.
 *
 * Shared across Judgments, Analysis, and Researcher (one scope).
 */
struct JudgmentSession {
  JudgmentScopeKind kind = JudgmentScopeKind::Average;
  /** Participant or group id when kind is Participant or Group. */
  std::string id;
};

/** Default participant id used when migrating single-user (v1) models. */
inline constexpr const char* kDefaultParticipantId = "default";

/**
 * @brief Geometric mean of pairwise ratios into @p out (same alternatives).
 *
 * For each upper-triangle cell, averages only inputs with a positive finite
 * ratio. Incomplete cells (0) are skipped. If no contributor, leaves 0.
 */
void aggregate_pairwise_geometric(
    const std::vector<const PairwiseJudgments*>& inputs,
    PairwiseJudgments& out);

/**
 * @brief Arithmetic mean of rating intensity scores into @p out.
 *
 * @p out is set to Numeric + Identity; values are the mean of each
 * contributor's @ref RatingsPrioritizer::scores for that alternative
 * (missing scores skipped). Shared categories/interpreter from @p scale_template
 * are copied onto @p out for reference but mode is Numeric for exact means.
 */
void aggregate_ratings_arithmetic(
    const std::vector<const RatingsPrioritizer*>& inputs,
    const RatingsPrioritizer& scale_template,
    RatingsPrioritizer& out);

/**
 * @brief Copy @p src pairwise into @p out (alternatives aligned to @p out).
 */
void copy_pairwise_into(const PairwiseJudgments& src, PairwiseJudgments& out);

/**
 * @brief Copy votes from @p src into @p out, syncing mode/scale from @p src.
 */
void copy_ratings_votes_into(const RatingsPrioritizer& src,
                             RatingsPrioritizer& out);

/** @brief Disagreement stats for one pairwise upper-triangle cell. */
struct PairwiseCellDisagreement {
  double min = 0.0;
  double max = 0.0;
  /** max/min when contributor_count >= 2; else 0. */
  double ratio = 0.0;
  int contributor_count = 0;
};

/** @brief Disagreement stats for one ratings alternative. */
struct RatingsAltDisagreement {
  std::string alt;
  double min = 0.0;
  double max = 0.0;
  /** max - min when contributor_count >= 2; else 0. */
  double range = 0.0;
  int contributor_count = 0;
};

/** @brief Filled vs needed judgment counts for coverage grids. */
struct JudgmentFillCounts {
  std::size_t filled = 0;
  std::size_t needed = 0;
};

/**
 * @brief Per upper-triangle cell max/min ratio across @p inputs.
 *
 * Alternatives are taken from the first non-null input with size > 0.
 * Inputs with a different size are skipped. Cells with fewer than two
 * positive finite votes get ratio 0.
 */
[[nodiscard]] std::vector<std::vector<PairwiseCellDisagreement>>
pairwise_disagreement(const std::vector<const PairwiseJudgments*>& inputs);

/**
 * @brief Per-alternative intensity range across @p inputs.
 *
 * Uses scores() for present votes (categorical rating or numeric value).
 * Alternatives come from @p alt_order when non-empty; otherwise from the
 * first non-null input.
 */
[[nodiscard]] std::vector<RatingsAltDisagreement> ratings_disagreement(
    const std::vector<const RatingsPrioritizer*>& inputs,
    const std::vector<std::string>& alt_order = {});

/**
 * @brief Count filled upper-triangle cells (comparison != 0) vs needed.
 */
[[nodiscard]] JudgmentFillCounts pairwise_fill_counts(
    const PairwiseJudgments& pw);

/**
 * @brief Count present ratings votes vs number of alternatives.
 */
[[nodiscard]] JudgmentFillCounts ratings_fill_counts(
    const RatingsPrioritizer& rt);

/**
 * @brief Summary of collected votes for one comparison / ratings alternative.
 *
 * Pairwise: geometric mean and ±1 geometric SD (log-space, SD about the mean).
 * Ratings: arithmetic mean and ±1 arithmetic SD (SD about the mean).
 * Alignment is 0–100 (100 = identical votes).
 */
struct VoteSpreadSummary {
  std::vector<double> values;
  double min = 0.0;
  double max = 0.0;
  /** Geometric mean (pairwise) or arithmetic mean (ratings). */
  double mean = 0.0;
  double band_low = 0.0;
  double band_high = 0.0;
  /** 0–100; 100 when all votes equal (or a single vote). */
  double alignment_pct = 0.0;
  int contributor_count = 0;
};

/** max/min span that maps to 0% alignment on the Saaty strip (9 ÷ 1/9). */
inline constexpr double kPairwiseAlignmentSpanRatio = 81.0;

/**
 * @brief Pairwise alignment from min/max ratios.
 *
 * @code 100 * (1 - ln(max/min) / ln(81)) @endcode clamped to [0, 100].
 * Returns 100 when max/min <= 1 or count would be identical; 0 at full span.
 */
[[nodiscard]] double pairwise_alignment_pct(double min_v, double max_v);

/**
 * @brief Ratings alignment from intensity range vs @p full_scale.
 *
 * @code 100 * (1 - range / full_scale) @endcode clamped to [0, 100].
 */
[[nodiscard]] double ratings_alignment_pct(double range, double full_scale);

/**
 * @brief Geometric mean / SD spread for positive pairwise ratios.
 */
[[nodiscard]] VoteSpreadSummary summarize_pairwise_votes(
    const std::vector<double>& values);

/**
 * @brief Arithmetic mean / SD spread for ratings intensities.
 */
[[nodiscard]] VoteSpreadSummary summarize_ratings_votes(
    const std::vector<double>& values, double full_scale = 1.0);

}  // namespace anpcpp
