/**
 * @file ratings.hpp
 * @brief Ratings-based node prioritizers (categorical or numeric).
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "anpcpp/matrix.hpp"

namespace anpcpp {

/**
 * @brief A named rating category with an associated score in [0, 1].
 */
struct RatingCategory {
  /** Stable id used in rating assignments (e.g. "H"). */
  std::string id;
  /** Display label (e.g. "High"). */
  std::string label;
  /** Score in [0, 1]. */
  double value = 0.0;
};

/** @brief Pass-through interpreter (values already in [0, 1]). */
struct IdentityInterpreter {};

/** @brief Divide each raw value by the max among present values. */
struct DivideByMaxInterpreter {};

/** @brief Divide each raw value by a fixed positive constant. */
struct DivideByConstantInterpreter {
  double constant = 1.0;
};

/**
 * @brief Map the column's [min, max] onto [0, 1].
 *
 * If all present values are equal, each present score is 1.
 */
struct MinMaxNormalizeInterpreter {};

/**
 * @brief Piecewise-linear map from raw value to [0, 1] via sorted knots.
 *
 * Outside the knot range, scores are clamped to the endpoint y values.
 */
struct PiecewiseLinearInterpreter {
  /** Knots (x, y); sorted by x ascending on use. */
  std::vector<std::pair<double, double>> knots;
};

/**
 * @brief Declarative raw-value → [0, 1] interpreter (JSON-serializable).
 */
using ScoreInterpreter =
    std::variant<IdentityInterpreter,
                 DivideByMaxInterpreter,
                 DivideByConstantInterpreter,
                 MinMaxNormalizeInterpreter,
                 PiecewiseLinearInterpreter>;

/**
 * @brief Apply @p interpreter to a column of optional raw values.
 *
 * Missing entries stay nullopt. Present entries become scores in [0, 1]
 * (clamped). Interpreters that need column statistics ignore missing cells.
 */
[[nodiscard]] std::vector<std::optional<double>> apply_score_interpreter(
    const ScoreInterpreter& interpreter,
    const std::vector<std::optional<double>>& raw);

/**
 * @brief Ratings table over named alternatives for one (wrt node, dest cluster).
 *
 * Produces intensity @ref scores and L1-normalized @ref priorities for the
 * unscaled supermatrix column (same contract as pairwise priorities).
 */
class RatingsPrioritizer {
public:
  /** @brief How alternative ratings are stored. */
  enum class Mode { Categorical, Numeric };

  /** @brief Default-constructs an empty numeric ratings table. */
  RatingsPrioritizer() = default;

  /**
   * @brief Constructs a table with the given alternative names.
   * @param alternatives Ordered alternative names.
   */
  explicit RatingsPrioritizer(std::vector<std::string> alternatives);

  /** @return Number of alternatives. */
  [[nodiscard]] std::size_t size() const noexcept {
    return alternatives_.size();
  }
  /** @return True if there are no alternatives. */
  [[nodiscard]] bool empty() const noexcept { return alternatives_.empty(); }
  /** @return Ordered alternative names. */
  [[nodiscard]] const std::vector<std::string>& alternatives() const noexcept {
    return alternatives_;
  }

  /** @return True if @p name is in the alternative list. */
  [[nodiscard]] bool has_alternative(const std::string& name) const;
  /**
   * @return Index of @p name.
   * @throws std::invalid_argument if not found.
   */
  [[nodiscard]] std::size_t index_of(const std::string& name) const;

  /**
   * @brief Appends an alternative with a missing rating.
   * @param name Alternative name to append.
   * @param ignore_existing If true, no-op when already present; else throws.
   */
  void add_alternative(const std::string& name, bool ignore_existing = false);

  /**
   * @brief Removes an alternative and its rating.
   * @param name Alternative name to remove.
   * @param ignore_missing If true, no-op when absent; else throws.
   */
  void remove_alternative(const std::string& name,
                          bool ignore_missing = false);

  /**
   * @brief Renames an alternative in place (keeps rating values).
   * @throws std::invalid_argument if @p old_name is missing or @p new_name clashes.
   */
  void rename_alternative(const std::string& old_name,
                          const std::string& new_name);

  /** @return Current storage mode. */
  [[nodiscard]] Mode mode() const noexcept { return mode_; }
  /** @brief Sets storage mode (does not clear the other mode's data). */
  void set_mode(Mode mode) { mode_ = mode; }

  /** @return Category definitions (categorical mode). */
  [[nodiscard]] const std::vector<RatingCategory>& categories() const noexcept {
    return categories_;
  }
  /**
   * @brief Replaces the category list.
   * @throws std::invalid_argument if any value is outside [0, 1] or ids clash.
   */
  void set_categories(std::vector<RatingCategory> categories);

  /** @return Score interpreter (numeric mode). */
  [[nodiscard]] const ScoreInterpreter& interpreter() const noexcept {
    return interpreter_;
  }
  /** @brief Sets the numeric score interpreter. */
  void set_interpreter(ScoreInterpreter interpreter) {
    interpreter_ = std::move(interpreter);
  }

  /**
   * @brief Sets a categorical rating for @p alt.
   * @param alt Alternative name.
   * @param category_id Must match a category id, or empty to clear.
   * @throws std::invalid_argument if alt or category id is unknown.
   */
  void set_rating(const std::string& alt, const std::string& category_id);
  /** @brief Clears the categorical rating for @p alt. */
  void clear_rating(const std::string& alt);
  /** @return Category id for @p alt, if set. */
  [[nodiscard]] std::optional<std::string> rating(
      const std::string& alt) const;

  /**
   * @brief Sets a numeric raw value for @p alt.
   * @throws std::invalid_argument if alt is unknown.
   */
  void set_value(const std::string& alt, double raw);
  /** @brief Clears the numeric value for @p alt. */
  void clear_value(const std::string& alt);
  /** @return Raw numeric value for @p alt, if set. */
  [[nodiscard]] std::optional<double> value(const std::string& alt) const;

  /**
   * @brief Intensity scores in [0, 1] (missing → 0).
   *
   * Categorical: category values. Numeric: interpreter applied to raw values.
   */
  [[nodiscard]] Vector scores() const;

  /**
   * @brief L1-normalized priorities over present (non-missing) scores.
   *
   * Missing alternatives contribute 0. Empty / all-missing → zero vector.
   */
  [[nodiscard]] Vector priorities() const;

private:
  [[nodiscard]] std::optional<double> intensity_at(std::size_t i) const;
  [[nodiscard]] const RatingCategory* find_category(
      const std::string& id) const;

  std::vector<std::string> alternatives_;
  Mode mode_ = Mode::Numeric;
  std::vector<RatingCategory> categories_;
  std::vector<std::optional<std::string>> categorical_;
  std::vector<std::optional<double>> numeric_;
  ScoreInterpreter interpreter_ = IdentityInterpreter{};
};

}  // namespace anpcpp
