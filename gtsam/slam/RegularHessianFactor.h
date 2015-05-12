/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file   RegularHessianFactor.h
 * @brief  HessianFactor class with constant sized blocks
 * @author Sungtae An
 * @date   March 2014
 */

#pragma once

#include <gtsam/linear/HessianFactor.h>
#include <boost/foreach.hpp>
#include <vector>

namespace gtsam {

template<size_t D>
class RegularHessianFactor: public HessianFactor {

private:

  // Use Eigen magic to access raw memory
  typedef Eigen::Matrix<double, D, 1> DVector;
  typedef Eigen::Map<DVector> DMap;
  typedef Eigen::Map<const DVector> ConstDMap;

public:

  /** Construct an n-way factor.  Gs contains the upper-triangle blocks of the
   * quadratic term (the Hessian matrix) provided in row-order, gs the pieces
   * of the linear vector term, and f the constant term.
   */
  RegularHessianFactor(const std::vector<Key>& js,
      const std::vector<Matrix>& Gs, const std::vector<Vector>& gs, double f) :
      HessianFactor(js, Gs, gs, f) {
  }

  /** Construct a binary factor.  Gxx are the upper-triangle blocks of the
   * quadratic term (the Hessian matrix), gx the pieces of the linear vector
   * term, and f the constant term.
   */
  RegularHessianFactor(Key j1, Key j2, const Matrix& G11, const Matrix& G12,
      const Vector& g1, const Matrix& G22, const Vector& g2, double f) :
      HessianFactor(j1, j2, G11, G12, g1, G22, g2, f) {
  }

  /** Constructor with an arbitrary number of keys and with the augmented information matrix
   *   specified as a block matrix. */
  template<typename KEYS>
  RegularHessianFactor(const KEYS& keys,
      const SymmetricBlockMatrix& augmentedInformation) :
      HessianFactor(keys, augmentedInformation) {
  }

  // Scratch space for multiplyHessianAdd
  mutable std::vector<DVector> y;

  /** y += alpha * A'*A*x */
  virtual void multiplyHessianAdd(double alpha, const VectorValues& x,
      VectorValues& y) const {
    HessianFactor::multiplyHessianAdd(alpha, x, y);
  }

  /** y += alpha * A'*A*x */
  void multiplyHessianAdd(double alpha, const double* x,
      double* yvalues) const {
    // Create a vector of temporary y values, corresponding to rows i
    y.resize(size());
    BOOST_FOREACH(DVector & yi, y)
      yi.setZero();

    // Accessing the VectorValues one by one is expensive
    // So we will loop over columns to access x only once per column
    // And fill the above temporary y values, to be added into yvalues after
    DVector xj(D);
    for (DenseIndex j = 0; j < (DenseIndex) size(); ++j) {
      Key key = keys_[j];
      const double* xj = x + key * D;
      DenseIndex i = 0;
      for (; i < j; ++i)
        y[i] += info_(i, j).knownOffDiagonal() * ConstDMap(xj);
      // blocks on the diagonal are only half
      y[i] += info_(j, j).selfadjointView() * ConstDMap(xj);
      // for below diagonal, we take transpose block from upper triangular part
      for (i = j + 1; i < (DenseIndex) size(); ++i)
        y[i] += info_(i, j).knownOffDiagonal() * ConstDMap(xj);
    }

    // copy to yvalues
    for (DenseIndex i = 0; i < (DenseIndex) size(); ++i) {
      Key key = keys_[i];
      DMap(yvalues + key * D) += alpha * y[i];
    }
  }

  /// Raw memory version, with offsets TODO document reasoning
  void multiplyHessianAdd(double alpha, const double* x, double* yvalues,
      std::vector<size_t> offsets) const {

    // Create a vector of temporary y values, corresponding to rows i
    std::vector<Vector> y;
    y.reserve(size());
    for (const_iterator it = begin(); it != end(); it++)
      y.push_back(zero(getDim(it)));

    // Accessing the VectorValues one by one is expensive
    // So we will loop over columns to access x only once per column
    // And fill the above temporary y values, to be added into yvalues after
    for (DenseIndex j = 0; j < (DenseIndex) size(); ++j) {
      DenseIndex i = 0;
      for (; i < j; ++i)
        y[i] += info_(i, j).knownOffDiagonal()
            * ConstDMap(x + offsets[keys_[j]],
                offsets[keys_[j] + 1] - offsets[keys_[j]]);
      // blocks on the diagonal are only half
      y[i] += info_(j, j).selfadjointView()
          * ConstDMap(x + offsets[keys_[j]],
              offsets[keys_[j] + 1] - offsets[keys_[j]]);
      // for below diagonal, we take transpose block from upper triangular part
      for (i = j + 1; i < (DenseIndex) size(); ++i)
        y[i] += info_(i, j).knownOffDiagonal()
            * ConstDMap(x + offsets[keys_[j]],
                offsets[keys_[j] + 1] - offsets[keys_[j]]);
    }

    // copy to yvalues
    for (DenseIndex i = 0; i < (DenseIndex) size(); ++i)
      DMap(yvalues + offsets[keys_[i]],
          offsets[keys_[i] + 1] - offsets[keys_[i]]) += alpha * y[i];
  }

  /** Return the diagonal of the Hessian for this factor (raw memory version) */
  virtual void hessianDiagonal(double* d) const {

    // Loop over all variables in the factor
    for (DenseIndex pos = 0; pos < (DenseIndex) size(); ++pos) {
      Key j = keys_[pos];
      // Get the diagonal block, and insert its diagonal
      const Matrix& B = info_(pos, pos).selfadjointView();
      DMap(d + D * j) += B.diagonal();
    }
  }

  /// Add gradient at zero to d TODO: is it really the goal to add ??
  virtual void gradientAtZero(double* d) const {

    // Loop over all variables in the factor
    for (DenseIndex pos = 0; pos < (DenseIndex) size(); ++pos) {
      Key j = keys_[pos];
      // Get the diagonal block, and insert its diagonal
      DVector dj = -info_(pos, size()).knownOffDiagonal();
      DMap(d + D * j) += dj;
    }
  }

  /* ************************************************************************* */

};
// end class RegularHessianFactor

// traits
template<size_t D> struct traits<RegularHessianFactor<D> > : public Testable<
    RegularHessianFactor<D> > {
};

}
