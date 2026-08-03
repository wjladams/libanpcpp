#include "anpcpp/multiuser.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace anpcpp {

void aggregate_pairwise_geometric(
    const std::vector<const PairwiseJudgments*>& inputs,
    PairwiseJudgments& out) {
  if (out.empty()) return;
  const std::size_t n = out.size();
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      double log_sum = 0.0;
      int count = 0;
      for (const PairwiseJudgments* pw : inputs) {
        if (pw == nullptr || pw->size() != n) continue;
        const double v = pw->comparison(i, j);
        if (!(v > 0.0) || !std::isfinite(v)) continue;
        log_sum += std::log(v);
        ++count;
      }
      if (count == 0) {
        out.set_comparison(i, j, 0.0);
      } else {
        out.set_comparison(i, j, std::exp(log_sum / static_cast<double>(count)));
      }
    }
  }
}

void aggregate_ratings_arithmetic(
    const std::vector<const RatingsPrioritizer*>& inputs,
    const RatingsPrioritizer& scale_template,
    RatingsPrioritizer& out) {
  out = RatingsPrioritizer(scale_template.alternatives());
  out.set_mode(RatingsPrioritizer::Mode::Numeric);
  out.set_interpreter(IdentityInterpreter{});
  out.set_categories(scale_template.categories());

  for (const std::string& alt : out.alternatives()) {
    double sum = 0.0;
    int count = 0;
    for (const RatingsPrioritizer* rt : inputs) {
      if (rt == nullptr || !rt->has_alternative(alt)) continue;
      const Vector s = rt->scores();
      const std::size_t idx = rt->index_of(alt);
      if (idx >= s.size()) continue;
      // Treat missing intensity as skip: categorical/numeric missing → 0 in
      // scores(), but we need to know if the vote was present.
      const bool present =
          (rt->mode() == RatingsPrioritizer::Mode::Categorical)
              ? rt->rating(alt).has_value()
              : rt->value(alt).has_value();
      if (!present) continue;
      sum += s[idx];
      ++count;
    }
    if (count > 0) {
      out.set_value(alt, sum / static_cast<double>(count));
    } else {
      out.clear_value(alt);
    }
  }
}

void copy_pairwise_into(const PairwiseJudgments& src, PairwiseJudgments& out) {
  const std::size_t n = out.size();
  if (src.size() != n) {
    throw std::invalid_argument(
        "copy_pairwise_into: alternative count mismatch");
  }
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      out.set_comparison(i, j, src.comparison(i, j));
    }
  }
}

void copy_ratings_votes_into(const RatingsPrioritizer& src,
                             RatingsPrioritizer& out) {
  out.set_mode(src.mode());
  out.set_categories(src.categories());
  out.set_interpreter(src.interpreter());
  for (const std::string& alt : out.alternatives()) {
    if (!src.has_alternative(alt)) {
      if (out.mode() == RatingsPrioritizer::Mode::Categorical) {
        out.clear_rating(alt);
      } else {
        out.clear_value(alt);
      }
      continue;
    }
    if (src.mode() == RatingsPrioritizer::Mode::Categorical) {
      const auto r = src.rating(alt);
      if (r.has_value()) {
        out.set_rating(alt, *r);
      } else {
        out.clear_rating(alt);
      }
    } else {
      const auto v = src.value(alt);
      if (v.has_value()) {
        out.set_value(alt, *v);
      } else {
        out.clear_value(alt);
      }
    }
  }
}

std::vector<std::vector<PairwiseCellDisagreement>> pairwise_disagreement(
    const std::vector<const PairwiseJudgments*>& inputs) {
  std::size_t n = 0;
  for (const PairwiseJudgments* pw : inputs) {
    if (pw != nullptr && !pw->empty()) {
      n = pw->size();
      break;
    }
  }
  std::vector<std::vector<PairwiseCellDisagreement>> out(
      n, std::vector<PairwiseCellDisagreement>(n));
  if (n < 2) return out;

  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      double vmin = 0.0;
      double vmax = 0.0;
      int count = 0;
      for (const PairwiseJudgments* pw : inputs) {
        if (pw == nullptr || pw->size() != n) continue;
        const double v = pw->comparison(i, j);
        if (!(v > 0.0) || !std::isfinite(v)) continue;
        if (count == 0) {
          vmin = vmax = v;
        } else {
          vmin = std::min(vmin, v);
          vmax = std::max(vmax, v);
        }
        ++count;
      }
      PairwiseCellDisagreement cell;
      cell.contributor_count = count;
      if (count >= 1) {
        cell.min = vmin;
        cell.max = vmax;
      }
      if (count >= 2 && vmin > 0.0) {
        cell.ratio = vmax / vmin;
      }
      out[i][j] = cell;
      // Mirror for convenience when reading the lower triangle.
      PairwiseCellDisagreement mirror = cell;
      if (count >= 1 && vmax > 0.0) {
        mirror.min = 1.0 / vmax;
        mirror.max = 1.0 / vmin;
        if (count >= 2) mirror.ratio = cell.ratio;
      }
      out[j][i] = mirror;
    }
  }
  return out;
}

std::vector<RatingsAltDisagreement> ratings_disagreement(
    const std::vector<const RatingsPrioritizer*>& inputs,
    const std::vector<std::string>& alt_order) {
  std::vector<std::string> alts = alt_order;
  if (alts.empty()) {
    for (const RatingsPrioritizer* rt : inputs) {
      if (rt != nullptr && !rt->empty()) {
        alts = rt->alternatives();
        break;
      }
    }
  }

  std::vector<RatingsAltDisagreement> out;
  out.reserve(alts.size());
  for (const std::string& alt : alts) {
    RatingsAltDisagreement cell;
    cell.alt = alt;
    double vmin = 0.0;
    double vmax = 0.0;
    int count = 0;
    for (const RatingsPrioritizer* rt : inputs) {
      if (rt == nullptr || !rt->has_alternative(alt)) continue;
      const bool present =
          (rt->mode() == RatingsPrioritizer::Mode::Categorical)
              ? rt->rating(alt).has_value()
              : rt->value(alt).has_value();
      if (!present) continue;
      const Vector s = rt->scores();
      const std::size_t idx = rt->index_of(alt);
      if (idx >= s.size()) continue;
      const double v = s[idx];
      if (!std::isfinite(v)) continue;
      if (count == 0) {
        vmin = vmax = v;
      } else {
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
      }
      ++count;
    }
    cell.contributor_count = count;
    if (count >= 1) {
      cell.min = vmin;
      cell.max = vmax;
    }
    if (count >= 2) cell.range = vmax - vmin;
    out.push_back(std::move(cell));
  }
  return out;
}

JudgmentFillCounts pairwise_fill_counts(const PairwiseJudgments& pw) {
  JudgmentFillCounts c;
  const std::size_t n = pw.size();
  if (n < 2) {
    c.needed = 0;
    c.filled = n == 1 ? 1 : 0;
    return c;
  }
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      ++c.needed;
      if (pw.comparison(i, j) != 0.0) ++c.filled;
    }
  }
  return c;
}

JudgmentFillCounts ratings_fill_counts(const RatingsPrioritizer& rt) {
  JudgmentFillCounts c;
  c.needed = rt.size();
  if (c.needed == 0) return c;
  for (const std::string& alt : rt.alternatives()) {
    if (rt.mode() == RatingsPrioritizer::Mode::Categorical) {
      if (rt.rating(alt).has_value()) ++c.filled;
    } else if (rt.value(alt).has_value()) {
      ++c.filled;
    }
  }
  return c;
}

namespace {

double sample_sd(const std::vector<double>& xs, double center) {
  if (xs.size() < 2) return 0.0;
  double sum_sq = 0.0;
  for (double x : xs) {
    const double d = x - center;
    sum_sq += d * d;
  }
  return std::sqrt(sum_sq / static_cast<double>(xs.size() - 1));
}

}  // namespace

double pairwise_alignment_pct(double min_v, double max_v) {
  if (!(min_v > 0.0) || !(max_v > 0.0) || !std::isfinite(min_v) ||
      !std::isfinite(max_v)) {
    return 0.0;
  }
  if (max_v < min_v) std::swap(min_v, max_v);
  const double ratio = max_v / min_v;
  if (!(ratio > 1.0)) return 100.0;
  const double span = std::log(kPairwiseAlignmentSpanRatio);
  if (!(span > 0.0)) return 0.0;
  const double raw = 100.0 * (1.0 - std::log(ratio) / span);
  return std::clamp(raw, 0.0, 100.0);
}

double ratings_alignment_pct(double range, double full_scale) {
  if (!(full_scale > 0.0) || !std::isfinite(full_scale) || !std::isfinite(range)) {
    return 0.0;
  }
  if (!(range > 0.0)) return 100.0;
  const double raw = 100.0 * (1.0 - range / full_scale);
  return std::clamp(raw, 0.0, 100.0);
}

VoteSpreadSummary summarize_pairwise_votes(const std::vector<double>& values) {
  VoteSpreadSummary s;
  for (double v : values) {
    if (v > 0.0 && std::isfinite(v)) s.values.push_back(v);
  }
  s.contributor_count = static_cast<int>(s.values.size());
  if (s.values.empty()) return s;

  s.min = *std::min_element(s.values.begin(), s.values.end());
  s.max = *std::max_element(s.values.begin(), s.values.end());
  s.alignment_pct = (s.contributor_count == 1)
                        ? 100.0
                        : pairwise_alignment_pct(s.min, s.max);

  std::vector<double> logs;
  logs.reserve(s.values.size());
  double sum_log = 0.0;
  for (double v : s.values) {
    const double L = std::log(v);
    logs.push_back(L);
    sum_log += L;
  }
  const double mean_log =
      sum_log / static_cast<double>(s.values.size());
  s.mean = std::exp(mean_log);
  const double sd_log = sample_sd(logs, mean_log);
  s.band_low = std::exp(mean_log - sd_log);
  s.band_high = std::exp(mean_log + sd_log);
  return s;
}

VoteSpreadSummary summarize_ratings_votes(const std::vector<double>& values,
                                          double full_scale) {
  VoteSpreadSummary s;
  for (double v : values) {
    if (std::isfinite(v)) s.values.push_back(v);
  }
  s.contributor_count = static_cast<int>(s.values.size());
  if (s.values.empty()) return s;

  s.min = *std::min_element(s.values.begin(), s.values.end());
  s.max = *std::max_element(s.values.begin(), s.values.end());
  const double range = s.max - s.min;
  const double scale =
      (full_scale > 0.0) ? full_scale : std::max(1.0, s.max - std::min(0.0, s.min));
  s.alignment_pct =
      (s.contributor_count == 1) ? 100.0 : ratings_alignment_pct(range, scale);

  double sum = 0.0;
  for (double v : s.values) sum += v;
  s.mean = sum / static_cast<double>(s.values.size());
  const double sd = sample_sd(s.values, s.mean);
  s.band_low = s.mean - sd;
  s.band_high = s.mean + sd;
  return s;
}

}  // namespace anpcpp
