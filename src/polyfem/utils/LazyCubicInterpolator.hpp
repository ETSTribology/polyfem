#pragma once

#include <map>
#include <unordered_map>
#include <shared_mutex>
#include <Eigen/Dense>
#include <nanospline/BSpline.h>
#include <nanospline/BSplinePatch.h>

namespace polyfem
{

	class LazyCubicInterpolator
	{
	public:
		LazyCubicInterpolator(const int dim, const double delta) : dim_(dim), delta_(delta)
		{
			if (dim_ == 2)
			{
				cubic_mat.resize(16, 16);
				cubic_mat << 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-3, 3, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					2, -2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, -2, -1, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 1, 1, 0, 0,
					-3, 0, 3, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, -3, 0, 3, 0, 0, 0, 0, 0, -2, 0, -1, 0,
					9, -9, -9, 9, 6, 3, -6, -3, 6, -6, 3, -3, 4, 2, 2, 1,
					-6, 6, 6, -6, -3, -3, 3, 3, -4, 4, -2, 2, -2, -2, -1, -1,
					2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 1, 0,
					-6, 6, 6, -6, -4, -2, 4, 2, -3, 3, -3, 3, -2, -1, -2, -1,
					4, -4, -4, 4, 2, 2, -2, -2, 2, -2, 2, -2, 1, 1, 1, 1;
			}
			else if (dim_ == 3)
			{
				cubic_mat.resize(64, 64);
				cubic_mat << 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-3, 3, 0, 0, 0, 0, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					2, -2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					9, -9, -9, 9, 0, 0, 0, 0, 6, 3, -6, -3, 0, 0, 0, 0, 6, -6, 3, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-6, 6, 6, -6, 0, 0, 0, 0, -3, -3, 3, 3, 0, 0, 0, 0, -4, 4, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-6, 6, 6, -6, 0, 0, 0, 0, -4, -2, 4, 2, 0, 0, 0, 0, -3, 3, -3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -1, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					4, -4, -4, 4, 0, 0, 0, 0, 2, 2, -2, -2, 0, 0, 0, 0, 2, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, -9, -9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 3, -6, -3, 0, 0, 0, 0, 6, -6, 3, -3, 0, 0, 0, 0, 4, 2, 2, 1, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -6, 6, 6, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, -3, 3, 3, 0, 0, 0, 0, -4, 4, -2, 2, 0, 0, 0, 0, -2, -2, -1, -1, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -6, 6, 6, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -2, 4, 2, 0, 0, 0, 0, -3, 3, -3, 3, 0, 0, 0, 0, -2, -1, -2, -1, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -4, -4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, -2, -2, 0, 0, 0, 0, 2, -2, 2, -2, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
					-3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					9, -9, 0, 0, -9, 9, 0, 0, 6, 3, 0, 0, -6, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 3, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 2, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-6, 6, 0, 0, 6, -6, 0, 0, -3, -3, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 4, 0, 0, -2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, -1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, -9, 0, 0, -9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 3, 0, 0, -6, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, -6, 0, 0, 3, -3, 0, 0, 4, 2, 0, 0, 2, 1, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -6, 6, 0, 0, 6, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, -3, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 4, 0, 0, -2, 2, 0, 0, -2, -2, 0, 0, -1, -1, 0, 0,
					9, 0, -9, 0, -9, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 3, 0, -6, 0, -3, 0, 6, 0, -6, 0, 3, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 2, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 9, 0, -9, 0, -9, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 3, 0, -6, 0, -3, 0, 6, 0, -6, 0, 3, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 2, 0, 2, 0, 1, 0,
					-27, 27, 27, -27, 27, -27, -27, 27, -18, -9, 18, 9, 18, 9, -18, -9, -18, 18, -9, 9, 18, -18, 9, -9, -18, 18, 18, -18, -9, 9, 9, -9, -12, -6, -6, -3, 12, 6, 6, 3, -12, -6, 12, 6, -6, -3, 6, 3, -12, 12, -6, 6, -6, 6, -3, 3, -8, -4, -4, -2, -4, -2, -2, -1,
					18, -18, -18, 18, -18, 18, 18, -18, 9, 9, -9, -9, -9, -9, 9, 9, 12, -12, 6, -6, -12, 12, -6, 6, 12, -12, -12, 12, 6, -6, -6, 6, 6, 6, 3, 3, -6, -6, -3, -3, 6, 6, -6, -6, 3, 3, -3, -3, 8, -8, 4, -4, 4, -4, 2, -2, 4, 4, 2, 2, 2, 2, 1, 1,
					-6, 0, 6, 0, 6, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, -3, 0, 3, 0, 3, 0, -4, 0, 4, 0, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -2, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, -6, 0, 6, 0, 6, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 0, -3, 0, 3, 0, 3, 0, -4, 0, 4, 0, -2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -2, 0, -1, 0, -1, 0,
					18, -18, -18, 18, -18, 18, 18, -18, 12, 6, -12, -6, -12, -6, 12, 6, 9, -9, 9, -9, -9, 9, -9, 9, 12, -12, -12, 12, 6, -6, -6, 6, 6, 3, 6, 3, -6, -3, -6, -3, 8, 4, -8, -4, 4, 2, -4, -2, 6, -6, 6, -6, 3, -3, 3, -3, 4, 2, 4, 2, 2, 1, 2, 1,
					-12, 12, 12, -12, 12, -12, -12, 12, -6, -6, 6, 6, 6, 6, -6, -6, -6, 6, -6, 6, 6, -6, 6, -6, -8, 8, 8, -8, -4, 4, 4, -4, -3, -3, -3, -3, 3, 3, 3, 3, -4, -4, 4, 4, -2, -2, 2, 2, -4, 4, -4, 4, -2, 2, -2, 2, -2, -2, -2, -2, -1, -1, -1, -1,
					2, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					-6, 6, 0, 0, 6, -6, 0, 0, -4, -2, 0, 0, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -1, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					4, -4, 0, 0, -4, 4, 0, 0, 2, 2, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -6, 6, 0, 0, 6, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -2, 0, 0, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -2, -1, 0, 0, -2, -1, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -4, 0, 0, -4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 2, -2, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
					-6, 0, 6, 0, 6, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, -2, 0, 4, 0, 2, 0, -3, 0, 3, 0, -3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 0, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, -6, 0, 6, 0, 6, 0, -6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, -2, 0, 4, 0, 2, 0, -3, 0, 3, 0, -3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 0, -2, 0, -1, 0,
					18, -18, -18, 18, -18, 18, 18, -18, 12, 6, -12, -6, -12, -6, 12, 6, 12, -12, 6, -6, -12, 12, -6, 6, 9, -9, -9, 9, 9, -9, -9, 9, 8, 4, 4, 2, -8, -4, -4, -2, 6, 3, -6, -3, 6, 3, -6, -3, 6, -6, 3, -3, 6, -6, 3, -3, 4, 2, 2, 1, 4, 2, 2, 1,
					-12, 12, 12, -12, 12, -12, -12, 12, -6, -6, 6, 6, 6, 6, -6, -6, -8, 8, -4, 4, 8, -8, 4, -4, -6, 6, 6, -6, -6, 6, 6, -6, -4, -4, -2, -2, 4, 4, 2, 2, -3, -3, 3, 3, -3, -3, 3, 3, -4, 4, -2, 2, -4, 4, -2, 2, -2, -2, -1, -1, -2, -2, -1, -1,
					4, 0, -4, 0, -4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, -2, 0, -2, 0, 2, 0, -2, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 4, 0, -4, 0, -4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, -2, 0, -2, 0, 2, 0, -2, 0, 2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0,
					-12, 12, 12, -12, 12, -12, -12, 12, -8, -4, 8, 4, 8, 4, -8, -4, -6, 6, -6, 6, 6, -6, 6, -6, -6, 6, 6, -6, -6, 6, 6, -6, -4, -2, -4, -2, 4, 2, 4, 2, -4, -2, 4, 2, -4, -2, 4, 2, -3, 3, -3, 3, -3, 3, -3, 3, -2, -1, -2, -1, -2, -1, -2, -1,
					8, -8, -8, 8, -8, 8, 8, -8, 4, 4, -4, -4, -4, -4, 4, 4, 4, -4, 4, -4, -4, 4, -4, 4, 4, -4, -4, 4, 4, -4, -4, 4, 2, 2, 2, 2, -2, -2, -2, -2, 2, 2, -2, -2, 2, 2, -2, -2, 2, -2, 2, -2, 2, -2, 2, -2, 1, 1, 1, 1, 1, 1, 1, 1;
			}
		}

		void bicubic_interpolation(const Eigen::MatrixXd &corner_point, const std::vector<std::string> &keys, const Eigen::MatrixXd &point, double &val, Eigen::MatrixXd &grad) const;
		void tricubic_interpolation(const Eigen::MatrixXd &corner_point, const std::vector<std::string> &keys, const Eigen::MatrixXd &point, double &val, Eigen::MatrixXd &grad) const;
		void lazy_evaluate(std::function<void(const Eigen::MatrixXd &, double &)> compute_distance, const Eigen::MatrixXd &point, double &val, Eigen::MatrixXd &grad);
		void cache_grid(std::function<void(const Eigen::MatrixXd &, double &)> compute_distance, const Eigen::MatrixXd &point);
		void evaluate(const Eigen::MatrixXd &point, double &val, Eigen::MatrixXd &grad) const;

	private:
		inline void build_corner_keys(const Eigen::MatrixXd &point, Eigen::MatrixXi &keys) const
		{
			int num_corner_points = dim_ == 2 ? 4 : 8;
			Eigen::MatrixXi bin(dim_, 1);
			keys.resize(num_corner_points, dim_);
			std::vector<std::string> keys_string;
			for (int k = 0; k < dim_; ++k)
				bin(k) = (int)std::floor(point(k) / delta_);
			if (dim_ == 2)
			{
				keys << bin(0), bin(1),
					bin(0) + 1, bin(1),
					bin(0), bin(1) + 1,
					bin(0) + 1, bin(1) + 1;
			}
			else if (dim_ == 3)
			{
				keys << bin(0), bin(1), bin(2),
					bin(0) + 1, bin(1), bin(2),
					bin(0), bin(1) + 1, bin(2),
					bin(0) + 1, bin(1) + 1, bin(2),
					bin(0), bin(1), bin(2) + 1,
					bin(0) + 1, bin(1), bin(2) + 1,
					bin(0), bin(1) + 1, bin(2) + 1,
					bin(0) + 1, bin(1) + 1, bin(2) + 1;
			}
		}
		inline void setup_key(const Eigen::VectorXi &key, std::string &key_string, Eigen::MatrixXd &clamped_point) const
		{
			key_string = "";
			clamped_point.setZero(dim_, 1);
			for (int k = 0; k < dim_; ++k)
			{
				// key_string += fmt::format("{:d},", key(k));
				key_string += std::to_string(key(k)) + ",";
				clamped_point(k) = (double)key(k) * delta_;
			}
		};

		int dim_;
		double delta_;
		std::unordered_map<std::string, double> implicit_function_distance;
		std::unordered_map<std::string, Eigen::VectorXd> implicit_function_grads;

		Eigen::MatrixXd cubic_mat;

		mutable std::shared_mutex distance_mutex_;
		mutable std::shared_mutex grad_mutex_;
	};
} // namespace polyfem