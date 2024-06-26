/* -------------------------------------------------------------------------*
 *								SPHinXsys									*
 * --------------------------------------------------------------------------*
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle	*
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
 * physical accurate simulation and aims to model coupled industrial dynamic *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH	*
 * (smoothed particle hydrodynamics), a meshless computational method using	*
 * particle discretization.													*
 *																			*
 * SPHinXsys is partially funded by German Research Foundation				*
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1				*
 * and HU1527/12-1.															*
 *                                                                           *
 * Portions copyright (c) 2017-2020 Technical University of Munich and		*
 * the authors' affiliations.												*
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may   *
 * not use this file except in compliance with the License. You may obtain a *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
 *                                                                           *
 * --------------------------------------------------------------------------*/
/**
 * @file 	general_interpolation.h
 * @brief 	There are the classes for interpolation algorithm.
 * @author	Xiangyu Hu and Chi Zhang
 */

#ifndef GENERAL_INTERPOLATION_H
#define GENERAL_INTERPOLATION_H

#include "general_dynamics.h"

namespace SPH
{
	typedef DataDelegateContact<BaseParticles, BaseParticles> InterpolationContactData;

	/**
	 * @class BaseInterpolation
	 * @brief Base class for interpolation.
	 */
	template <typename VariableType>
	class BaseInterpolation : public LocalDynamics, public InterpolationContactData
	{
	public:
		StdLargeVec<VariableType> *interpolated_quantities_;

		explicit BaseInterpolation(BaseContactRelation &contact_relation, const std::string &variable_name)
			: LocalDynamics(contact_relation.getSPHBody()), InterpolationContactData(contact_relation),
			  interpolated_quantities_(nullptr)
		{
			for (size_t k = 0; k != this->contact_particles_.size(); ++k)
			{
				contact_Vol_.push_back(&(this->contact_particles_[k]->Vol_));
				StdLargeVec<VariableType> *contact_data =
					this->contact_particles_[k]->template getVariableByName<VariableType>(variable_name);
				contact_data_.push_back(contact_data);
			}
		};
		virtual ~BaseInterpolation(){};

		void interaction(size_t index_i, Real dt = 0.0)
		{
			VariableType observed_quantity = ZeroData<VariableType>::value;
			Real ttl_weight(0);

			for (size_t k = 0; k < this->contact_configuration_.size(); ++k)
			{
				StdLargeVec<Real> &Vol_k = *(contact_Vol_[k]);
				StdLargeVec<VariableType> &data_k = *(contact_data_[k]);
				Neighborhood &contact_neighborhood = (*this->contact_configuration_[k])[index_i];
				for (size_t n = 0; n != contact_neighborhood.current_size_; ++n)
				{
					size_t index_j = contact_neighborhood.j_[n];
					Real weight_j = contact_neighborhood.W_ij_[n] * Vol_k[index_j];

					observed_quantity += weight_j * data_k[index_j];
					ttl_weight += weight_j;
				}
			}
			(*interpolated_quantities_)[index_i] = observed_quantity / (ttl_weight + TinyReal);
		};

	protected:
		StdVec<StdLargeVec<Real> *> contact_Vol_;
		StdVec<StdLargeVec<VariableType> *> contact_data_;
	};

	/**
	 * @class InterpolatingAQuantity
	 * @brief Interpolate a given member data in the particles of a general body
	 */
	template <typename VariableType>
	class InterpolatingAQuantity : public BaseInterpolation<VariableType>
	{
	public:
		explicit InterpolatingAQuantity(BaseContactRelation &contact_relation,
										const std::string &interpolated_variable, const std::string &target_variable)
			: BaseInterpolation<VariableType>(contact_relation, target_variable)
		{
			this->interpolated_quantities_ =
				this->particles_->template getVariableByName<VariableType>(interpolated_variable);
		};
		virtual ~InterpolatingAQuantity(){};
	};

	/**
	 * @class ObservingAQuantity
	 * @brief Observing a variable from contact bodies.
	 */
	template <typename VariableType>
	class ObservingAQuantity : public InteractionDynamics<BaseInterpolation<VariableType>>
	{
	public:
		explicit ObservingAQuantity(BaseContactRelation &contact_relation, const std::string &variable_name)
			: InteractionDynamics<BaseInterpolation<VariableType>>(contact_relation, variable_name)
		{
			this->interpolated_quantities_ = registerObservedQuantity(variable_name);
		};
		virtual ~ObservingAQuantity(){};

	protected:
		StdLargeVec<VariableType> observed_quantities_;

		/** Register the  observed variable if the variable name is new.
		 * If the variable is registered already, the registered variable will be returned. */
		StdLargeVec<VariableType> *registerObservedQuantity(const std::string &variable_name)
		{
			BaseParticles *particles = this->particles_;
			constexpr int type_index = DataTypeIndex<VariableType>::value;
			if (particles->all_variable_maps_[type_index].find(variable_name) == particles->all_variable_maps_[type_index].end())
			{
				particles->registerVariable(observed_quantities_, variable_name, ZeroData<VariableType>::value);
				return &observed_quantities_;
			}
			return particles->getVariableByName<VariableType>(variable_name);
		};
	};

	/**
	 * @class CorrectInterpolationKernelWeights
	 * @brief  correct kernel weights for interpolation between general bodies
	 */
	class CorrectInterpolationKernelWeights : public LocalDynamics,
											  public InterpolationContactData
	{
	public:
		explicit CorrectInterpolationKernelWeights(BaseContactRelation &contact_relation);
		virtual ~CorrectInterpolationKernelWeights(){};
		void interaction(size_t index_i, Real dt = 0.0);

	protected:
		StdVec<StdLargeVec<Real> *> contact_Vol_;
	};
}
#endif // GENERAL_INTERPOLATION_H
