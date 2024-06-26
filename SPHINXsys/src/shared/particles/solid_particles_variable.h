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
 *  HU1527/12-1 and HU1527/12-4													*
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
 * @file 	solid_particle_variable.h
 * @brief 	Here, we define the algorithm classes for computing derived solid dynamics variables.
 * @details 	These variable can be added into variable list for state output.
 * @author	Chi ZHang and Xiangyu Hu
 */

#ifndef SOLID_PARTICLES_VARIABLE_H
#define SOLID_PARTICLES_VARIABLE_H

#include "particle_dynamics_algorithms.h"
#include "all_body_relations.h"
#include "solid_body.h"
#include "solid_particles.h"

namespace SPH
{
	//----------------------------------------------------------------------
	//		for general solid dynamics variables
	//----------------------------------------------------------------------
	typedef DataDelegateSimple<SolidParticles> SolidDataSimple;

	/**
	 * @class Displacement
	 * @brief computing displacement from current and initial particle position
	 */
	class Displacement : public BaseDerivedVariable<Vecd>,
						 public SolidDataSimple,
						 public LocalDynamics
	{
	public:
		explicit Displacement(SPHBody &sph_body);
		virtual ~Displacement(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		StdLargeVec<Vecd> &pos_, &pos0_;
	};

	/**
	 * @class OffsetInitialPosition
	 * @brief offset initial particle position
	 */
	class OffsetInitialPosition : public SolidDataSimple, 
								  public LocalDynamics
	{
	public:
		explicit OffsetInitialPosition(SPHBody &sph_body, Vecd &offset);
		virtual ~OffsetInitialPosition(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		Vecd offset_;
		StdLargeVec<Vecd> &pos_, &pos0_;
	};

	/**
	 * @class TranslationAndRotation
	 * @brief transformation on particle position and rotation
	 */
	class TranslationAndRotation : public SolidDataSimple, 
								   public LocalDynamics
	{
	public:
		explicit TranslationAndRotation(SPHBody &sph_body, Transformd &transform);
		virtual ~TranslationAndRotation(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		Transformd &transform_;
		StdLargeVec<Vecd> &pos_, &pos0_;
	};

	/**
	 * @class NormalDirectionFromBodyShape
	 * @brief normal direction at particles
	 */
	class NormalDirectionFromBodyShape : public SolidDataSimple, 
										 public LocalDynamics
	{
	public:
		explicit NormalDirectionFromBodyShape(SPHBody &sph_body);
		virtual ~NormalDirectionFromBodyShape(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		Shape &body_shape_;
		StdLargeVec<Vecd> &pos_, &n_, &n0_;
	};

	/**
	 * @class NormalDirectionFromBodyShape
	 * @brief normal direction at particles
	 */
	class NormalDirectionFromShapeAndOp : public SolidDataSimple, 
										  public LocalDynamics
	{
	public:
		explicit NormalDirectionFromShapeAndOp(SPHBody &sph_body, const std::string &shape_name);
		virtual ~NormalDirectionFromShapeAndOp(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		ShapeAndOp *shape_and_op_;
		Shape *shape_;
		const Real switch_sign_;
		StdLargeVec<Vecd> &pos_, &n_, &n0_;
	};

	//----------------------------------------------------------------------
	//		for general elastic solid dynamics variables
	//----------------------------------------------------------------------
	typedef DataDelegateSimple<ElasticSolidParticles> ElasticSolidDataSimple;

	class GreenLagrangeStrain : public BaseDerivedVariable<Matd>,
								public ElasticSolidDataSimple,
								public LocalDynamics
	{
	public:
		explicit GreenLagrangeStrain(SPHBody &sph_body);
		virtual ~GreenLagrangeStrain(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		StdLargeVec<Matd> &F_;
	};

	/**
	 * @class VonMisesStress
	 * @brief computing von_Mises_stress
	 */
	class VonMisesStress : public BaseDerivedVariable<Real>,
						   public ElasticSolidDataSimple,
						   public LocalDynamics
	{
	public:
		explicit VonMisesStress(SPHBody &sph_body);
		virtual ~VonMisesStress(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		Real rho0_;
		StdLargeVec<Real> &rho_;
		StdLargeVec<Matd> &F_;
		ElasticSolid &elastic_solid_;
	};

	/**
	 * @class VonMisesStrain
	 * @brief computing von Mises strain
	 */
	class VonMisesStrain : public BaseDerivedVariable<Real>,
						   public ElasticSolidDataSimple,
						   public LocalDynamics
	{
	public:
		explicit VonMisesStrain(SPHBody &sph_body);
		virtual ~VonMisesStrain(){};
		void update(size_t index_i, Real dt = 0.0);
	};	
	/**
	 * @class VonMisesStrain
	 * @brief update von Mises strain
	 */
	class VonMisesStrainDynamic : public BaseDerivedVariable<Real>,
								  public ElasticSolidDataSimple,
								  public LocalDynamics
	{
	public:
		explicit VonMisesStrainDynamic(SPHBody &sph_body);
		virtual ~VonMisesStrainDynamic(){};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		Real poisson_ratio_;
	};
	//----------------------------------------------------------------------
	//		for PD plastic solid dynamics variables
	//----------------------------------------------------------------------
	typedef DataDelegateSimple<NosbPDParticles> PDSolidDataSimple;
	typedef DataDelegateSimple<NosbPDPlasticParticles> PDPlasticSolidDataSimple;
	/**
	 * @Created by Haotian Shi from SJTU
	 * @class VonMisesStressforPD
	 * @brief computing von_Mises_stress in context of NosbPD
	 */
	class VonMisesStressforPD : public BaseDerivedVariable<Real>,
		public PDSolidDataSimple,
		public LocalDynamics
	{
	public:
		explicit VonMisesStressforPD(SPHBody& sph_body);
		virtual ~VonMisesStressforPD() {};
		void update(size_t index_i, Real dt = 0.0);

	protected:		
		const Real sqrt_3_over_2_ = sqrt(3.0 / 2.0);
		StdLargeVec<Matd>& stress_;	
	};
	/**
	 * @Created by Haotian Shi from SJTU
	 * @class VonMisesPlasticStrainforPD
	 * @brief computing von_Mises_plastic_strain
	 */
	class VonMisesPlasticStrainforPD : public BaseDerivedVariable<Real>,
		public PDPlasticSolidDataSimple,
		public LocalDynamics
	{
	public:
		explicit VonMisesPlasticStrainforPD(SPHBody& sph_body);
		virtual ~VonMisesPlasticStrainforPD() {};
		void update(size_t index_i, Real dt = 0.0);

	protected:
		const Real sqrt_3_over_2_ = sqrt(3.0 / 2.0);
		StdLargeVec<Matd>& plastic_strain_;
	};
}
#endif // SOLID_PARTICLES_VARIABLE_H