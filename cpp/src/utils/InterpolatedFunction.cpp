#include "InterpolatedFunction.hpp"


namespace poly_fem
{
	InterpolatedFunction2d::InterpolatedFunction2d(const Eigen::MatrixXd &fun, const Eigen::MatrixXd &pts, const Eigen::MatrixXi &tris)
	: fun_(fun), pts_(pts), tris_(tris)
	{
		assert(pts_.cols() == 2);
		assert(tris_.cols() == 3);

		tree_.init(pts_,tris_);
	}

	Eigen::MatrixXd InterpolatedFunction2d::interpolate(const Eigen::MatrixXd &pts) const
	{
		assert(pts.cols() == 2);

		Eigen::VectorXi I;
		igl::in_element(pts_, tris_, pts, tree_, I);

		Eigen::MatrixXd res(pts.rows(), fun_.cols());
		res.setZero();

		Eigen::MatrixXd bc;

		for(long i = 0; i < pts.rows(); ++i)
		{
			const auto index = I(i);

			if(index < 0)
			{
				continue;
			}

			const Eigen::MatrixXd pt = pts.row(i);
			const Eigen::MatrixXd A = pts_.row(tris_(index, 0));
			const Eigen::MatrixXd B = pts_.row(tris_(index, 1));
			const Eigen::MatrixXd C = pts_.row(tris_(index, 2));
			igl::barycentric_coordinates(pt, A, B, C, bc);

			for(int j = 0; j < 3; ++j)
				res.row(i) += fun_.row(tris_(index, j)) * bc(j);
		}

		return res;
	}
}