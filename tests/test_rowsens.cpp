#include "anpcpp/rowsens.hpp"
#include "anpcpp/network.hpp"

#include <gtest/gtest.h>

#include <cmath>

using anpcpp::Matrix;
using anpcpp::P0Mode;
using anpcpp::priority_after_row_adjust;
using anpcpp::row_adjust;

namespace {

Matrix mat4() {
  // From pyanp rowsensitivity tutorial.
  Matrix m(4, 4, 0.0);
  const double d[4][4] = {{0.5, 0.1, 0.0, 0.0},
                          {0.2, 0.6, 0.0, 0.0},
                          {0.1, 0.05, 0.75, 0.1},
                          {0.2, 0.25, 0.25, 0.9}};
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 4; ++j) m(i, j) = d[i][j];
  }
  return m;
}

}  // namespace

TEST(RowSensTest, RowAdjustMatchesPyanpDirectP0) {
  const Matrix adj = row_adjust(mat4(), 0, 0.1, P0Mode::Direct(0.5));
  EXPECT_NEAR(adj(0, 0), 0.1, 1e-6);
  EXPECT_NEAR(adj(0, 1), 0.02, 1e-6);
  EXPECT_NEAR(adj(1, 0), 0.36, 1e-6);
  EXPECT_NEAR(adj(1, 1), 0.653333, 1e-5);
  EXPECT_NEAR(adj(2, 0), 0.18, 1e-6);
  EXPECT_NEAR(adj(2, 1), 0.054444, 1e-5);
  EXPECT_NEAR(adj(3, 0), 0.36, 1e-6);
  EXPECT_NEAR(adj(3, 1), 0.272222, 1e-5);
  // Unaffected columns preserved.
  EXPECT_NEAR(adj(2, 2), 0.75, 1e-12);
  EXPECT_NEAR(adj(3, 3), 0.9, 1e-12);
}

TEST(RowSensTest, PriorityAfterAdjustAtP0MatchesOriginalShape) {
  const Matrix m = mat4();
  const auto pri = priority_after_row_adjust(m, 0, 0.5, P0Mode::Direct(0.5));
  ASSERT_EQ(pri.size(), 4u);
  double sum = 0.0;
  for (std::size_t i = 0; i < pri.size(); ++i) sum += pri[i];
  EXPECT_NEAR(sum, 1.0, 1e-8);
}

TEST(RowSensTest, InfluenceRawHasUpDownAroundP0) {
  const Matrix m = mat4();
  const std::vector<std::size_t> alts = {2, 3};
  const std::vector<std::string> names = {"A", "B"};
  const auto rows =
      anpcpp::influence_raw(m, 0, alts, names, 0.1, 0.1, 0.5);
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_EQ(rows[0].name, "A");
  EXPECT_NEAR(rows[0].up_diff, rows[0].up_score - rows[0].original, 1e-12);
  EXPECT_NEAR(rows[0].down_diff, rows[0].down_score - rows[0].original,
              1e-12);
  double sum = rows[0].original + rows[1].original;
  EXPECT_NEAR(sum, 1.0, 1e-8);
}

TEST(RowSensTest, InfluenceMarginalSmartReturnsOnePerAlt) {
  const Matrix m = mat4();
  const std::vector<std::size_t> alts = {2, 3};
  const std::vector<std::string> names = {"A", "B"};
  const auto rows = anpcpp::influence_marginal_smart(m, 0, alts, names);
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_GT(rows[0].smart_p0, 0.0);
  EXPECT_LT(rows[0].smart_p0, 1.0);
}

TEST(RowSensTest, NetworkSubnetPriorityAtPChangesWithP) {
  using anpcpp::AnpNetwork;
  AnpNetwork net(/*create_alts_cluster=*/false);
  net.add_cluster("Goal");
  net.add_cluster("Control");
  net.add_node("Goal", "Choose");
  net.add_node("Control", "Benefits");
  net.add_node("Control", "Costs");
  net.node_connect("Choose", "Benefits");
  net.node_connect("Choose", "Costs");
  net.set_node_comparison("Choose", "Benefits", "Costs", 2.0);

  auto build = [](AnpNetwork& sub, const char* f1, const char* f2) {
    sub.add_cluster("Factors");
    sub.add_cluster("Alternatives");
    sub.set_alternatives_cluster("Alternatives");
    sub.add_node("Factors", f1);
    sub.add_node("Factors", f2);
    for (const char* a : {"Plan1", "Plan2", "Plan3"}) {
      sub.add_node("Alternatives", a);
    }
    for (const std::string& f : {std::string(f1), std::string(f2)}) {
      for (const char* a : {"Plan1", "Plan2", "Plan3"}) {
        sub.node_connect(f, a);
      }
    }
  };

  AnpNetwork& ben = net.subnet("Benefits");
  build(ben, "Performance", "Convenience");
  ben.set_node_comparison("Performance", "Plan1", "Plan2", 2.0);
  ben.set_node_comparison("Performance", "Plan1", "Plan3", 4.0);
  ben.set_node_comparison("Performance", "Plan2", "Plan3", 2.0);

  AnpNetwork& cost = net.subnet("Costs");
  build(cost, "Money", "Risk");
  cost.set_node_comparison("Money", "Plan3", "Plan2", 2.0);
  cost.set_node_comparison("Money", "Plan3", "Plan1", 4.0);
  net.node("Costs").set_invert(true);

  const auto low = net.priority_map_at_p("Benefits", 0.1);
  const auto mid = net.priority_map_at_p("Benefits", 0.5);
  const auto high = net.priority_map_at_p("Benefits", 0.9);
  ASSERT_EQ(low.size(), 3u);
  // Resting p=0.5 should match ordinary synthesis.
  const auto base = net.priority_map();
  EXPECT_NEAR(mid.at("Plan1"), base.at("Plan1"), 1e-6);
  // Sweeping p should move alternative scores when subnet weights change.
  EXPECT_GT(std::abs(high.at("Plan1") - low.at("Plan1")) +
                std::abs(high.at("Plan2") - low.at("Plan2")) +
                std::abs(high.at("Plan3") - low.at("Plan3")),
            1e-6);

  const auto raw = net.influence_raw("Benefits", 0.2, 0.2, 0.5);
  ASSERT_EQ(raw.size(), 3u);
  bool any_diff = false;
  for (const auto& r : raw) {
    if (std::abs(r.up_diff) > 1e-9 || std::abs(r.down_diff) > 1e-9) {
      any_diff = true;
    }
  }
  EXPECT_TRUE(any_diff);

  const auto nodes = net.node_names();
  const auto rank = net.influence_rank();
  ASSERT_EQ(rank.size(), nodes.size());
  for (std::size_t i = 0; i < rank.size(); ++i) {
    EXPECT_EQ(rank[i].name, nodes[i]);
    EXPECT_GE(rank[i].rank_influence, 0.0);
    EXPECT_LE(rank[i].rank_influence, 1.0);
  }

  const auto marg = net.influence_marginal_smart();
  ASSERT_EQ(marg.size(), nodes.size());
  for (std::size_t i = 0; i < marg.size(); ++i) {
    EXPECT_EQ(marg[i].name, nodes[i]);
    EXPECT_GT(marg[i].smart_p0, 0.0);
    EXPECT_LT(marg[i].smart_p0, 1.0);
    EXPECT_GE(marg[i].marginal, 0.0);
  }

  const auto total = net.influence_total(0.25);
  ASSERT_EQ(total.size(), nodes.size());
  bool any_total = false;
  for (std::size_t i = 0; i < total.size(); ++i) {
    EXPECT_EQ(total[i].name, nodes[i]);
    EXPECT_GE(total[i].total_influence, 0.0);
    EXPECT_GE(total[i].max_alt_change, 0.0);
    EXPECT_LE(total[i].max_alt_change, total[i].total_influence + 1e-12);
    if (total[i].total_influence > 1e-9) any_total = true;
  }
  EXPECT_TRUE(any_total);

  // Total for Benefits should match L1 of |up - orig| from raw-like fixed delta.
  const auto mid_pri = net.priority_at_p("Benefits", 0.5);
  const auto up_pri = net.priority_at_p("Benefits", 0.75);
  double expected_l1 = 0.0;
  double expected_max = 0.0;
  for (std::size_t i = 0; i < mid_pri.size(); ++i) {
    const double d = std::abs(up_pri[i] - mid_pri[i]);
    expected_l1 += d;
    if (d > expected_max) expected_max = d;
  }
  const auto* benefits_total = [&]() -> const anpcpp::InfluenceTotalEntry* {
    for (const auto& t : total) {
      if (t.name == "Benefits") return &t;
    }
    return nullptr;
  }();
  ASSERT_NE(benefits_total, nullptr);
  EXPECT_NEAR(benefits_total->total_influence, expected_l1, 1e-9);
  EXPECT_NEAR(benefits_total->max_alt_change, expected_max, 1e-9);
}

TEST(RowSensTest, MatrixInfluenceTotalMatchesAbsDiffL1) {
  const Matrix m = mat4();
  const std::vector<std::size_t> alts = {2, 3};
  const std::vector<std::size_t> rows = {0, 1};
  const std::vector<std::string> names = {"R0", "R1"};
  const auto totals = anpcpp::influence_total(m, rows, names, alts, 0.25, 0.5);
  ASSERT_EQ(totals.size(), 2u);
  EXPECT_EQ(totals[0].name, "R0");
  EXPECT_GE(totals[0].total_influence, 0.0);
  EXPECT_LE(totals[0].max_alt_change, totals[0].total_influence + 1e-12);

  const auto one = anpcpp::influence_total_row(m, 0, alts, 0.25, 0.5);
  EXPECT_NEAR(one.total_influence, totals[0].total_influence, 1e-12);
  EXPECT_NEAR(one.max_alt_change, totals[0].max_alt_change, 1e-12);
}
