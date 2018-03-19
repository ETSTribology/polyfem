#include "ElasticityUtils.hpp"


namespace poly_fem
{
	Eigen::VectorXd gradient_from_energy(const int size, const int n_bases, const ElementAssemblyValues &vals, const Eigen::MatrixXd &displacement, const Eigen::VectorXd &da,
		const std::function<DScalar1<double, Eigen::Matrix<double, 6, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun6,
		const std::function<DScalar1<double, Eigen::Matrix<double, 8, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun8,
		const std::function<DScalar1<double, Eigen::Matrix<double, 12, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun12,
		const std::function<DScalar1<double, Eigen::Matrix<double, 18, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun18,
		const std::function<DScalar1<double, Eigen::Matrix<double, 24, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun24,
		const std::function<DScalar1<double, Eigen::Matrix<double, 30, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun30,
		const std::function<DScalar1<double, Eigen::Matrix<double, 81, 1>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun81,
		const std::function<DScalar1<double, Eigen::VectorXd>				(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &funn
		)
	{
		Eigen::VectorXd grad;

		switch(size)
		{
			//2d
			case 2:
			{
				switch(n_bases)
				{
					//P1
					case 3:
					{
						auto auto_diff_energy = fun6(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
					//P2
					case 6:
					{
						auto auto_diff_energy = fun12(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
			 		//Q1
					case 4:
					{
						auto auto_diff_energy = fun8(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
					//Q2
					case 9:
					{
						auto auto_diff_energy = fun18(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
				}

				break;
			}


			//3d
			case 3:
			{
				switch(n_bases)
				{
					//P1
					case 4:
					{
						auto auto_diff_energy = fun12(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
					//P2
					case 10:
					{
						auto auto_diff_energy = fun30(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
					//Q1
					case 8:
					{
						auto auto_diff_energy = fun24(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
					//Q2
					case 27:
					{
						auto auto_diff_energy = fun81(vals, displacement, da);
						grad = auto_diff_energy.getGradient();
						break;
					}
				}
			}
		}


		if(grad.size()<=0)
		{
			static bool show_message = true;

			if(show_message)
			{
				std::cout<<"[Warning] "<<n_bases<<" not using static sizes"<<std::endl;
				show_message = false;
			}

			auto auto_diff_energy = funn(vals, displacement, da);
			grad = auto_diff_energy.getGradient();
		}

		return grad;
	}


	Eigen::MatrixXd hessian_from_energy(const int size, const int n_bases, const ElementAssemblyValues &vals, const Eigen::MatrixXd &displacement, const Eigen::VectorXd &da,
		const std::function<DScalar2<double, Eigen::Matrix<double, 6, 1>, Eigen::Matrix<double, 6, 6>>		(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun6,
		const std::function<DScalar2<double, Eigen::Matrix<double, 8, 1>, Eigen::Matrix<double, 8, 8>>		(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun8,
		const std::function<DScalar2<double, Eigen::Matrix<double, 12, 1>, Eigen::Matrix<double, 12, 12>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun12,
		const std::function<DScalar2<double, Eigen::Matrix<double, 18, 1>, Eigen::Matrix<double, 18, 18>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun18,
		const std::function<DScalar2<double, Eigen::Matrix<double, 24, 1>, Eigen::Matrix<double, 24, 24>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun24,
		const std::function<DScalar2<double, Eigen::Matrix<double, 30, 1>, Eigen::Matrix<double, 30, 30>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun30,
		// const std::function<DScalar2<double, Eigen::Matrix<double, 81, 1>, Eigen::Matrix<double, 81, 81>>	(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &fun81,
		const std::function<DScalar2<double, Eigen::VectorXd, Eigen::MatrixXd>								(const ElementAssemblyValues &, const Eigen::MatrixXd &, const Eigen::VectorXd &)> &funn
		)
	{
		Eigen::MatrixXd hessian;

		switch(size)
		{
			//2d
			case 2:
			{
				switch(n_bases)
				{
					//P1
					case 3:
					{
						auto auto_diff_energy = fun6(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}
					//P2
					case 6:
					{
						auto auto_diff_energy = fun12(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}
					//Q1
					case 4:
					{
						auto auto_diff_energy = fun8(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}
					//Q2
					case 9:
					{
						auto auto_diff_energy = fun18(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}
				}

				break;
			}


			//3d
			case 3:
			{
				switch(n_bases)
				{
					//P2
					case 4:
					{
						auto auto_diff_energy = fun12(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}
					//P2
					case 10:
					{
						auto auto_diff_energy = fun30(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}
			 		//Q1
					case 8:
					{
						auto auto_diff_energy = fun24(vals, displacement, da);
						hessian = auto_diff_energy.getHessian();
						break;
					}

					// // Q2
					// case 27:
					// {
					// 	auto auto_diff_energy = fun81(vals, displacement, da);
					// 	hessian = auto_diff_energy.getHessian();
					// 	break;
					// }
				}
			}
		}

		if(hessian.size() <= 0)
		{
			static bool show_message = true;

			if(show_message)
			{
				std::cout<<"[Warning] "<<n_bases<<" not using static sizes"<<std::endl;
				show_message = false;
			}

			auto auto_diff_energy = funn(vals, displacement, da);
			hessian = auto_diff_energy.getHessian();
		}

		// time.stop();
		// std::cout << "-- hessian: " << time.getElapsedTime() << std::endl;

		return hessian;
	}


	double von_mises_stress_for_stress_tensor(const Eigen::MatrixXd &stress)
	{
		double von_mises_stress =  0.5 * ( stress(0, 0) - stress(1, 1) ) * ( stress(0, 0) - stress(1, 1) ) + 3.0  *  stress(0, 1) * stress(0, 1);

		if(stress.rows() == 3)
		{
			von_mises_stress += 0.5 * (stress(2, 2) - stress(1, 1)) * (stress(2, 2) - stress(1, 1)) + 3.0  * stress(2, 1) * stress(2, 1);
			von_mises_stress += 0.5 * (stress(2, 2) - stress(0, 0)) * (stress(2, 2) - stress(0, 0)) + 3.0  * stress(2, 0) * stress(2, 0);
		}

		von_mises_stress = sqrt( fabs(von_mises_stress) );

		return von_mises_stress;
	}



	void ElasticityTensor::resize(const int size)
	{
		if(size == 2)
			stifness_tensor_.resize(6, 1);
		else
			stifness_tensor_.resize(21, 1);

		stifness_tensor_.setZero();

		size_ = size;
	}

	double ElasticityTensor::operator()(int i, int j) const
	{
		if(j < i)
		{
			int tmp=i;
			i = j;
			j = tmp;
		}

		assert(j>=i);
		const int n = size_ == 2 ? 3 : 6;
		assert(i < n);
		assert(j < n);
		assert(i >= 0);
		assert(j >= 0);
		const int index = n * i + j - i * (i + 1) / 2;
		assert(index < stifness_tensor_.size());

		return stifness_tensor_(index);
	}

	double &ElasticityTensor::operator()(int i, int j)
	{
		if(j < i)
		{
			int tmp=i;
			i = j;
			j = tmp;
		}

		assert(j>=i);
		const int n = size_ == 2 ? 3 : 6;
		assert(i < n);
		assert(j < n);
		assert(i >= 0);
		assert(j >= 0);
		const int index = n * i + j - i * (i + 1) / 2;
		assert(index < stifness_tensor_.size());

		return stifness_tensor_(index);
	}


	void ElasticityTensor::set_from_entries(const std::vector<double> &entries)
	{
		if(size_ == 2)
		{
			assert(entries.size() >= 6);

			(*this)(0, 0) = entries[0];
			(*this)(0, 1) = entries[1];
			(*this)(0, 2) = entries[2];

			(*this)(1, 1) = entries[3];
			(*this)(1, 2) = entries[4];

			(*this)(2, 2) = entries[5];
		}
		else
		{
			assert(entries.size() >= 21);

			(*this)(0, 0) = entries[0];
			(*this)(0, 1) = entries[1];
			(*this)(0, 2) = entries[2];
			(*this)(0, 3) = entries[3];
			(*this)(0, 4) = entries[4];
			(*this)(0, 5) = entries[5];

			(*this)(1, 1) = entries[6];
			(*this)(1, 2) = entries[7];
			(*this)(1, 3) = entries[8];
			(*this)(1, 4) = entries[9];
			(*this)(1, 5) = entries[10];

			(*this)(2, 2) = entries[11];
			(*this)(2, 3) = entries[12];
			(*this)(2, 4) = entries[13];
			(*this)(2, 5) = entries[14];

			(*this)(3, 3) = entries[15];
			(*this)(3, 4) = entries[16];
			(*this)(3, 5) = entries[17];

			(*this)(4, 4) = entries[18];
			(*this)(4, 5) = entries[19];

			(*this)(5, 5) = entries[20];

		}
	}


	void ElasticityTensor::set_from_lambda_mu(const double lambda, const double mu)
	{
		if(size_ == 2)
		{
			(*this)(0, 0) = 2*mu+lambda;
			(*this)(0, 1) = lambda;
			(*this)(0, 2) = 0;

			(*this)(1, 1) = 2*mu+lambda;
			(*this)(1, 2) = 0;

			(*this)(2, 2) = mu;
		}
		else
		{
			(*this)(0, 0) = 2*mu+lambda;
			(*this)(0, 1) = lambda;
			(*this)(0, 2) = lambda;
			(*this)(0, 3) = 0;
			(*this)(0, 4) = 0;
			(*this)(0, 5) = 0;

			(*this)(1, 1) = 2*mu+lambda;
			(*this)(1, 2) = lambda;
			(*this)(1, 3) = 0;
			(*this)(1, 4) = 0;
			(*this)(1, 5) = 0;

			(*this)(2, 2) = 2*mu+lambda;
			(*this)(2, 3) = 0;
			(*this)(2, 4) = 0;
			(*this)(2, 5) = 0;

			(*this)(3, 3) = mu;
			(*this)(3, 4) = 0;
			(*this)(3, 5) = 0;

			(*this)(4, 4) = mu;
			(*this)(4, 5) = 0;

			(*this)(5, 5) = mu;

		}
	}

	template<int DIM>
	double ElasticityTensor::compute_stress(const std::array<double, DIM> &strain, const int j) const
	{
		double res = 0;

		for(int k = 0; k < DIM; ++k)
			res += (*this)(j, k)*strain[k];

		return res;
	}


	//template instantiation
	template double ElasticityTensor::compute_stress<3>(const std::array<double, 3> &strain, const int j) const;
	template double ElasticityTensor::compute_stress<6>(const std::array<double, 6> &strain, const int j) const;

}