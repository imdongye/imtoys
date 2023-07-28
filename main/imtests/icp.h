#ifndef ICP_H
#define ICP_H

#include "Eigen/Eigen"
#include <vector>

// cols:nr_points
void icp_init(const Eigen::MatrixXd& src, const Eigen::MatrixXd& dst);

//Eigen::Matrix4d icp_iterate();

Eigen::Matrix4d icp_step(const Eigen::MatrixXd &A, const Eigen::MatrixXd &B);

Eigen::Matrix4d icp(const Eigen::MatrixXd &A, const Eigen::MatrixXd &B, int max_iterations=20, int tolerance = 0.001);

#endif