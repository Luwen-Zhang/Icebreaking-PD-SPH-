/* -------------------------------------------------------------------------*
 *								SPHinXsys									*
 * -------------------------------------------------------------------------*
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle*
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
 * physical accurate simulation and aims to model coupled industrial dynamic*
 * systems including fluid, solid, multi-body dynamics and beyond with SPH	*
 * (smoothed particle hydrodynamics), a meshless computational method using	*
 * particle discretization.													*
 *																			*
 * SPHinXsys is partially funded by German Research Foundation				*
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,			*
 *  HU1527/12-1 and HU1527/12-4												*
 *                                                                          *
 * Portions copyright (c) 2017-2022 Technical University of Munich and		*
 * the authors' affiliations.												*
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may  *
 * not use this file except in compliance with the License. You may obtain a*
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.       *
 *                                                                          *
 * ------------------------------------------------------------------------*/
/**
 * @file 	solid_particles.h
 * @brief 	This is the derived class of base particles.
 * @author	Chi ZHang, Dong Wu and Xiangyu Hu
 */

#ifndef SOLID_PARTICLES_H
#define SOLID_PARTICLES_H

#include "elastic_solid.h"
#include <inelastic_solid.h>
#include "base_particles.h"
#include "base_particles.hpp"

#include "particle_generator_lattice.h"
namespace SPH
{
	/**
	 *	pre-claimed classes
	 */
	class Solid;
	class ElasticSolid;

	/**
	 * @class SolidParticles
	 * @brief A group of particles with solid body particle data.
	 */
	class SolidParticles : public BaseParticles
	{
	public:
		SolidParticles(SPHBody &sph_body, Solid *solid);
		virtual ~SolidParticles(){};

		StdLargeVec<Vecd> pos0_; /**< initial position */
		StdLargeVec<Vecd> n_;	 /**< normal direction */
		StdLargeVec<Vecd> n0_;	 /**< initial normal direction */
		StdLargeVec<Matd> B_;	 /**< configuration correction for linear reproducing */
		Solid &solid_;

		StdLargeVec<Real> contact_density_; /**< contact density Added by Haotian Shi from SJTU */

		/** Get the kernel gradient in weak form. */
		virtual Vecd getKernelGradient(size_t index_i, size_t index_j, Real dW_ijV_j, Vecd &e_ij) override;
		/** Get wall average velocity when interacting with fluid. */
		virtual StdLargeVec<Vecd> *AverageVelocity() { return &vel_; };
		/** Get wall average acceleration when interacting with fluid. */
		virtual StdLargeVec<Vecd> *AverageAcceleration() { return &acc_; };
		/** Initialized variables for solid particles. */
		virtual void initializeOtherVariables() override;
		/** Return this pointer. */
		virtual SolidParticles *ThisObjectPtr() override { return this; };
	};

	/**
	 * @class ElasticSolidParticles
	 * @brief A group of particles with elastic body particle data.
	 */
	class ElasticSolidParticles : public SolidParticles
	{
	public:
		ElasticSolidParticles(SPHBody &sph_body, ElasticSolid *elastic_solid);
		virtual ~ElasticSolidParticles(){};

		StdLargeVec<Matd> F_;	  /**<  deformation tensor */
		StdLargeVec<Matd> dF_dt_; /**<  deformation tensor change rate */
		ElasticSolid &elastic_solid_;
		//----------------------------------------------------------------------
		//		for fluid-structure interaction (FSI)
		//----------------------------------------------------------------------
		StdLargeVec<Vecd> vel_ave_; /**<  fluid time-step averaged particle velocity */
		StdLargeVec<Vecd> acc_ave_; /**<  fluid time-step averaged particle acceleration */
		//----------------------------------------------------------------------
		//		for adaptive dynamic relaxtion (ADR)
		//----------------------------------------------------------------------
		StdLargeVec<Vecd> acc_old_; /**<  acceleration in last step */

		/** Return the Lagrange strain. */
		Matd getGreenLagrangeStrain(size_t particle_i);
		/** Computing principal strain - returns the principal strains in descending order (starting from the largest) */
		Vecd getPrincipalStrains(size_t particle_i);
		/** Computing von Mises equivalent strain from a static (constant) formulation. */
		Real getVonMisesStrain(size_t particle_i);
		/** Computing von Mises equivalent strain from a "dynamic" formulation. This depends on the Poisson's ratio (from commercial FEM software Help). */
		Real getVonMisesStrainDynamic(size_t particle_i, Real poisson);
		/** Computing von Mises strain for all particles. - "static" or "dynamic"*/
		StdLargeVec<Real> getVonMisesStrainVector(std::string strain_measure = "static");
		/** Computing maximum von Mises strain from all particles. - "static" or "dynamic" */
		Real getVonMisesStrainMax(std::string strain_measure = "static");
		/** Return the max principal strain. */
		Real getPrincipalStrainMax();
		/** get the Cauchy stress. */
		Matd getStressCauchy(size_t particle_i);
		/** get the PK2 stress. */
		Matd getStressPK2(size_t particle_i);
		/** Computing principal_stresses - returns the principal stresses in descending order (starting from the largest) */
		Vecd getPrincipalStresses(size_t particle_i);
		/** Computing von_Mises_stress - "Cauchy" or "PK2" decided based on the stress_measure_ */
		Real getVonMisesStress(size_t particle_i);
		/** Computing von Mises stress for all particles. - "Cauchy" or "PK2" decided based on the stress_measure_ */
		StdLargeVec<Real> getVonMisesStressVector();
		/** Computing maximum von Mises stress from all particles. - "Cauchy" or "PK2" decided based on the stress_measure_ */
		Real getVonMisesStressMax();
		Real getPrincipalStressMax();

		/** Computing displacement. */
		Vecd displacement(size_t particle_i);
		/** Return the displacement. */
		StdLargeVec<Vecd> getDisplacement();
		/** get the max displacement. */
		Real getMaxDisplacement();

		/**< Computing normal vector. */
		Vecd normal(size_t particle_i);
		/** get the normal vector. */
		StdLargeVec<Vecd> getNormal();

		/** relevant stress measure */
		std::string stress_measure_;

		/** Get wall average velocity when interacting with fluid. */
		virtual StdLargeVec<Vecd> *AverageVelocity() override { return &vel_ave_; };
		/** Get wall average acceleration when interacting with fluid. */
		virtual StdLargeVec<Vecd> *AverageAcceleration() override { return &acc_ave_; };

		/** Initialize the variables for elastic particle. */
		virtual void initializeOtherVariables() override;
		/** Return this pointer. */
		virtual ElasticSolidParticles *ThisObjectPtr() override { return this; };
	};

	/**
	 * @class ShellParticles
	 * @brief A group of particles with shell particle data.
	 */
	class ShellParticles : public ElasticSolidParticles
	{
	public:
		ShellParticles(SPHBody &sph_body, ElasticSolid *elastic_solid);
		virtual ~ShellParticles(){};

		Real thickness_ref_;						/**< Shell thickness. */
		StdLargeVec<Matd> transformation_matrix_; 	/**< initial transformation matrix from global to local coordinates */
		StdLargeVec<Real> thickness_;			  	/**< shell thickness */
		/**
		 *	extra generalized coordinates in global coordinate
		 */
		StdLargeVec<Vecd> pseudo_n_;	  /**< current pseudo-normal vector */
		StdLargeVec<Vecd> dpseudo_n_dt_;  /**< pseudo-normal vector change rate */
		StdLargeVec<Vecd> dpseudo_n_d2t_; /**< pseudo-normal vector second order time derivation */
		/**
		 *	extra generalized coordinate and velocity in local coordinate
		 */
		StdLargeVec<Vecd> rotation_;		/**< rotation angle of the initial normal respective to each axis */
		StdLargeVec<Vecd> angular_vel_;		/**< angular velocity respective to each axis */
		StdLargeVec<Vecd> dangular_vel_dt_; /**< angular acceleration of respective to each axis*/
		/**
		 *	extra deformation and deformation rate in local coordinate
		 */
		StdLargeVec<Matd> F_bending_;	  /**< bending deformation gradient	*/
		StdLargeVec<Matd> dF_bending_dt_; /**< bending deformation gradient change rate	*/
		/**
		 *	extra stress for pair interaction in global coordinate
		 */
		StdLargeVec<Vecd> global_shear_stress_; /**< global shear stress */
		StdLargeVec<Matd> global_stress_;		/**<  global stress for pair interaction */
		StdLargeVec<Matd> global_moment_;		/**<  global bending moment for pair interaction */
		/** get particle volume. */
		virtual Real ParticleVolume(size_t index_i) override { return Vol_[index_i] * thickness_[index_i]; }
		/** get particle mass. */
		virtual Real ParticleMass(size_t index_i) override { return mass_[index_i] * thickness_[index_i]; }
		/** Initialize variable for shell particles. */
		virtual void initializeOtherVariables() override;
		/** Return this pointer. */
		virtual ShellParticles *ThisObjectPtr() override { return this; };
	};
	/**
	 * Created by Haotian Shi from SJTU
	 * @class NosbPDParticles
	 * @brief A group of particles with Non-Oridinary State Based Peridynamic body particle data.
	 */
	class PDParticles : public ElasticSolidParticles
	{
	public:
		PDParticles(SPHBody& sph_body, ElasticSolid* elastic_solid);
		virtual ~PDParticles() {};

		StdLargeVec<int> particleLive_;	  /**<  particleLive = false, when all the bonds are broken */
		//StdLargeVec<int> bond0_;		  /**<  number of bonds in the initial configuration */
		StdLargeVec<int> bond_;		  /**<  number of bonds in the current configuration */
		StdLargeVec<Real> damage_;		  /**<  Local damage */

		/** Initialize variable for Nosb-PD particles. */
		virtual void initializeOtherVariables() override;
		/** Return this pointer. */
		virtual PDParticles* ThisObjectPtr() override { return this; };
	};

	/**
	 * Created by Haotian Shi from SJTU
	 * @class NosbPDParticles
	 * @brief A group of particles with Non-Oridinary State Based Peridynamic body particle data.
	 */
	class NosbPDParticles : public PDParticles
	{
	public:
		NosbPDParticles(SPHBody& sph_body, ElasticSolid* elastic_solid);
		virtual ~NosbPDParticles() {};		
				
		StdLargeVec<Matd> shape_K_;			  /**<  shape tensor determined in the reference configuration */
		StdLargeVec<Matd> shape_K_1_;		  /**<  inverse of shape_K */
		//variables for Hughes-Winget algorithm
		StdLargeVec<Matd> N_;				  /**<  deformation tensor determined in the current configuration */
		StdLargeVec<Matd> N_deltaU_;		  /**<  velocity tensor determined in the current configuration */
		StdLargeVec<Matd> N_half_;			  /**<  deformation tensor determined in the middle configuration */

		StdLargeVec<Matd> F_half_;			  /**<  deformation gradient of middle configuration */
		StdLargeVec<Matd> F_delta_;			  /**<  velocity gradient determined in the current configuration */
		StdLargeVec<Matd> F_1_;				  /**<  inverse of F_ */
		StdLargeVec<Matd> F_1_half_;		  /**<  inverse of F_half_ */

		StdLargeVec<Matd> stress_;	  /**<  Cauchy Stress tensor */
		StdLargeVec<Matd> PK1_;		  /**<  the lagrange stress tensor namely the first Piola-Kirchhoff stress tensor */
		StdLargeVec<Matd> T0_;		  /**<  force state on particle i */		

		/** Initialize variable for Nosb-PD particles. */
		virtual void initializeOtherVariables() override;
		/** Return this pointer. */
		virtual NosbPDParticles* ThisObjectPtr() override { return this; };
	};
	/**
	 * Created by Haotian Shi from SJTU
	 * @class NosbPDPlasticParticles
	 * @brief A group of particles with Non-Oridinary State Based Peridynamic body particle data.
	 */
	class NosbPDPlasticParticles : public NosbPDParticles
	{
	public:
		NosbPDPlasticParticles(SPHBody& sph_body, PlasticSolidforPD* elastic_solid);
		virtual ~NosbPDPlasticParticles() {};

		PlasticSolidforPD& plastic_solid_;
		//variables for Plastic deformation
		StdLargeVec<Matd> plastic_strain_;	  /**<  Plastic Strain tensor */

		/** Initialize variable for Nosb-PD particles. */
		virtual void initializeOtherVariables() override;
		/** Return this pointer. */
		virtual NosbPDPlasticParticles* ThisObjectPtr() override { return this; };
	};

}
#endif // SOLID_PARTICLES_H
