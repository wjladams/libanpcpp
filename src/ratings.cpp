#include "anpcpp/ratings.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace anpcpp {
namespace {

constexpr double kEps = 1e-15;

double clamp01(double v) {
  if (v < 0.0) return 0.0;
  if (v > 1.0) return 1.0;
  return v;
}

std::vector<std::pair<double, double>> sorted_knots(
    std::vector<std::pair<double, double>> knots) {
  std::sort(knots.begin(), knots.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  return knots;
}

double eval_piecewise(const std::vector<std::pair<double, double>>& knots,
                      double x) {
  if (knots.empty()) {
    return 0.0;
  }
  if (x <= knots.front().first) {
    return clamp01(knots.front().second);
  }
  if (x >= knots.back().first) {
    return clamp01(knots.back().second);
  }
  for (std::size_t i = 0; i + 1 < knots.size(); ++i) {
    const double x0 = knots[i].first;
    const double x1 = knots[i + 1].first;
    if (x >= x0 && x <= x1) {
      if (std::abs(x1 - x0) < kEps) {
        return clamp01(knots[i].second);
      }
      const double t = (x - x0) / (x1 - x0);
      const double y =
          knots[i].second + t * (knots[i + 1].second - knots[i].second);
      return clamp01(y);
    }
  }
  return clamp01(knots.back().second);
}

}  // namespace

std::vector<std::optional<double>> apply_score_interpreter(
    const ScoreInterpreter& interpreter,
    const std::vector<std::optional<double>>& raw) {
  std::vector<std::optional<double>> out(raw.size());

  auto present_stats = [&](double& min_v, double& max_v, bool& any) {
    any = false;
    for (const auto& v : raw) {
      if (!v.has_value()) continue;
      if (!any) {
        min_v = max_v = *v;
        any = true;
      } else {
        min_v = std::min(min_v, *v);
        max_v = std::max(max_v, *v);
      }
    }
  };

  std::visit(
      [&](const auto& interp) {
        using T = std::decay_t<decltype(interp)>;
        if constexpr (std::is_same_v<T, IdentityInterpreter>) {
          for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i].has_value()) {
              out[i] = clamp01(*raw[i]);
            }
          }
        } else if constexpr (std::is_same_v<T, DivideByMaxInterpreter>) {
          double min_v = 0, max_v = 0;
          bool any = false;
          present_stats(min_v, max_v, any);
          if (!any || std::abs(max_v) < kEps) {
            return;
          }
          for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i].has_value()) {
              out[i] = clamp01(*raw[i] / max_v);
            }
          }
        } else if constexpr (std::is_same_v<T, DivideByConstantInterpreter>) {
          if (std::abs(interp.constant) < kEps) {
            throw std::invalid_argument(
                "DivideByConstantInterpreter constant must be non-zero");
          }
          for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i].has_value()) {
              out[i] = clamp01(*raw[i] / interp.constant);
            }
          }
        } else if constexpr (std::is_same_v<T, MinMaxNormalizeInterpreter>) {
          double min_v = 0, max_v = 0;
          bool any = false;
          present_stats(min_v, max_v, any);
          if (!any) {
            return;
          }
          const double span = max_v - min_v;
          for (std::size_t i = 0; i < raw.size(); ++i) {
            if (!raw[i].has_value()) continue;
            if (std::abs(span) < kEps) {
              out[i] = 1.0;
            } else {
              out[i] = clamp01((*raw[i] - min_v) / span);
            }
          }
        } else if constexpr (std::is_same_v<T, PiecewiseLinearInterpreter>) {
          const auto knots = sorted_knots(interp.knots);
          for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i].has_value()) {
              out[i] = eval_piecewise(knots, *raw[i]);
            }
          }
        }
      },
      interpreter);

  return out;
}

RatingsPrioritizer::RatingsPrioritizer(std::vector<std::string> alternatives)
    : alternatives_(std::move(alternatives)),
      categorical_(alternatives_.size()),
      numeric_(alternatives_.size()) {}

bool RatingsPrioritizer::has_alternative(const std::string& name) const {
  for (const auto& alt : alternatives_) {
    if (alt == name) return true;
  }
  return false;
}

std::size_t RatingsPrioritizer::index_of(const std::string& name) const {
  for (std::size_t i = 0; i < alternatives_.size(); ++i) {
    if (alternatives_[i] == name) return i;
  }
  throw std::invalid_argument("unknown ratings alternative: " + name);
}

void RatingsPrioritizer::add_alternative(const std::string& name,
                                         bool ignore_existing) {
  if (has_alternative(name)) {
    if (ignore_existing) return;
    throw std::invalid_argument("ratings alternative already exists: " + name);
  }
  alternatives_.push_back(name);
  categorical_.emplace_back(std::nullopt);
  numeric_.emplace_back(std::nullopt);
}

void RatingsPrioritizer::remove_alternative(const std::string& name,
                                            bool ignore_missing) {
  if (!has_alternative(name)) {
    if (ignore_missing) return;
    throw std::invalid_argument("unknown ratings alternative: " + name);
  }
  const std::size_t idx = index_of(name);
  alternatives_.erase(alternatives_.begin() +
                      static_cast<std::ptrdiff_t>(idx));
  categorical_.erase(categorical_.begin() + static_cast<std::ptrdiff_t>(idx));
  numeric_.erase(numeric_.begin() + static_cast<std::ptrdiff_t>(idx));
}

void RatingsPrioritizer::set_categories(
    std::vector<RatingCategory> categories) {
  for (const auto& c : categories) {
    if (c.id.empty()) {
      throw std::invalid_argument("rating category id must be non-empty");
    }
    if (c.value < 0.0 || c.value > 1.0) {
      throw std::invalid_argument("rating category value must be in [0, 1]: " +
                                  c.id);
    }
  }
  for (std::size_t i = 0; i < categories.size(); ++i) {
    for (std::size_t j = i + 1; j < categories.size(); ++j) {
      if (categories[i].id == categories[j].id) {
        throw std::invalid_argument("duplicate rating category id: " +
                                    categories[i].id);
      }
    }
  }
  categories_ = std::move(categories);
}

const RatingCategory* RatingsPrioritizer::find_category(
    const std::string& id) const {
  for (const auto& c : categories_) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

void RatingsPrioritizer::set_rating(const std::string& alt,
                                    const std::string& category_id) {
  const std::size_t i = index_of(alt);
  if (category_id.empty()) {
    categorical_[i] = std::nullopt;
    return;
  }
  if (find_category(category_id) == nullptr) {
    throw std::invalid_argument("unknown rating category: " + category_id);
  }
  categorical_[i] = category_id;
}

void RatingsPrioritizer::clear_rating(const std::string& alt) {
  categorical_[index_of(alt)] = std::nullopt;
}

std::optional<std::string> RatingsPrioritizer::rating(
    const std::string& alt) const {
  return categorical_[index_of(alt)];
}

void RatingsPrioritizer::set_value(const std::string& alt, double raw) {
  numeric_[index_of(alt)] = raw;
}

void RatingsPrioritizer::clear_value(const std::string& alt) {
  numeric_[index_of(alt)] = std::nullopt;
}

std::optional<double> RatingsPrioritizer::value(const std::string& alt) const {
  return numeric_[index_of(alt)];
}

std::optional<double> RatingsPrioritizer::intensity_at(std::size_t i) const {
  if (mode_ == Mode::Categorical) {
    if (!categorical_[i].has_value()) return std::nullopt;
    const RatingCategory* cat = find_category(*categorical_[i]);
    if (cat == nullptr) return std::nullopt;
    return cat->value;
  }
  // Numeric intensities are computed in batch via the interpreter.
  return numeric_[i];
}

Vector RatingsPrioritizer::scores() const {
  Vector out(size(), 0.0);
  if (empty()) return out;

  if (mode_ == Mode::Categorical) {
    for (std::size_t i = 0; i < size(); ++i) {
      const auto s = intensity_at(i);
      out[i] = s.value_or(0.0);
    }
    return out;
  }

  const auto interpreted = apply_score_interpreter(interpreter_, numeric_);
  for (std::size_t i = 0; i < size(); ++i) {
    out[i] = interpreted[i].value_or(0.0);
  }
  return out;
}

Vector RatingsPrioritizer::priorities() const {
  if (empty()) return Vector{};

  std::vector<std::optional<double>> present(size());
  if (mode_ == Mode::Categorical) {
    for (std::size_t i = 0; i < size(); ++i) {
      present[i] = intensity_at(i);
    }
  } else {
    present = apply_score_interpreter(interpreter_, numeric_);
  }

  double sum = 0.0;
  for (const auto& s : present) {
    if (s.has_value()) sum += *s;
  }
  Vector out(size(), 0.0);
  if (sum < kEps) return out;
  for (std::size_t i = 0; i < size(); ++i) {
    if (present[i].has_value()) {
      out[i] = *present[i] / sum;
    }
  }
  return out;
}

}  // namespace anpcpp
