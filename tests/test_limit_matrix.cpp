#include "anpcpp/limit_matrix.hpp"

#include <gtest/gtest.h>

using anpcpp::LimitMatrixOptions;
using anpcpp::Matrix;
using anpcpp::Vector;

namespace {

Matrix make_matrix(std::size_t rows,
                   std::size_t cols,
                   std::initializer_list<double> values) {
  Matrix m(rows, cols);
  auto it = values.begin();
  for (std::size_t i = 0; i < rows; ++i) {
    for (std::size_t j = 0; j < cols; ++j) {
      m(i, j) = *it++;
    }
  }
  return m;
}

}  // namespace

TEST(LimitMatrixTest, ColumnNormalizeLeavesZeroColumn) {
  Matrix m = make_matrix(2, 2, {1.0, 0.0, 3.0, 0.0});
  anpcpp::column_normalize_inplace(m);
  EXPECT_DOUBLE_EQ(m(0, 0), 0.25);
  EXPECT_DOUBLE_EQ(m(1, 0), 0.75);
  EXPECT_DOUBLE_EQ(m(0, 1), 0.0);
  EXPECT_DOUBLE_EQ(m(1, 1), 0.0);
}

TEST(LimitMatrixTest, HierarchyFormulaForStrictHierarchy) {
  // Nodes: Goal -> C1,C2 -> A1 (sinks). Column-stochastic hierarchy.
  // Order: Goal, C1, C2, A1
  // Goal compares C1:C2 as 2:1 -> column [0, 2/3, 1/3, 0]
  // C1 and C2 each point only to A1
  Matrix W = make_matrix(4, 4,
                         {0, 0, 0, 0,
                          2.0 / 3.0, 0, 0, 0,
                          1.0 / 3.0, 0, 0, 0,
                          0, 1, 1, 0});

  const Matrix L = anpcpp::hierarchy_formula(W);
  ASSERT_FALSE(L.empty());

  // Hand: W + W^2 + W^3, then column normalize.
  // summ col0 = [0, 2/3, 1/3, 1], sum = 2
  EXPECT_NEAR(L(0, 0), 0.0, 1e-12);
  EXPECT_NEAR(L(1, 0), 1.0 / 3.0, 1e-9);
  EXPECT_NEAR(L(2, 0), 1.0 / 6.0, 1e-9);
  EXPECT_NEAR(L(3, 0), 0.5, 1e-9);
}

TEST(LimitMatrixTest, CalculusOnColumnStochasticMatrix) {
  // Simple Markov matrix with unique stationary distribution.
  Matrix W = make_matrix(2, 2, {0.0, 0.5, 1.0, 0.5});
  LimitMatrixOptions options;
  options.start_pow = 8;
  const Matrix L = anpcpp::calculus_limit(W, options);

  // Stationary pi satisfies pi = W pi for columns that converged: each column
  // of the limit should be the same distribution [1/3, 2/3].
  EXPECT_NEAR(L(0, 0), 1.0 / 3.0, 1e-6);
  EXPECT_NEAR(L(1, 0), 2.0 / 3.0, 1e-6);
  EXPECT_NEAR(L(0, 1), 1.0 / 3.0, 1e-6);
  EXPECT_NEAR(L(1, 1), 2.0 / 3.0, 1e-6);
}

TEST(LimitMatrixTest, PriorityFromLimitRowSums) {
  Matrix L = make_matrix(2, 2, {0.5, 0.5, 0.5, 0.5});
  const Vector p = anpcpp::priority_from_limit(L);
  EXPECT_NEAR(p[0], 0.5, 1e-12);
  EXPECT_NEAR(p[1], 0.5, 1e-12);
}

TEST(LimitMatrixTest, IdentityLimitIsIdentity) {
  const Matrix I = Matrix::identity(3);
  LimitMatrixOptions options;
  options.start_pow = 4;
  const Matrix L = anpcpp::calculus_limit(I, options);
  EXPECT_TRUE(L.is_near(I, 1e-8, 1e-8));
}

namespace {

// Tutorial matrix from pyanp.org/tutorials/limitmatrix.html §3.3
Matrix tutorial_matrix2() {
  return make_matrix(5, 5,
                     {0.5, 0.3, 0.4, 0.0, 0.0,
                      0.1, 0.2, 0.2, 0.0, 0.0,
                      0.1, 0.1, 0.1, 0.0, 0.0,
                      0.2, 0.3, 0.1, 0.0, 0.0,
                      0.1, 0.1, 0.2, 0.0, 0.0});
}

void expect_column_matches(const Matrix& L,
                           std::size_t col,
                           std::initializer_list<double> expected,
                           double tol = 1e-6) {
  ASSERT_EQ(expected.size(), L.rows());
  auto it = expected.begin();
  for (std::size_t i = 0; i < L.rows(); ++i, ++it) {
    EXPECT_NEAR(L(i, col), *it, tol) << "row " << i << " col " << col;
  }
}

}  // namespace

TEST(LimitMatrixTest, HierarchyNodesFindsSinkColumns) {
  const Matrix W = tutorial_matrix2();
  const auto hier = anpcpp::hierarchy_nodes(W);
  ASSERT_EQ(hier.size(), 2u);
  EXPECT_EQ(hier[0], 3u);
  EXPECT_EQ(hier[1], 4u);
}

TEST(LimitMatrixTest, LimitNewHierarchyWithoutLimitMatchesPyanp) {
  LimitMatrixOptions options;
  options.method = anpcpp::LimitMatrixMethod::NewHierarchy;
  options.with_limit = false;
  const Matrix L = anpcpp::limit_newhierarchy(tutorial_matrix2(), options);

  // Reference: pyanp.limitmatrix.limit_newhierarchy(mat, with_limit=False)
  for (std::size_t j = 0; j < 3; ++j) {
    expect_column_matches(L, j,
                          {0.3276551126513143, 0.0988405418235169,
                           0.0735043455251688, 0.3206499239533065,
                           0.1793500760466935});
  }
  for (std::size_t j = 3; j < 5; ++j) {
    expect_column_matches(L, j, {0.0, 0.0, 0.0, 0.0, 0.0});
  }
}

TEST(LimitMatrixTest, LimitNewHierarchyWithLimitMatchesPyanp) {
  LimitMatrixOptions options;
  options.method = anpcpp::LimitMatrixMethod::NewHierarchy;
  options.with_limit = true;
  const Matrix L = anpcpp::limit_newhierarchy(tutorial_matrix2(), options);

  // Reference: pyanp.limitmatrix.limit_newhierarchy(mat, with_limit=True)
  for (std::size_t j = 0; j < 3; ++j) {
    expect_column_matches(L, j,
                          {0.4965343692950798, 0.1497847102033525,
                           0.1113897889474636, 0.1553812658147681,
                           0.086909865739336});
  }
}

TEST(LimitMatrixTest, LimitSinksStraightNormalizerMatchesPyanp) {
  LimitMatrixOptions options;
  options.method = anpcpp::LimitMatrixMethod::Sinks;
  options.straight_normalizer = true;
  const Matrix L = anpcpp::limit_sinks(tutorial_matrix2(), options);

  for (std::size_t j = 0; j < 3; ++j) {
    expect_column_matches(L, j,
                          {0.4965343692950798, 0.1497847102033525,
                           0.1113897889474636, 0.1553812658147681,
                           0.086909865739336});
  }
}

TEST(LimitMatrixTest, LimitSinksNoStraightNormalizerMatchesPyanp) {
  LimitMatrixOptions options;
  options.method = anpcpp::LimitMatrixMethod::Sinks;
  options.straight_normalizer = false;
  const Matrix L = anpcpp::limit_sinks(tutorial_matrix2(), options);

  for (std::size_t j = 0; j < 3; ++j) {
    expect_column_matches(L, j,
                          {0.6553102253026286, 0.1976810836470339,
                           0.1470086910503375, 0.2050672392596697,
                           0.1147008691050338});
  }
}

TEST(LimitMatrixTest, ComputeLimitMatrixDispatchesByMethod) {
  const Matrix W = tutorial_matrix2();

  LimitMatrixOptions sinks_opts;
  sinks_opts.method = anpcpp::LimitMatrixMethod::Sinks;
  const Matrix via_dispatch = anpcpp::compute_limit_matrix(W, sinks_opts);
  const Matrix direct = anpcpp::limit_sinks(W, sinks_opts);
  EXPECT_TRUE(via_dispatch.is_near(direct, 1e-12, 1e-12));

  LimitMatrixOptions nh_opts;
  nh_opts.method = anpcpp::LimitMatrixMethod::NewHierarchy;
  nh_opts.with_limit = false;
  const Matrix via_nh = anpcpp::compute_limit_matrix(W, nh_opts);
  const Matrix direct_nh = anpcpp::limit_newhierarchy(W, nh_opts);
  EXPECT_TRUE(via_nh.is_near(direct_nh, 1e-12, 1e-12));
}
