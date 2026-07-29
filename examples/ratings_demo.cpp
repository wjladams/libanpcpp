// Ratings demo -- alternatives scored with RatingsPrioritizer columns.
//
//   Goal:         Best (pairwise equal weight on criteria)
//   Criteria:     Price (categorical Hi/Med/Low), Quality (numeric / max)
//   Alternatives: A, B, C
//
// Price and Quality rate the Alternatives cluster. The goal compares criteria
// with pairwise judgments so the full ANP calculation path runs.

#include <iostream>

#include "anp_print.hpp"
#include "anpcpp/network.hpp"
#include "anpcpp/ratings.hpp"

using namespace anpcpp;
using namespace anpcpp::examples;

int main() {
  print_header("Ratings demo: categorical + numeric prioritizers");

  AnpNetwork net;
  net.add_cluster("Goal");
  net.add_node("Goal", "Best");
  net.add_cluster("Criteria");
  for (const char* c : {"Price", "Quality"}) {
    net.add_node("Criteria", c);
  }
  for (const char* a : {"A", "B", "C"}) {
    net.add_node("Alternatives", a);
  }

  net.node_connect("Best", "Price");
  net.node_connect("Best", "Quality");
  net.set_node_comparison("Best", "Price", "Quality", 1.0);  // equal criteria

  // --- Price: categorical ratings toward Alternatives ---
  for (const char* a : {"A", "B", "C"}) net.node_connect("Price", a);
  net.set_node_prioritizer_kind("Price", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    RatingsPrioritizer* rt = net.node("Price").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Categorical);
    rt->set_categories({
        {"L", "Low", 0.2},
        {"M", "Medium", 0.5},
        {"H", "High", 1.0},
    });
    // Goodness of price: A best, C worst.
    rt->set_rating("A", "H");
    rt->set_rating("B", "M");
    rt->set_rating("C", "L");
  }

  // --- Quality: numeric ratings, divide by max ---
  for (const char* a : {"A", "B", "C"}) net.node_connect("Quality", a);
  net.set_node_prioritizer_kind("Quality", "Alternatives",
                                NodePrioritizerKind::Ratings);
  {
    RatingsPrioritizer* rt = net.node("Quality").node_ratings("Alternatives");
    rt->set_mode(RatingsPrioritizer::Mode::Numeric);
    rt->set_interpreter(DivideByMaxInterpreter{});
    rt->set_value("A", 40.0);
    rt->set_value("B", 80.0);
    rt->set_value("C", 100.0);
  }

  // Spot-check categorical L1 column: scores 1, 0.5, 0.2 → pris / 1.7
  {
    const RatingsPrioritizer* rt =
        net.node("Price").node_ratings("Alternatives");
    const Vector pris = rt->priorities();
    const double s = 1.0 + 0.5 + 0.2;
    std::cout << "\nSpot-check Price categorical priorities:\n";
    std::cout << "  A=" << pris[0] << " (expect " << (1.0 / s) << ")\n";
    std::cout << "  B=" << pris[1] << " (expect " << (0.5 / s) << ")\n";
    std::cout << "  C=" << pris[2] << " (expect " << (0.2 / s) << ")\n";
  }

  print_network_inputs(net);
  print_network_results(net);

  std::cout << "\nQualitative check: Price favors A > B > C; Quality favors "
               "C > B > A;\n"
               "with equal criteria weights, B is often competitive.\n";
  return 0;
}
