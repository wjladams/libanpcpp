#include "anpcpp/ratings.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <string>

#include "anpcpp/json_io.hpp"
#include "anpcpp/network.hpp"

using anpcpp::AnpNetwork;
using anpcpp::DivideByConstantInterpreter;
using anpcpp::DivideByMaxInterpreter;
using anpcpp::IdentityInterpreter;
using anpcpp::MinMaxNormalizeInterpreter;
using anpcpp::NodePrioritizerKind;
using anpcpp::PiecewiseLinearInterpreter;
using anpcpp::RatingCategory;
using anpcpp::RatingsPrioritizer;
using anpcpp::Vector;
using anpcpp::apply_score_interpreter;
using anpcpp::network_from_json;
using anpcpp::network_to_json;

TEST(RatingsTest, CategoricalScoresAndPriorities) {
  RatingsPrioritizer rt({"A", "B", "C"});
  rt.set_mode(RatingsPrioritizer::Mode::Categorical);
  rt.set_categories({
      {"L", "Low", 0.2},
      {"M", "Med", 0.5},
      {"H", "High", 1.0},
  });
  rt.set_rating("A", "H");
  rt.set_rating("B", "M");
  rt.set_rating("C", "L");

  const Vector scores = rt.scores();
  EXPECT_NEAR(scores[0], 1.0, 1e-12);
  EXPECT_NEAR(scores[1], 0.5, 1e-12);
  EXPECT_NEAR(scores[2], 0.2, 1e-12);

  const Vector pris = rt.priorities();
  const double sum = 1.0 + 0.5 + 0.2;
  EXPECT_NEAR(pris[0], 1.0 / sum, 1e-12);
  EXPECT_NEAR(pris[1], 0.5 / sum, 1e-12);
  EXPECT_NEAR(pris[2], 0.2 / sum, 1e-12);
  EXPECT_NEAR(pris[0] + pris[1] + pris[2], 1.0, 1e-12);
}

TEST(RatingsTest, MissingIgnoredInL1) {
  RatingsPrioritizer rt({"A", "B", "C"});
  rt.set_mode(RatingsPrioritizer::Mode::Categorical);
  rt.set_categories({{"H", "High", 1.0}, {"L", "Low", 0.0}});
  rt.set_rating("A", "H");
  // B missing, C Low=0 still present with score 0
  rt.set_rating("C", "L");

  const Vector pris = rt.priorities();
  EXPECT_NEAR(pris[0], 1.0, 1e-12);  // only A contributes to sum (1+0)
  EXPECT_NEAR(pris[1], 0.0, 1e-12);
  EXPECT_NEAR(pris[2], 0.0, 1e-12);
}

TEST(RatingsTest, DivideByMax) {
  RatingsPrioritizer rt({"A", "B", "C"});
  rt.set_mode(RatingsPrioritizer::Mode::Numeric);
  rt.set_interpreter(DivideByMaxInterpreter{});
  rt.set_value("A", 10.0);
  rt.set_value("B", 5.0);
  rt.set_value("C", 2.0);

  const Vector scores = rt.scores();
  EXPECT_NEAR(scores[0], 1.0, 1e-12);
  EXPECT_NEAR(scores[1], 0.5, 1e-12);
  EXPECT_NEAR(scores[2], 0.2, 1e-12);
}

TEST(RatingsTest, DivideByConstant) {
  RatingsPrioritizer rt({"A", "B"});
  rt.set_interpreter(DivideByConstantInterpreter{4.0});
  rt.set_value("A", 4.0);
  rt.set_value("B", 2.0);
  const Vector scores = rt.scores();
  EXPECT_NEAR(scores[0], 1.0, 1e-12);
  EXPECT_NEAR(scores[1], 0.5, 1e-12);
}

TEST(RatingsTest, MinMaxNormalize) {
  RatingsPrioritizer rt({"A", "B", "C"});
  rt.set_interpreter(MinMaxNormalizeInterpreter{});
  rt.set_value("A", 10.0);
  rt.set_value("B", 5.0);
  rt.set_value("C", 0.0);
  const Vector scores = rt.scores();
  EXPECT_NEAR(scores[0], 1.0, 1e-12);
  EXPECT_NEAR(scores[1], 0.5, 1e-12);
  EXPECT_NEAR(scores[2], 0.0, 1e-12);
}

TEST(RatingsTest, PiecewiseLinear) {
  const auto out = apply_score_interpreter(
      PiecewiseLinearInterpreter{{{0.0, 0.0}, {10.0, 1.0}}},
      {std::optional<double>{5.0}, std::optional<double>{15.0},
       std::nullopt});
  ASSERT_TRUE(out[0].has_value());
  EXPECT_NEAR(*out[0], 0.5, 1e-12);
  ASSERT_TRUE(out[1].has_value());
  EXPECT_NEAR(*out[1], 1.0, 1e-12);  // clamped
  EXPECT_FALSE(out[2].has_value());
}

TEST(RatingsTest, IdentityIsDirect) {
  RatingsPrioritizer rt({"A", "B"});
  rt.set_interpreter(IdentityInterpreter{});
  rt.set_value("A", 0.75);
  rt.set_value("B", 0.25);
  const Vector pris = rt.priorities();
  EXPECT_NEAR(pris[0], 0.75, 1e-12);
  EXPECT_NEAR(pris[1], 0.25, 1e-12);
}

TEST(RatingsTest, NetworkUnscaledColumn) {
  AnpNetwork net;
  net.add_cluster("Criteria");
  net.add_node("Criteria", "Quality");
  net.add_node("Alternatives", "A");
  net.add_node("Alternatives", "B");
  net.add_node("Alternatives", "C");

  net.node_connect("Quality", "A");
  net.node_connect("Quality", "B");
  net.node_connect("Quality", "C");
  net.set_node_prioritizer_kind("Quality", "Alternatives",
                                NodePrioritizerKind::Ratings);

  RatingsPrioritizer* rt =
      net.node("Quality").node_ratings("Alternatives");
  ASSERT_NE(rt, nullptr);
  rt->set_mode(RatingsPrioritizer::Mode::Categorical);
  rt->set_categories(
      {{"L", "Low", 0.2}, {"M", "Med", 0.5}, {"H", "High", 1.0}});
  rt->set_rating("A", "H");
  rt->set_rating("B", "M");
  rt->set_rating("C", "L");

  EXPECT_EQ(net.node("Quality").node_pairwise("Alternatives"), nullptr);

  const Vector col = net.node("Quality").unscaled_column();
  const double sum = 1.0 + 0.5 + 0.2;
  const auto names = net.node_names();
  ASSERT_EQ(names.size(), 4u);
  std::size_t iA = 0, iB = 0, iC = 0, iQ = 0;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == "A") iA = i;
    if (names[i] == "B") iB = i;
    if (names[i] == "C") iC = i;
    if (names[i] == "Quality") iQ = i;
  }
  EXPECT_NEAR(col[iQ], 0.0, 1e-12);
  EXPECT_NEAR(col[iA], 1.0 / sum, 1e-12);
  EXPECT_NEAR(col[iB], 0.5 / sum, 1e-12);
  EXPECT_NEAR(col[iC], 0.2 / sum, 1e-12);
}

TEST(RatingsTest, SetNodeComparisonThrowsOnRatings) {
  AnpNetwork net;
  net.add_cluster("Criteria");
  net.add_node("Criteria", "Q");
  net.add_node("Alternatives", "A");
  net.add_node("Alternatives", "B");
  net.node_connect("Q", "A");
  net.node_connect("Q", "B");
  net.set_node_prioritizer_kind("Q", "Alternatives",
                                NodePrioritizerKind::Ratings);
  EXPECT_THROW(net.set_node_comparison("Q", "A", "B", 2.0), std::logic_error);
}

TEST(RatingsTest, JsonRoundTripRatings) {
  AnpNetwork net;
  net.add_cluster("Criteria");
  net.add_node("Criteria", "Price");
  net.add_node("Criteria", "Quality");
  net.add_node("Alternatives", "A");
  net.add_node("Alternatives", "B");
  net.add_node("Alternatives", "C");

  net.node_connect("Price", "A");
  net.node_connect("Price", "B");
  net.node_connect("Price", "C");
  net.set_node_prioritizer_kind("Price", "Alternatives",
                                NodePrioritizerKind::Ratings);
  RatingsPrioritizer* price =
      net.node("Price").node_ratings("Alternatives");
  price->set_mode(RatingsPrioritizer::Mode::Categorical);
  price->set_categories(
      {{"L", "Low", 0.2}, {"M", "Med", 0.5}, {"H", "High", 1.0}});
  price->set_rating("A", "H");
  price->set_rating("B", "M");
  price->set_rating("C", "L");

  net.node_connect("Quality", "A");
  net.node_connect("Quality", "B");
  net.node_connect("Quality", "C");
  net.set_node_prioritizer_kind("Quality", "Alternatives",
                                NodePrioritizerKind::Ratings);
  RatingsPrioritizer* quality =
      net.node("Quality").node_ratings("Alternatives");
  quality->set_mode(RatingsPrioritizer::Mode::Numeric);
  quality->set_interpreter(DivideByMaxInterpreter{});
  quality->set_value("A", 2.0);
  quality->set_value("B", 10.0);
  quality->set_value("C", 5.0);

  const std::string json = network_to_json(net);
  EXPECT_NE(json.find("node_prioritizers"), std::string::npos);
  EXPECT_NE(json.find("\"type\": \"ratings\""), std::string::npos);

  auto loaded = network_from_json(json);
  ASSERT_NE(loaded, nullptr);
  EXPECT_EQ(loaded->node("Price").node_prioritizer_kind("Alternatives"),
            NodePrioritizerKind::Ratings);
  const RatingsPrioritizer* lp =
      loaded->node("Price").node_ratings("Alternatives");
  ASSERT_NE(lp, nullptr);
  EXPECT_EQ(lp->mode(), RatingsPrioritizer::Mode::Categorical);
  EXPECT_EQ(lp->rating("A"), std::optional<std::string>("H"));
  EXPECT_TRUE(loaded->unscaled_supermatrix().is_near(net.unscaled_supermatrix(),
                                                     1e-9, 1e-9));
}

TEST(RatingsTest, LegacyNodePairwiseStillLoads) {
  const char* legacy = R"({
    "format": "anpcpp",
    "version": 1,
    "network": {
      "clusters": [
        {
          "name": "Alternatives",
          "description": "",
          "cluster_pairwise": {"alternatives": [], "matrix": []},
          "nodes": [
            {"name": "A", "description": "", "invert": false, "node_pairwise": {}},
            {"name": "B", "description": "", "invert": false, "node_pairwise": {}}
          ]
        },
        {
          "name": "Criteria",
          "description": "",
          "cluster_pairwise": {"alternatives": [], "matrix": []},
          "nodes": [
            {
              "name": "Price",
              "description": "",
              "invert": false,
              "node_pairwise": {
                "Alternatives": {
                  "alternatives": ["A", "B"],
                  "matrix": [[1.0, 2.0], [0.5, 1.0]]
                }
              }
            }
          ]
        }
      ],
      "alternatives_cluster": "Alternatives",
      "synthesis": {"kind": "additive", "custom_expr": ""}
    }
  })";
  auto loaded = network_from_json(legacy);
  ASSERT_NE(loaded->node("Price").node_pairwise("Alternatives"), nullptr);
  EXPECT_NEAR(loaded->node("Price")
                  .node_pairwise("Alternatives")
                  ->comparison("A", "B"),
              2.0, 1e-12);
}
