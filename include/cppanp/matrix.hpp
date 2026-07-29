/**
 * @file matrix.hpp
 * @brief Dense row-major vectors and matrices.
 */

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace cppanp
 * @brief Analytic Network Process computational library.
 */
namespace cppanp {

/**
 * @brief Thrown when vector/matrix dimensions do not match an operation.
 */
class DimensionError : public std::runtime_error {
public:
  /**
   * @param message Human-readable description of the mismatch.
   */
  explicit DimensionError(const std::string& message)
      : std::runtime_error(message) {}
};

/**
 * @brief One-dimensional array of doubles with element-wise arithmetic.
 */
class Vector {
public:
  /** @brief Default-constructs an empty vector. */
  Vector() = default;

  /**
   * @brief Constructs a vector of the given length.
   * @param size Number of elements.
   * @param fill Initial value for every element (default 0).
   */
  explicit Vector(std::size_t size, double fill = 0.0);

  /** @brief Copy constructor (deep-copies element storage). */
  Vector(const Vector&) = default;
  /** @brief Move constructor. */
  Vector(Vector&&) noexcept = default;
  /** @brief Copy assignment. */
  Vector& operator=(const Vector&) = default;
  /** @brief Move assignment. */
  Vector& operator=(Vector&&) noexcept = default;

  /** @return Number of elements. */
  [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
  /** @return True if size() is zero. */
  [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

  /**
   * @brief Mutable element access.
   * @param i Zero-based index.
   * @return Reference to element @p i.
   * @throws DimensionError if @p i is out of range.
   */
  double& operator[](std::size_t i);

  /**
   * @brief Const element access.
   * @param i Zero-based index.
   * @return Const reference to element @p i.
   * @throws DimensionError if @p i is out of range.
   */
  [[nodiscard]] const double& operator[](std::size_t i) const;

  /** @return Pointer to contiguous element storage (mutable). */
  [[nodiscard]] double* data() noexcept { return data_.data(); }
  /** @return Pointer to contiguous element storage (const). */
  [[nodiscard]] const double* data() const noexcept { return data_.data(); }

  /** @brief Element-wise addition. @throws DimensionError on size mismatch. */
  [[nodiscard]] Vector operator+(const Vector& other) const;
  /** @brief Element-wise subtraction. @throws DimensionError on size mismatch. */
  [[nodiscard]] Vector operator-(const Vector& other) const;
  /** @brief Scalar multiplication (each element times @p scalar). */
  [[nodiscard]] Vector operator*(double scalar) const;

  /** @brief In-place element-wise addition. @throws DimensionError on size mismatch. */
  Vector& operator+=(const Vector& other);
  /** @brief In-place element-wise subtraction. @throws DimensionError on size mismatch. */
  Vector& operator-=(const Vector& other);
  /** @brief In-place scalar multiplication. */
  Vector& operator*=(double scalar);

  /** @return Sum of all elements. */
  [[nodiscard]] double sum() const;

  /** @brief Divides each element by sum(); no-op if sum is zero. */
  void normalize();

  /** @return A copy of this vector with elements L1-normalized. */
  [[nodiscard]] Vector normalized() const;

  /**
   * @brief Exact equality within absolute tolerance.
   * @param other Vector to compare.
   * @param abs_tol Maximum absolute difference per element.
   */
  [[nodiscard]] bool is_equal(const Vector& other,
                              double abs_tol = 1e-12) const;

  /**
   * @brief Approximate equality (absolute and relative tolerances).
   * @param other Vector to compare.
   * @param abs_tol Absolute tolerance per element.
   * @param rel_tol Relative tolerance scaled by element magnitudes.
   */
  [[nodiscard]] bool is_near(const Vector& other,
                             double abs_tol = 1e-9,
                             double rel_tol = 1e-9) const;

  /** @brief Scalar multiplication from the left (@p scalar * @p v). */
  friend Vector operator*(double scalar, const Vector& v);

private:
  std::vector<double> data_;

  void check_same_size(const Vector& other, const char* op) const;
};

/**
 * @brief Dense row-major matrix stored in a flat buffer.
 *
 * Element (i, j) is at index @c i * cols() + j.
 */
class Matrix {
public:
  /** @brief Default-constructs a 0x0 matrix. */
  Matrix() = default;

  /**
   * @brief Constructs an @p rows by @p cols matrix filled with @p fill.
   * @throws DimensionError if exactly one of rows or cols is zero.
   */
  Matrix(std::size_t rows, std::size_t cols, double fill = 0.0);

  /** @brief Copy constructor. */
  Matrix(const Matrix&) = default;
  /** @brief Move constructor. */
  Matrix(Matrix&&) noexcept = default;
  /** @brief Copy assignment. */
  Matrix& operator=(const Matrix&) = default;
  /** @brief Move assignment. */
  Matrix& operator=(Matrix&&) noexcept = default;

  /** @return @p n x @p n identity matrix. */
  [[nodiscard]] static Matrix identity(std::size_t n);
  /** @return Matrix filled with zeros. */
  [[nodiscard]] static Matrix zeros(std::size_t rows, std::size_t cols);
  /** @return Matrix filled with ones. */
  [[nodiscard]] static Matrix ones(std::size_t rows, std::size_t cols);

  /** @return Row count. */
  [[nodiscard]] std::size_t rows() const noexcept { return rows_; }
  /** @return Column count. */
  [[nodiscard]] std::size_t cols() const noexcept { return cols_; }
  /** @return True if either dimension is zero. */
  [[nodiscard]] bool empty() const noexcept { return rows_ == 0 || cols_ == 0; }

  /**
   * @brief Mutable element access.
   * @param i Row index (0-based).
   * @param j Column index (0-based).
   * @throws DimensionError if out of range.
   */
  double& operator()(std::size_t i, std::size_t j);

  /**
   * @brief Const element access.
   * @param i Row index (0-based).
   * @param j Column index (0-based).
   * @throws DimensionError if out of range.
   */
  [[nodiscard]] const double& operator()(std::size_t i, std::size_t j) const;

  /** @return Pointer to row-major contiguous storage (mutable). */
  [[nodiscard]] double* data() noexcept { return data_.data(); }
  /** @return Pointer to row-major contiguous storage (const). */
  [[nodiscard]] const double* data() const noexcept { return data_.data(); }

  /** @brief Element-wise addition. @throws DimensionError on shape mismatch. */
  [[nodiscard]] Matrix operator+(const Matrix& other) const;
  /** @brief Element-wise subtraction. @throws DimensionError on shape mismatch. */
  [[nodiscard]] Matrix operator-(const Matrix& other) const;
  /** @brief Scalar multiplication. */
  [[nodiscard]] Matrix operator*(double scalar) const;
  /**
   * @brief Matrix product (@c this * @p other).
   * @throws DimensionError if cols() != other.rows().
   */
  [[nodiscard]] Matrix operator*(const Matrix& other) const;
  /**
   * @brief Matrix-vector product.
   * @throws DimensionError if cols() != v.size().
   */
  [[nodiscard]] Vector operator*(const Vector& v) const;

  /** @brief In-place element-wise addition. */
  Matrix& operator+=(const Matrix& other);
  /** @brief In-place element-wise subtraction. */
  Matrix& operator-=(const Matrix& other);
  /** @brief In-place scalar multiplication. */
  Matrix& operator*=(double scalar);

  /** @return Transpose (cols x rows). */
  [[nodiscard]] Matrix transposed() const;

  /** @return Vector of row sums. */
  [[nodiscard]] Vector row_sums() const;
  /** @return Vector of column sums. */
  [[nodiscard]] Vector col_sums() const;

  /** @brief Divides each row by its sum; zero-sum rows are unchanged. */
  void normalize_rows();
  /** @brief Divides each column by its sum; zero-sum columns are unchanged. */
  void normalize_cols();

  /** @return Copy with rows L1-normalized. */
  [[nodiscard]] Matrix rows_normalized() const;
  /** @return Copy with columns L1-normalized. */
  [[nodiscard]] Matrix cols_normalized() const;

  /** @brief Per-element absolute tolerance equality. */
  [[nodiscard]] bool is_equal(const Matrix& other,
                              double abs_tol = 1e-12) const;
  /** @brief Absolute and relative tolerance equality. */
  [[nodiscard]] bool is_near(const Matrix& other,
                             double abs_tol = 1e-9,
                             double rel_tol = 1e-9) const;

  /** @brief Scalar multiplication from the left. */
  friend Matrix operator*(double scalar, const Matrix& m);

private:
  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::vector<double> data_;  // row-major

  [[nodiscard]] std::size_t index(std::size_t i, std::size_t j) const;
  void check_bounds(std::size_t i, std::size_t j) const;
  void check_same_shape(const Matrix& other, const char* op) const;
};

}  // namespace cppanp
