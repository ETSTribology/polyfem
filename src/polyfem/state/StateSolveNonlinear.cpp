#include <polyfem/State.hpp>

#include <polyfem/assembler/Mass.hpp>
#include <polyfem/assembler/ViscousDamping.hpp>

#include <polyfem/solver/forms/BodyForm.hpp>
#include <polyfem/solver/forms/ContactForm.hpp>
#include <polyfem/solver/forms/ElasticForm.hpp>
#include <polyfem/solver/forms/FrictionForm.hpp>
#include <polyfem/solver/forms/InertiaForm.hpp>
#include <polyfem/solver/forms/LaggedRegForm.hpp>
#include <polyfem/solver/forms/RayleighDampingForm.hpp>

#include <polyfem/solver/NonlinearSolver.hpp>
#include <polyfem/solver/LBFGSSolver.hpp>
#include <polyfem/solver/SparseNewtonDescentSolver.hpp>
#include <polyfem/solver/NLProblem.hpp>
#include <polyfem/solver/ALSolver.hpp>
#include <polyfem/solver/SolveData.hpp>
#include <polyfem/io/MshWriter.hpp>
#include <polyfem/io/OBJWriter.hpp>
#ifdef POLYFEM_WITH_REMESHING
#include <polyfem/mesh/remesh/Remesh.hpp>
#endif
#include <polyfem/utils/MatrixUtils.hpp>
#include <polyfem/utils/Timer.hpp>
#include <polyfem/utils/JSONUtils.hpp>

#include <ipc/ipc.hpp>

extern "C" size_t getPeakRSS();

namespace polyfem
{
	using namespace mesh;
	using namespace solver;
	using namespace time_integrator;
	using namespace io;
	using namespace utils;

	template <typename ProblemType>
	std::shared_ptr<cppoptlib::NonlinearSolver<ProblemType>> State::make_nl_solver(
		const std::string &linear_solver_type) const
	{
		const std::string name = args["solver"]["nonlinear"]["solver"];
		const double dt = problem->is_time_dependent() ? args["time"]["dt"].get<double>() : 1.0;
		if (name == "newton" || name == "Newton")
		{
			json linear_solver_params = args["solver"]["linear"];
			if (!linear_solver_type.empty())
				linear_solver_params["solver"] = linear_solver_type;
			return std::make_shared<cppoptlib::SparseNewtonDescentSolver<ProblemType>>(
				args["solver"]["nonlinear"], linear_solver_params, dt);
		}
		else if (name == "lbfgs" || name == "LBFGS" || name == "L-BFGS")
		{
			return std::make_shared<cppoptlib::LBFGSSolver<ProblemType>>(args["solver"]["nonlinear"], dt);
		}
		else
		{
			throw std::invalid_argument(fmt::format("invalid nonlinear solver type: {}", name));
		}
	}

	void State::solve_transient_tensor_nonlinear(const int time_steps, const double t0, const double dt, Eigen::MatrixXd &sol)
	{
		init_nonlinear_tensor_solve(sol, t0 + dt);

		const bool remesh_enabled = args["space"]["remesh"]["enabled"];
		const double save_dt = remesh_enabled ? (dt / 3) : dt;
		int save_i = 0;

		save_timestep(t0, save_i++, t0, save_dt, sol, Eigen::MatrixXd()); // no pressure

		// Write the total energy to a CSV file
		std::ofstream energy_file(resolve_output_path("energy.csv"));
		energy_file << "i,elastic_energy,body_energy,inertia,contact_form,AL_lagr_energy,AL_pen_energy,total_energy" << std::endl;
		const auto save_energy = [&](int i) {
			energy_file << fmt::format(
				"{},{},{},{},{},{},{},{}\n", i,
				solve_data.elastic_form->value(sol),
				solve_data.body_form->value(sol),
				solve_data.inertia_form ? solve_data.inertia_form->value(sol) : 0,
				solve_data.contact_form ? solve_data.contact_form->value(sol) : 0,
				solve_data.al_lagr_form->value(sol),
				solve_data.al_pen_form->value(sol),
				solve_data.nl_problem->value(sol));
			energy_file.flush();
		};

		std::ofstream stats_file(resolve_output_path("stats.csv"));
		stats_file << "step,time,forward,remeshing,global_relaxation,peak_mem,#V,#T" << std::endl;
		double total_forward_solve_time = 0, total_remeshing_time = 0, total_global_relaxation_time = 0;
		const auto save_stats = [&](const int t, const double forward, const double remeshing, const double global_relaxation) {
			total_forward_solve_time += forward;
			total_remeshing_time += remeshing;
			total_global_relaxation_time += global_relaxation;

			logger().debug(
				"Forward (cur, avg, total): {} s, {} s, {} s",
				forward, total_forward_solve_time / t, total_forward_solve_time);
			logger().debug(
				"Remeshing (cur, avg, total): {} s, {} s, {} s",
				remeshing, total_remeshing_time / t, total_remeshing_time);
			logger().debug(
				"Global relaxation (cur, avg, total): {} s, {} s, {} s",
				global_relaxation, total_global_relaxation_time / t, total_global_relaxation_time);

			const double peak_mem = getPeakRSS() / double(1 << 30);
			logger().debug("Peak mem: {} GiB", peak_mem);

			stats_file << fmt::format(
				"{},{},{},{},{},{},{},{}\n",
				t, t0 + dt * t, forward, remeshing, global_relaxation, peak_mem,
				n_bases, mesh->n_elements());
			stats_file.flush();
		};

		for (int t = 1; t <= time_steps; ++t)
		{
			double cur_forward_solve_time = 0, cur_remeshing_time = 0, cur_global_relaxation_time = 0;

			igl::Timer timer;
			timer.start();
			solve_tensor_nonlinear(sol, t);
			timer.stop();
			cur_forward_solve_time = timer.getElapsedTimeInSec();

			save_energy(save_i);
			save_timestep(t0 + dt * t, save_i++, t0, save_dt, sol, Eigen::MatrixXd()); // no pressure

#ifdef POLYFEM_WITH_REMESHING
			if (remesh_enabled)
			{
				timer.start();
				const bool remesh_success = mesh::remesh(*this, sol, t0 + dt * t, dt);
				timer.stop();
				cur_remeshing_time = timer.getElapsedTimeInSec();

				// Save the solution after remeshing
				save_energy(save_i);
				save_timestep(t0 + dt * t, save_i++, t0, save_dt, sol, Eigen::MatrixXd()); // no pressure

				// Only do global relaxation if remeshing was successful
				if (remesh_success)
				{
					timer.start();
					solve_tensor_nonlinear(sol, t, false); // solve the scene again after remeshing
					timer.stop();
					cur_global_relaxation_time = timer.getElapsedTimeInSec();
				}
				// Always save the solution for consistency
				save_energy(save_i);
				save_timestep(t0 + dt * t, save_i++, t0, save_dt, sol, Eigen::MatrixXd()); // no pressure
			}
#endif

			{
				POLYFEM_SCOPED_TIMER("Update quantities");

				solve_data.time_integrator->update_quantities(sol);

				solve_data.nl_problem->update_quantities(t0 + (t + 1) * dt, sol);

				solve_data.update_dt();
				solve_data.update_barrier_stiffness(sol);
			}

			logger().info("{}/{}  t={}", t, time_steps, t0 + dt * t);

			const std::string rest_mesh_path = args["output"]["data"]["rest_mesh"].get<std::string>();
			if (remesh_enabled && !rest_mesh_path.empty())
			{
				Eigen::MatrixXd V;
				Eigen::MatrixXi F;
				build_mesh_matrices(V, F);
				io::MshWriter::write(
					resolve_output_path(fmt::format(args["output"]["data"]["rest_mesh"], t)),
					V, F, mesh->get_body_ids(), mesh->is_volume(), /*binary=*/true);
			}

			solve_data.time_integrator->save_raw(
				resolve_output_path(fmt::format(args["output"]["data"]["u_path"], t)),
				resolve_output_path(fmt::format(args["output"]["data"]["v_path"], t)),
				resolve_output_path(fmt::format(args["output"]["data"]["a_path"], t)));

			// save restart file
			save_restart_json(t0, dt, t);
			save_stats(t, cur_forward_solve_time, cur_remeshing_time, cur_global_relaxation_time);
		}
	}

	void State::init_nonlinear_tensor_solve(Eigen::MatrixXd &sol, const double t, const bool init_time_integrator)
	{
		assert(!assembler->is_linear() || is_contact_enabled()); // non-linear
		assert(!problem->is_scalar());                           // tensor
		assert(mixed_assembler == nullptr);

		// --------------------------------------------------------------------
		// Check for initial intersections
		if (is_contact_enabled())
		{
			POLYFEM_SCOPED_TIMER("Check for initial intersections");

			const Eigen::MatrixXd displaced = collision_mesh.displace_vertices(
				utils::unflatten(sol, mesh->dimension()));

			if (ipc::has_intersections(collision_mesh, displaced))
			{
				OBJWriter::write(
					resolve_output_path("intersection.obj"), displaced,
					collision_mesh.edges(), collision_mesh.faces());
				log_and_throw_error("Unable to solve, initial solution has intersections!");
			}
		}

		// --------------------------------------------------------------------
		// Initialize time integrator
		if (problem->is_time_dependent())
		{
			if (init_time_integrator)
			{
				POLYFEM_SCOPED_TIMER("Initialize time integrator");
				solve_data.time_integrator = ImplicitTimeIntegrator::construct_time_integrator(args["time"]["integrator"]);

				Eigen::MatrixXd velocity, acceleration;
				initial_velocity(velocity);
				assert(velocity.size() == sol.size());
				initial_acceleration(acceleration);
				assert(acceleration.size() == sol.size());

				const double dt = args["time"]["dt"];
				solve_data.time_integrator->init(sol, velocity, acceleration, dt);
			}
			assert(solve_data.time_integrator != nullptr);
		}
		else
		{
			solve_data.time_integrator = nullptr;
		}

		// --------------------------------------------------------------------
		// Initialize forms

		std::shared_ptr<assembler::ViscousDamping> damping_assembler = std::make_shared<assembler::ViscousDamping>();
		set_materials(*damping_assembler);

		const std::vector<std::shared_ptr<Form>> forms = solve_data.init_forms(
			// General
			mesh->dimension(), t,
			// Elastic form
			n_bases, bases, geom_bases(), *assembler, ass_vals_cache, mass_ass_vals_cache,
			// Body form
			n_pressure_bases, boundary_nodes, local_boundary, local_neumann_boundary,
			n_boundary_samples(), rhs, sol, mass_matrix_assembler->density(),
			// Inertia form
			args["time"]["quasistatic"], mass, damping_assembler->is_valid() ? damping_assembler : nullptr,
			// Lagged regularization form
			args["solver"]["advanced"]["lagged_regularization_weight"],
			args["solver"]["advanced"]["lagged_regularization_iterations"],
			// Augmented lagrangian form
			obstacle.ndof(),
			// Contact form
			args["contact"]["enabled"], collision_mesh, args["contact"]["dhat"],
			avg_mass, args["contact"]["use_convergent_formulation"],
			args["solver"]["contact"]["barrier_stiffness"],
			args["solver"]["contact"]["CCD"]["broad_phase"],
			args["solver"]["contact"]["CCD"]["tolerance"],
			args["solver"]["contact"]["CCD"]["max_iterations"],
			// Friction form
			args["contact"]["friction_coefficient"],
			args["contact"]["epsv"],
			args["solver"]["contact"]["friction_iterations"],
			// Rayleigh damping form
			args["solver"]["rayleigh_damping"]);

		for (const auto &form : forms)
			form->set_output_dir(output_dir);

		if (solve_data.contact_form != nullptr)
			solve_data.contact_form->save_ccd_debug_meshes = args["output"]["advanced"]["save_ccd_debug_meshes"];

		// --------------------------------------------------------------------
		// Initialize nonlinear problems

		const int ndof = n_bases * mesh->dimension();
		solve_data.nl_problem = std::make_shared<NLProblem>(
			ndof, boundary_nodes, local_boundary, n_boundary_samples(),
			*solve_data.rhs_assembler, t, forms);

		// --------------------------------------------------------------------

		stats.solver_info = json::array();
	}

	void State::solve_tensor_nonlinear(Eigen::MatrixXd &sol, const int t, const bool init_lagging)
	{
		assert(solve_data.nl_problem != nullptr);
		NLProblem &nl_problem = *(solve_data.nl_problem);

		assert(sol.size() == rhs.size());

		if (nl_problem.uses_lagging())
		{
			if (init_lagging)
			{
				POLYFEM_SCOPED_TIMER("Initializing lagging");
				nl_problem.init_lagging(sol); // TODO: this should be u_prev projected
			}
			logger().info("Lagging iteration 1:");
		}

		// ---------------------------------------------------------------------

		// Save the subsolve sequence for debugging
		int subsolve_count = 0;
		save_subsolve(subsolve_count, t, sol, Eigen::MatrixXd()); // no pressure

		// ---------------------------------------------------------------------

		std::shared_ptr<cppoptlib::NonlinearSolver<NLProblem>> nl_solver = make_nl_solver<NLProblem>();

		ALSolver al_solver(
			nl_solver, solve_data.al_lagr_form, solve_data.al_pen_form,
			args["solver"]["augmented_lagrangian"]["initial_weight"],
			args["solver"]["augmented_lagrangian"]["scaling"],
			args["solver"]["augmented_lagrangian"]["max_weight"],
			args["solver"]["augmented_lagrangian"]["eta"],
			args["solver"]["augmented_lagrangian"]["max_solver_iters"],
			[&](const Eigen::VectorXd &x) {
				this->solve_data.update_barrier_stiffness(sol);
			});

		al_solver.post_subsolve = [&](const double al_weight) {
			stats.solver_info.push_back(
				{{"type", al_weight > 0 ? "al" : "rc"},
				 {"t", t}, // TODO: null if static?
				 {"info", nl_solver->get_info()}});
			if (al_weight > 0)
				stats.solver_info.back()["weight"] = al_weight;
			save_subsolve(++subsolve_count, t, sol, Eigen::MatrixXd()); // no pressure
		};

		Eigen::MatrixXd prev_sol = sol;
		al_solver.solve(nl_problem, sol, args["solver"]["augmented_lagrangian"]["force"]);

		// ---------------------------------------------------------------------

		// TODO: Make this more general
		const double lagging_tol = args["solver"]["contact"].value("friction_convergence_tol", 1e-2);

		// Lagging loop (start at 1 because we already did an iteration above)
		bool lagging_converged = !nl_problem.uses_lagging();
		for (int lag_i = 1; !lagging_converged; lag_i++)
		{
			Eigen::VectorXd tmp_sol = nl_problem.full_to_reduced(sol);

			// Update the lagging before checking for convergence
			nl_problem.update_lagging(tmp_sol, lag_i);

			// Check if lagging converged
			Eigen::VectorXd grad;
			nl_problem.gradient(tmp_sol, grad);
			const double delta_x_norm = (prev_sol - sol).lpNorm<Eigen::Infinity>();
			logger().debug("Lagging convergence grad_norm={:g} tol={:g} (||Δx||={:g})", grad.norm(), lagging_tol, delta_x_norm);
			if (grad.norm() <= lagging_tol)
			{
				logger().info(
					"Lagging converged in {:d} iteration(s) (grad_norm={:g} tol={:g})",
					lag_i, grad.norm(), lagging_tol);
				lagging_converged = true;
				break;
			}

			if (delta_x_norm <= 1e-12)
			{
				logger().warn(
					"Lagging produced tiny update between iterations {:d} and {:d} (grad_norm={:g} grad_tol={:g} ||Δx||={:g} Δx_tol={:g}); stopping early",
					lag_i - 1, lag_i, grad.norm(), lagging_tol, delta_x_norm, 1e-6);
				lagging_converged = false;
				break;
			}

			// Check for convergence first before checking if we can continue
			if (lag_i >= nl_problem.max_lagging_iterations())
			{
				logger().warn(
					"Lagging failed to converge with {:d} iteration(s) (grad_norm={:g} tol={:g})",
					lag_i, grad.norm(), lagging_tol);
				lagging_converged = false;
				break;
			}

			// Solve the problem with the updated lagging
			logger().info("Lagging iteration {:d}:", lag_i + 1);
			nl_problem.init(sol);
			solve_data.update_barrier_stiffness(sol);
			nl_solver->minimize(nl_problem, tmp_sol);
			prev_sol = sol;
			sol = nl_problem.reduced_to_full(tmp_sol);

			// Save the subsolve sequence for debugging and info
			stats.solver_info.push_back(
				{{"type", "rc"},
				 {"t", t}, // TODO: null if static?
				 {"lag_i", lag_i},
				 {"info", nl_solver->get_info()}});
			save_subsolve(++subsolve_count, t, sol, Eigen::MatrixXd()); // no pressure
		}
	}

	////////////////////////////////////////////////////////////////////////
	// Template instantiations
	template std::shared_ptr<cppoptlib::NonlinearSolver<NLProblem>> State::make_nl_solver(const std::string &) const;
} // namespace polyfem
