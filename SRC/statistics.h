/// \file
/// Neutron beam statistics

#include "defines.h"

#ifndef STATISTICS_H
#define STATISTICS_H

/// Weighted Welford's algorithm
class Variance
{
public:
  /// Add a (weighted) value to the statistics
  void addValue(double value, double weight = 1.0);
  /// Mean Value
  double mean() const;
  /// Population variance
  double population_variance() const;
  /// Sample variance
  double sample_variance() const;

private:
  double w_sum_ = 0.0;
  double w_sum_sq_ = 0.0;
  double mean_ = 0.0;
  double d_2_ = 0.0;
};


class Statistics
{
public:
  struct Result
  {
    uint64_t nevents{};     ///< total number of events
    double intensity{};     ///< total intensity
    double lambda_mean{};   ///< wavelength mean
    double lambda_var{};    ///< wavelength variance
    double position[3]{};   ///< beam center position
    double direction[3]{};  ///< mean beam direction
    double divergence[2]{}; ///< mean absolute divergence
  };

  /// add a neutron event to the statistics
  void addNeutron(Neutron const &n);
  /// calculate the end result
  Statistics::Result result() const;

private:
  int64_t nevents_{};
  double intensity_{};
  Variance lambda_stat_{};
  double pos_sum_[3]{};
  double vel_sum_[3]{};
  double div_abs_sum_[2]{};
};

#endif /* STATISTICS_H */
