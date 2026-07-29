/**
 * @file synthesis.hpp
 * @brief Combining subnetwork alternative scores.
 */

#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace anpcpp {

/**
 * @brief How control-node subnetwork scores are combined.
 */
enum class SynthesisKind {
  /** Weighted average of subnet scores (pyanp default). */
  Additive,
  /** Product of subnet scores across control nodes, then normalize. */
  Multiplicative,
  /** Custom expression over subnet-host node names. */
  Custom,
};

/**
 * @brief Synthesis configuration for networks with subnetworks.
 */
struct SynthesisOptions {
  /** Selected synthesis method. */
  SynthesisKind kind = SynthesisKind::Additive;
  /**
   * Expression when kind == Custom. Identifiers are subnet-host node names.
   * Example: @c "Benefits / Costs".
   */
  std::string custom_expr;
};

/**
 * @brief Thrown when synthesis or expression evaluation fails.
 */
class SynthesisError : public std::runtime_error {
public:
  /**
   * @param message Description of the error.
   */
  explicit SynthesisError(const std::string& message)
      : std::runtime_error(message) {}
};

/**
 * @brief Weighted average synthesis (matches AnpNetwork::sum_subnetwork_formula).
 *
 * @param subnet_weights Weight per control subnet (host node name -> weight).
 * @param alt_scores Per-subnet alternative scores (subnet -> alt -> score).
 */
[[nodiscard]] std::map<std::string, double> synthesize_additive(
    const std::map<std::string, double>& subnet_weights,
    const std::map<std::string, std::map<std::string, double>>& alt_scores);

/**
 * @brief Multiplicative synthesis: product of score^weight per alt, then L1-normalize.
 */
[[nodiscard]] std::map<std::string, double> synthesize_multiplicative(
    const std::map<std::string, double>& subnet_weights,
    const std::map<std::string, std::map<std::string, double>>& alt_scores);

/**
 * @brief Custom expression synthesis per alternative.
 *
 * @param expression Arithmetic expression with subnet-host names as variables.
 * @param alt_scores Per-subnet alternative scores.
 * @param alt_order Output ordering of alternatives.
 */
[[nodiscard]] std::map<std::string, double> synthesize_custom(
    const std::string& expression,
    const std::map<std::string, std::map<std::string, double>>& alt_scores,
    const std::vector<std::string>& alt_order);

/**
 * @brief Dispatches on @ref SynthesisOptions::kind.
 */
[[nodiscard]] std::map<std::string, double> synthesize(
    const SynthesisOptions& options,
    const std::map<std::string, double>& subnet_weights,
    const std::map<std::string, std::map<std::string, double>>& alt_scores,
    const std::vector<std::string>& alt_order);

/**
 * @brief Evaluates a numeric expression with named variables.
 * @param expression Infix expression (+ - * / parentheses).
 * @param variables Name -> value substitutions.
 */
[[nodiscard]] double eval_expression(
    const std::string& expression,
    const std::map<std::string, double>& variables);

}  // namespace anpcpp
