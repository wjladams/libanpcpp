#include <cmath>
#include <string>

#include <gtest/gtest.h>

#include "anpcpp/json_io.hpp"
#include "anpcpp/multiuser.hpp"
#include "anpcpp/network.hpp"
#include "anpcpp/ratings.hpp"

using namespace anpcpp;

namespace {

AnpNetwork make_pairwise_ahp() {
  AnpNetwork net;
  net.add_cluster("Criteria");
  net.add_node("Criteria", "Goal");
  for (const char* a : {"A", "B", "C"}) {
    net.add_node("Alternatives", a);
  }
  net.node_connect("Goal", "A");
  net.node_connect("Goal", "B");
  net.node_connect("Goal", "C");
  return net;
}

}  // namespace

TEST(Multiuser, PairwiseGeometricMean) {
  AnpNetwork net = make_pairwise_ahp();
  net.add_participant("u1", "Alice");
  net.add_participant("u2", "Bob");
  net.add_participant("u3", "Carol");

  // A vs B: 2, 8, 1/2 → geo mean = (2*8*0.5)^(1/3) = 8^(1/3) = 2
  net.set_node_comparison_for("u1", "Goal", "A", "B", 2.0);
  net.set_node_comparison_for("u2", "Goal", "A", "B", 8.0);
  net.set_node_comparison_for("u3", "Goal", "A", "B", 0.5);
  net.set_node_comparison_for("u1", "Goal", "A", "C", 3.0);
  net.set_node_comparison_for("u2", "Goal", "A", "C", 3.0);
  net.set_node_comparison_for("u3", "Goal", "A", "C", 3.0);
  net.set_node_comparison_for("u1", "Goal", "B", "C", 1.0);
  net.set_node_comparison_for("u2", "Goal", "B", "C", 1.0);
  net.set_node_comparison_for("u3", "Goal", "B", "C", 1.0);

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();

  const PairwiseJudgments* pw = net.node("Goal").node_pairwise("Alternatives");
  ASSERT_NE(pw, nullptr);
  EXPECT_NEAR(pw->comparison("A", "B"), 2.0, 1e-9);
  EXPECT_NEAR(pw->comparison("A", "C"), 3.0, 1e-9);
}

TEST(Multiuser, RatingsArithmeticMean) {
  AnpNetwork net = make_pairwise_ahp();
  net.add_participant("u1", "Alice");
  net.add_participant("u2", "Bob");

  NodePrioritizerSlot* slot =
      net.node("Goal").node_prioritizer("Alternatives");
  ASSERT_NE(slot, nullptr);
  net.set_node_prioritizer_kind("Goal", "Alternatives",
                                NodePrioritizerKind::Ratings);
  slot = net.node("Goal").node_prioritizer("Alternatives");
  slot->ratings.set_mode(RatingsPrioritizer::Mode::Numeric);
  slot->ratings.set_interpreter(IdentityInterpreter{});
  slot->sync_ratings_scale_to_users();

  net.set_node_rating_value_for("u1", "Goal", "A", 0.2);
  net.set_node_rating_value_for("u2", "Goal", "A", 0.8);
  net.set_node_rating_value_for("u1", "Goal", "B", 0.5);
  net.set_node_rating_value_for("u2", "Goal", "B", 0.5);

  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();

  const RatingsPrioritizer* rt = net.node("Goal").node_ratings("Alternatives");
  ASSERT_NE(rt, nullptr);
  ASSERT_TRUE(rt->value("A").has_value());
  EXPECT_NEAR(*rt->value("A"), 0.5, 1e-9);
  EXPECT_NEAR(*rt->value("B"), 0.5, 1e-9);
}

TEST(Multiuser, IndividualScopeAndGroup) {
  AnpNetwork net = make_pairwise_ahp();
  net.add_participant("u1", "Alice");
  net.add_participant("u2", "Bob");
  net.add_judgment_group("exec", "Executives", {"u1", "u2"});

  net.set_node_comparison_for("u1", "Goal", "A", "B", 9.0);
  net.set_node_comparison_for("u2", "Goal", "A", "B", 1.0);
  net.set_node_comparison_for("u1", "Goal", "A", "C", 1.0);
  net.set_node_comparison_for("u2", "Goal", "A", "C", 1.0);
  net.set_node_comparison_for("u1", "Goal", "B", "C", 1.0);
  net.set_node_comparison_for("u2", "Goal", "B", "C", 1.0);

  net.set_judgment_session({JudgmentScopeKind::Participant, "u1"});
  net.rebuild_effective_judgments();
  EXPECT_NEAR(net.node("Goal").node_pairwise("Alternatives")->comparison("A", "B"),
              9.0, 1e-9);

  net.set_judgment_session({JudgmentScopeKind::Group, "exec"});
  net.rebuild_effective_judgments();
  // geo mean of 9 and 1 = 3
  EXPECT_NEAR(net.node("Goal").node_pairwise("Alternatives")->comparison("A", "B"),
              3.0, 1e-9);
}

TEST(Multiuser, JsonRoundTrip) {
  AnpNetwork net = make_pairwise_ahp();
  net.add_participant("u1", "Alice", "a@x.com");
  net.add_participant("u2", "Bob");
  net.add_judgment_group("exec", "Executives", {"u1", "u2"});
  net.set_node_comparison_for("u1", "Goal", "A", "B", 4.0);
  net.set_node_comparison_for("u2", "Goal", "A", "B", 1.0);
  net.set_node_comparison_for("u1", "Goal", "A", "C", 2.0);
  net.set_node_comparison_for("u2", "Goal", "A", "C", 2.0);
  net.set_node_comparison_for("u1", "Goal", "B", "C", 1.0);
  net.set_node_comparison_for("u2", "Goal", "B", "C", 1.0);
  net.set_judgment_session({JudgmentScopeKind::Average, {}});
  net.rebuild_effective_judgments();

  const std::string json = network_to_json(net);
  auto loaded = network_from_json(json);
  ASSERT_EQ(loaded->participants().size(), 2u);
  EXPECT_EQ(loaded->participants()[0].email, "a@x.com");
  ASSERT_EQ(loaded->judgment_groups().size(), 1u);
  EXPECT_NEAR(
      loaded->node("Goal").node_pairwise("Alternatives")->comparison("A", "B"),
      2.0, 1e-9);
}

TEST(Multiuser, V1StillLoads) {
  AnpNetwork net = make_pairwise_ahp();
  net.set_node_comparison("Goal", "A", "B", 3.0);
  net.set_node_comparison("Goal", "A", "C", 1.0);
  net.set_node_comparison("Goal", "B", "C", 1.0);
  std::string j = network_to_json(net);
  auto pos = j.find("\"version\": 2");
  ASSERT_NE(pos, std::string::npos);
  j.replace(pos, std::string("\"version\": 2").size(), "\"version\": 1");
  auto loaded = network_from_json(j);
  EXPECT_FALSE(loaded->participants().empty());
  EXPECT_NEAR(
      loaded->node("Goal").node_pairwise("Alternatives")->comparison("A", "B"),
      3.0, 1e-9);
}

TEST(Multiuser, SamplePairwiseGolden) {
  const std::string path =
      std::string(ANPCPP_SAMPLES_DIR) + "/18_multiuser_pairwise_ahp.json";
  auto net = load_network_file(path);
  ASSERT_NE(net, nullptr);
  EXPECT_NEAR(
      net->node("Goal").node_pairwise("Alternatives")->comparison("A", "B"),
      2.0, 1e-9);
}

TEST(Multiuser, SampleRatingsGolden) {
  const std::string path =
      std::string(ANPCPP_SAMPLES_DIR) + "/19_multiuser_ratings.json";
  auto net = load_network_file(path);
  ASSERT_NE(net, nullptr);
  const auto* rt = net->node("Quality").node_ratings("Alternatives");
  ASSERT_NE(rt, nullptr);
  ASSERT_TRUE(rt->value("A").has_value());
  EXPECT_NEAR(*rt->value("A"), 0.5, 1e-9);
}

TEST(Multiuser, SamplePartialSkipsMissing) {
  const std::string path =
      std::string(ANPCPP_SAMPLES_DIR) + "/21_multiuser_partial.json";
  auto net = load_network_file(path);
  ASSERT_NE(net, nullptr);
  EXPECT_NEAR(
      net->node("Goal").node_pairwise("Alternatives")->comparison("A", "B"),
      2.0, 1e-9);
}

TEST(Multiuser, PairwiseDisagreementMaxMinRatio) {
  // Sample-18 style: A vs B = 2, 8, 0.5 → max/min = 16
  PairwiseJudgments a({"A", "B", "C"});
  PairwiseJudgments b({"A", "B", "C"});
  PairwiseJudgments c({"A", "B", "C"});
  a.set_comparison("A", "B", 2.0);
  b.set_comparison("A", "B", 8.0);
  c.set_comparison("A", "B", 0.5);
  a.set_comparison("A", "C", 3.0);
  b.set_comparison("A", "C", 3.0);
  c.set_comparison("A", "C", 3.0);
  a.set_comparison("B", "C", 1.0);
  b.set_comparison("B", "C", 1.0);
  // c missing B vs C → only 2 contributors, ratio = 1

  const auto d =
      pairwise_disagreement({&a, &b, &c});
  ASSERT_EQ(d.size(), 3u);
  EXPECT_EQ(d[0][1].contributor_count, 3);
  EXPECT_NEAR(d[0][1].min, 0.5, 1e-9);
  EXPECT_NEAR(d[0][1].max, 8.0, 1e-9);
  EXPECT_NEAR(d[0][1].ratio, 16.0, 1e-9);

  EXPECT_EQ(d[0][2].contributor_count, 3);
  EXPECT_NEAR(d[0][2].ratio, 1.0, 1e-9);

  EXPECT_EQ(d[1][2].contributor_count, 2);
  EXPECT_NEAR(d[1][2].ratio, 1.0, 1e-9);
}

TEST(Multiuser, RatingsDisagreementRange) {
  RatingsPrioritizer a({"A", "B"});
  RatingsPrioritizer b({"A", "B"});
  a.set_mode(RatingsPrioritizer::Mode::Numeric);
  b.set_mode(RatingsPrioritizer::Mode::Numeric);
  a.set_interpreter(IdentityInterpreter{});
  b.set_interpreter(IdentityInterpreter{});
  a.set_value("A", 0.2);
  b.set_value("A", 0.8);
  a.set_value("B", 0.5);
  // b missing B

  const auto d = ratings_disagreement({&a, &b});
  ASSERT_EQ(d.size(), 2u);
  EXPECT_EQ(d[0].alt, "A");
  EXPECT_EQ(d[0].contributor_count, 2);
  EXPECT_NEAR(d[0].range, 0.6, 1e-9);
  EXPECT_EQ(d[1].contributor_count, 1);
  EXPECT_NEAR(d[1].range, 0.0, 1e-9);
}

TEST(Multiuser, FillCounts) {
  PairwiseJudgments pw({"A", "B", "C"});
  pw.set_comparison("A", "B", 2.0);
  // A-C and B-C empty
  const JudgmentFillCounts pc = pairwise_fill_counts(pw);
  EXPECT_EQ(pc.needed, 3u);
  EXPECT_EQ(pc.filled, 1u);

  RatingsPrioritizer rt({"A", "B", "C"});
  rt.set_mode(RatingsPrioritizer::Mode::Numeric);
  rt.set_interpreter(IdentityInterpreter{});
  rt.set_value("A", 0.4);
  const JudgmentFillCounts rc = ratings_fill_counts(rt);
  EXPECT_EQ(rc.needed, 3u);
  EXPECT_EQ(rc.filled, 1u);
}

TEST(Multiuser, PairwiseVoteSpreadAlignment) {
  // 2, 8, 0.5 → max/min 16 → alignment ≈ 36.9%
  const auto s = summarize_pairwise_votes({2.0, 8.0, 0.5});
  EXPECT_EQ(s.contributor_count, 3);
  EXPECT_NEAR(s.mean, 2.0, 1e-9);
  EXPECT_NEAR(s.alignment_pct, pairwise_alignment_pct(0.5, 8.0), 1e-9);
  EXPECT_NEAR(s.alignment_pct, 100.0 * (1.0 - std::log(16.0) / std::log(81.0)),
              1e-6);
  EXPECT_NEAR(s.band_low, 0.5, 1e-6);
  EXPECT_NEAR(s.band_high, 8.0, 1e-6);

  const auto alike = summarize_pairwise_votes({3.0, 3.0, 3.0});
  EXPECT_NEAR(alike.alignment_pct, 100.0, 1e-9);
  EXPECT_NEAR(alike.mean, 3.0, 1e-9);

  const auto skewed = summarize_pairwise_votes({3.0, 3.0, 3.0, 4.0});
  EXPECT_NEAR(skewed.mean, std::pow(3.0 * 3.0 * 3.0 * 4.0, 0.25), 1e-9);

  EXPECT_NEAR(pairwise_alignment_pct(1.0 / 9.0, 9.0), 0.0, 1e-9);
}

TEST(Multiuser, RatingsVoteSpreadAlignment) {
  const auto s = summarize_ratings_votes({0.2, 0.8, 0.5}, 1.0);
  EXPECT_EQ(s.contributor_count, 3);
  EXPECT_NEAR(s.mean, 0.5, 1e-9);
  EXPECT_NEAR(s.alignment_pct, 40.0, 1e-9);  // range 0.6 → 40%
}
