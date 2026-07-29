/**
 * @file pairwise.hpp
 * @brief Named pairwise comparison tables.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "cppanp/eigen.hpp"
#include "cppanp/inconsistency.hpp"
#include "cppanp/matrix.hpp"

namespace cppanp {

/**
 * @brief Square pairwise comparison table over a named list of alternatives.
 */
class PairwiseJudgments {
public:
  /** @brief Default-constructs an empty table. */
  PairwiseJudgments() = default;

  /**
   * @brief Constructs a table with the given alternative names.
   * @param alternatives Ordered list of names (diagonal initialized to 1).
   */
  explicit PairwiseJudgments(std::vector<std::string> alternatives);

  /** @return Number of alternatives (matrix order). */
  [[nodiscard]] std::size_t size() const noexcept { return alternatives_.size(); }
  /** @return True if there are no alternatives. */
  [[nodiscard]] bool empty() const noexcept { return alternatives_.empty(); }
  /** @return Ordered alternative names. */
  [[nodiscard]] const std::vector<std::string>& alternatives() const noexcept {
    return alternatives_;
  }
  /** @return Underlying comparison matrix. */
  [[nodiscard]] const Matrix& matrix() const noexcept { return matrix_; }

  /** @return True if @p name is in the alternative list. */
  [[nodiscard]] bool has_alternative(const std::string& name) const;
  /**
   * @return Index of @p name.
   * @throws std::out_of_range if not found.
   */
  [[nodiscard]] std::size_t index_of(const std::string& name) const;

  /**
   * @brief Appends an alternative (diagonal 1).
   * @param name Alternative name.
   * @param ignore_existing If true, no-op when already present; else throws.
   */
  void add_alternative(const std::string& name, bool ignore_existing = false);

  /**
   * @brief Removes an alternative and its row/column.
   * @param name Alternative to remove.
   * @param ignore_missing If true, no-op when absent; else throws.
   */
  void remove_alternative(const std::string& name, bool ignore_missing = false);

  /**
   * @brief Sets comparison by index.
   * @param i Row index.
   * @param j Column index.
   * @param value @c a_ij; reciprocal set for @c a_ji unless 0 (incomplete).
   */
  void set_comparison(std::size_t i, std::size_t j, double value);

  /**
   * @brief Sets comparison by alternative name.
   * @param a First alternative.
   * @param b Second alternative.
   * @param value Comparison value (reciprocal applied automatically).
   */
  void set_comparison(const std::string& a,
                      const std::string& b,
                      double value);

  /** @return Comparison value at (@p i, @p j). */
  [[nodiscard]] double comparison(std::size_t i, std::size_t j) const;
  /** @return Comparison value for alternatives @p a and @p b. */
  [[nodiscard]] double comparison(const std::string& a,
                                  const std::string& b) const;

  /** @return Normalized priority vector from the comparison matrix. */
  [[nodiscard]] Vector priorities(const EigenOptions& options = {}) const;

  /** @return Saaty consistency ratio. */
  [[nodiscard]] double consistency_ratio(
      ConsistencyOptions options = {}) const;

private:
  std::vector<std::string> alternatives_;
  Matrix matrix_;
};

}  // namespace cppanp
