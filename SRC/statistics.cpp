#include "statistics.h"
#include <cmath>

void Variance::addValue(double value, double weight)
{
  double mean_old = mean_;
  w_sum_ += weight;
  w_sum_sq_ += weight * weight;
  mean_ = mean_old + (weight / w_sum_) * (value - mean_old);
  d_2_ += weight * (value - mean_old) * (value - mean_);
}

double Variance::mean() const
{
  return mean_;
}

double Variance::population_variance() const
{
  if (w_sum_ <= 0.0) return 0.0;
  return d_2_ / w_sum_;
}

double Variance::sample_variance() const
{
  if (w_sum_ <= 0.0) return 0.0;
  double n_eff = w_sum_ * w_sum_ / w_sum_sq_;
  if (n_eff <= 1.0) return 0.0;
  return d_2_ / (n_eff - 1.0);
}

void Statistics::addNeutron(Neutron const &n)
{
  double prob = n.Probability;
  nevents_++;
  intensity_ += prob;
  lambda_stat_.addValue(n.Wavelength, prob);
  for (int i = 0; i < 3; i++) {
    pos_sum_[i] += n.Position[i] * prob;
  }
  for (int i = 0; i < 3; i++) {
    vel_sum_[i] += n.Vector[i] * prob;
  }
  double const *v = n.Vector;
  double dy = atan2(v[1], sqrt(v[0] * v[0] + v[2] * v[2]) * (v[0] >= 0 ? 1.0 : -1.0));
  double dz = atan2(v[2], sqrt(v[0] * v[0] + v[1] * v[1]));
  div_abs_sum_[0] += fabs(dy) * 180 / M_PI * prob;
  div_abs_sum_[1] += fabs(dz) * 180 / M_PI * prob;
}

Statistics::Result Statistics::result() const
{
  Result res{};
  res.nevents = nevents_;
  res.intensity = intensity_;
  res.lambda_mean = lambda_stat_.mean();
  if (nevents_ == 0) return res;
  res.lambda_var = lambda_stat_.population_variance();
  if (intensity_ <= 0.0) return res;
  for (int i = 0; i < 3; i++) {
    res.position[i] = pos_sum_[i] / intensity_;
  }
  for (int i = 0; i < 3; i++) {
    res.direction[i] = vel_sum_[i] / intensity_;
  }
  for (int i = 0; i < 2; i++) {
    res.divergence[i] = div_abs_sum_[i] / intensity_;
  }
  return res;
}
