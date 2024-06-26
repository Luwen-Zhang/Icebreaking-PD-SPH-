/**
 * @file passive_cantilever.cpp
 * @brief This is the first example of cantilever 
 * @author Chi Zhang and Xiangyu Hu
 * @ref 	doi.org/10.1016/j.jcp.2013.12.012
 */
#include "sphinxsys.h"
/** Name space. */
using namespace SPH;
/** Geometry parameters. */
Real PL = 6.0;
Real PH = 1.0;
Real PW = 1.0;
Real SL = 0.5;
Real resolution_ref = PH / 12.0; /**< Initial particle spacing. */
Real BW = resolution_ref * 4;	 /**< Boundary width. */
Vecd halfsize_cantilever(0.5 * (PL + SL), 0.5 * PH, 0.5 * PW);
Vecd translation_cantilever(0.5 * (PL - SL), 0.5 * PH, 0.5 * PW);
Vecd halfsize_holder(0.5 * SL, 0.5 * PH, 0.5 * PW);
Vecd translation_holder(-0.5 * SL, 0.5 * PH, 0.5 * PW);
/** Domain bounds of the system. */
BoundingBox system_domain_bounds(Vecd(-SL - BW, -BW, -BW),
								 Vecd(PL + BW, PH + BW, PH + BW));
// Observer location
StdVec<Vecd> observation_location = {Vecd(PL, PH, PW)};
/** For material properties of the solid. */
Real rho0_s = 1100.0;
Real poisson = 0.45;
Real Youngs_modulus = 1.7e7;
Real a = Youngs_modulus / (2.0 * (1.0 + poisson));
Real a_f = 0.0 * a;
Real a0[4] = {a, a_f, 0.0, 0.0};
Real b0[4] = {1.0, 0.0, 0.0, 0.0};
Vec3d fiber_direction(1.0, 0.0, 0.0);
Vec3d sheet_direction(0.0, 1.0, 0.0);
Real bulk_modulus = Youngs_modulus / 3.0 / (1.0 - 2.0 * poisson);

Real gravity_g = 0.0;

/** Define the cantilever shape. */
class Cantilever : public ComplexShape
{
public:
	explicit Cantilever(const std::string &shape_name) : ComplexShape(shape_name)
	{
		add<TransformShape<GeometricShapeBox>>(Transformd(translation_cantilever), halfsize_cantilever);
		add<TransformShape<GeometricShapeBox>>(Transformd(translation_holder), halfsize_holder);
	}
};
/**
 * application dependent initial condition 
 */
class CantileverInitialCondition
	: public solid_dynamics::ElasticDynamicsInitialCondition
{
public:
	explicit CantileverInitialCondition(SPHBody &sph_body)
		: solid_dynamics::ElasticDynamicsInitialCondition(sph_body){};

	void update(size_t index_i, Real dt)
	{
		Real coff = 1.0;
		if (pos_[index_i][0] > 0.0)
		{
			vel_[index_i][1] = coff * 5.0 * sqrt(3.0);
			vel_[index_i][2] = coff * 5.0;
		}
	};
};
/**
 *  The main program
 */
int main()
{
	/** Setup the system. Please the make sure the global domain bounds are correctly defined. */
	SPHSystem system(system_domain_bounds, resolution_ref);
	/** create a Cantilever body, corresponding material, particles and reaction model. */
	PDBody cantilever_body(system, makeShared<Cantilever>("PDBody"));
	cantilever_body.defineParticlesAndMaterial<NosbPDParticles, HughesWingetSolid>(rho0_s, Youngs_modulus, poisson);
	cantilever_body.generateParticles<ParticleGeneratorLattice>();

	size_t particle_num_s = cantilever_body.getBaseParticles().total_real_particles_;

	/** Define Observer. */
	ObserverBody cantilever_observer(system, "CantileverObserver");
	cantilever_observer.generateParticles<ObserverParticleGenerator>(observation_location);

	/** topology */
	InnerRelation cantilever_body_inner(cantilever_body);
	ContactRelation cantilever_observer_contact(cantilever_observer, {&cantilever_body});

	/** 
	 * This section define all numerical methods will be used in this case.
	 */
	SimpleDynamics<CantileverInitialCondition> initialization(cantilever_body);
	/** calculate shape Matrix */
	InteractionWithUpdate<solid_dynamics::NosbPDShapeMatrix>
		cantilever_shapeMatrix(cantilever_body_inner);
	/** Time step size calculation. */
	ReduceDynamics<solid_dynamics::AcousticTimeStepSize>
		computing_time_step_size(cantilever_body);
	SimpleDynamics<TimeStepInitialization> initialize_a_solid_step(cantilever_body, makeShared<Gravity>(Vecd(0.0, 0.0, -gravity_g)));
	//stress relaxation for the beam by Hughes-Winget algorithm
	SimpleDynamics<solid_dynamics::NosbPDFirstStep> NosbPD_firstStep(cantilever_body);
	InteractionWithUpdate<solid_dynamics::NosbPDSecondStep> NosbPD_secondStep(cantilever_body_inner);
	InteractionDynamics<solid_dynamics::NosbPDThirdStep> NosbPD_thirdStep(cantilever_body_inner);
	//SimpleDynamics<solid_dynamics::NosbPDFourthStep> NosbPD_fourthStep(cantilever_body);
	SimpleDynamics<solid_dynamics::NosbPDFourthStepWithADR> NosbPD_fourthStepADR(cantilever_body);
	//hourglass displacement mode control by LittleWood method
	InteractionDynamics<solid_dynamics::LittleWoodHourGlassControl> hourglass_control(cantilever_body_inner, cantilever_body.sph_adaptation_->getKernel());
	//Numerical Damping
	InteractionDynamics<solid_dynamics::PairNumericalDampingforPD> numerical_damping(cantilever_body_inner, cantilever_body.sph_adaptation_->getKernel());
	// ADR_cn calculation
	ReduceDynamics<solid_dynamics::ADRFirstStep> computing_cn1(cantilever_body);
	ReduceDynamics<solid_dynamics::ADRSecondStep> computing_cn2(cantilever_body);

	/** Constrain the holder. */
	BodyRegionByParticle holder(cantilever_body, 
		makeShared<TransformShape<GeometricShapeBox>>(Transformd(translation_holder), halfsize_holder, "Holder"));
	SimpleDynamics<solid_dynamics::FixBodyPartConstraint> constraint_holder(holder);
	/** Output */
	IOEnvironment io_environment(system);
	BodyStatesRecordingToVtp write_states(io_environment, system.real_bodies_);
	RegressionTestDynamicTimeWarping<ObservedQuantityRecording<Vecd>>
		write_displacement("Position", io_environment, cantilever_observer_contact);
	//Log file
	std::string Logpath = io_environment.output_folder_ + "/SimLog.txt";
	if (fs::exists(Logpath))
	{
		fs::remove(Logpath);
	}
	std::ofstream log_file(Logpath.c_str(), ios::trunc);
	std::cout << "# PARAM SETING #" << "\n" << "\n"
		<< "	particle_spacing_ref = " << resolution_ref << "\n"
		<< "	particle_num_s = " << particle_num_s << "\n" << "\n"
		<< "	rho0_s = " << rho0_s << "\n"
		<< "	Youngs_modulus = " << Youngs_modulus << "\n"
		<< "	poisson = " << poisson << "\n" << "\n"
		<< "	gravity_g = " << gravity_g << "\n" << "\n"
		<< "# COMPUTATION START # " << "\n" << "\n";
	log_file << "#PARAM SETING#" << "\n" << "\n"
		<< "	particle_spacing_ref = " << resolution_ref << "\n"
		<< "	particle_num_s = " << particle_num_s << "\n" << "\n"
		<< "	rho0_s = " << rho0_s << "\n"
		<< "	Youngs_modulus = " << Youngs_modulus << "\n"
		<< "	poisson = " << poisson << "\n" << "\n"
		<< "	gravity_g = " << gravity_g << "\n" << "\n"
		<< "#COMPUTATION START HERE# " << "\n" << "\n";
	/**
	 * From here the time stepping begins.
	 * Set the starting time.
	 */
	GlobalStaticVariables::physical_time_ = 0.0;
	system.initializeSystemCellLinkedLists();
	system.initializeSystemConfigurations();
	/** apply initial condition */
	initialization.parallel_exec();
	cantilever_shapeMatrix.parallel_exec();
	write_states.writeToFile(0);
	write_displacement.writeToFile(0);
	/** Setup physical parameters. */
	int ite = 0;
	Real end_time = 2.0;
	Real output_period = end_time / 200.0;
	Real dt = 0.0;

	Real cn1 = 0.0;
	Real cn2 = 0.0;
	Real ADR_cn = 0.0;

	/** Statistics for computing time. */
	tick_count t1 = tick_count::now();
	tick_count::interval_t interval;
	/**
	 * Main loop
	 */
	while (GlobalStaticVariables::physical_time_ < end_time)
	{
		Real integration_time = 0.0;
		while (integration_time < output_period)
		{
			ite++;
			dt = 0.1 * computing_time_step_size.parallel_exec();
			integration_time += dt;
			GlobalStaticVariables::physical_time_ += dt;
			if (ite % 100 == 0) {
				std::cout << "	N=" << ite << " Time: "
					<< GlobalStaticVariables::physical_time_ << "	dt: "
					<< dt << "\n";
				log_file << "	N=" << ite << " Time: "
					<< GlobalStaticVariables::physical_time_ << "	dt: "
					<< dt << "\n";
			}
			initialize_a_solid_step.parallel_exec(dt);

			NosbPD_firstStep.parallel_exec(dt);
			NosbPD_secondStep.parallel_exec(dt);
			hourglass_control.parallel_exec(dt);
			numerical_damping.parallel_exec(dt);
			NosbPD_thirdStep.parallel_exec(dt);
			/*cn1 = SMAX(TinyReal, computing_cn1.parallel_exec(dt));
			cn2 = computing_cn2.parallel_exec(dt);
			if (cn2 > TinyReal) {
				ADR_cn = 2.0 * sqrt(cn1 / cn2);
			}
			else {
				ADR_cn = 0.0;
			}
			ADR_cn = 0.01 * ADR_cn;
			NosbPD_fourthStepADR.getADRcn(ADR_cn);*/
			NosbPD_fourthStepADR.parallel_exec(dt);

			constraint_holder.parallel_exec(dt);
			
			
		}
		write_displacement.writeToFile(ite);
		tick_count t2 = tick_count::now();
		write_states.writeToFile();
		tick_count t3 = tick_count::now();
		interval += t3 - t2;
	}
	tick_count t4 = tick_count::now();

	tick_count::interval_t tt;
	tick_count::interval_t tt2;
	tt = t4 - t1 - interval;
	tt2 = t4 - t1;
	std::cout << "Total wall time for computation: " << tt.seconds() << " seconds." << std::endl;
	log_file << "\n" << "Total wall time for computation: " << tt.seconds() << " seconds." << endl;
	cout << "\n" << "Total wall time for computation & output: " << tt2.seconds() << " seconds." << endl;
	log_file << "\n" << "Total wall time for computation & output: " << tt2.seconds() << " seconds." << endl;

	write_displacement.newResultTest();

	return 0;
}
