#include "adaptation.h"

#include "sph_system.h"
#include "all_kernels.h"
#include "base_body.h"
#include "base_particles.hpp"
#include "base_particle_dynamics.h"
#include "mesh_with_data_packages.hpp"
#include "vector_functions.h"

namespace SPH
{
	//=================================================================================================//
	SPHAdaptation::SPHAdaptation(Real resolution_ref, Real h_spacing_ratio, Real system_refinement_ratio)
		: h_spacing_ratio_(h_spacing_ratio), system_refinement_ratio_(system_refinement_ratio),
		  local_refinement_level_(0), spacing_ref_(resolution_ref / system_refinement_ratio_),
		  h_ref_(h_spacing_ratio_ * spacing_ref_), kernel_ptr_(makeUnique<KernelWendlandC2>(h_ref_)),
		  sigma0_ref_(computeReferenceNumberDensity(Vecd())),
		  spacing_min_(this->MostRefinedSpacing(spacing_ref_, local_refinement_level_)),
		  h_ratio_max_(powerN(2.0, local_refinement_level_)){};
	//=================================================================================================//
	SPHAdaptation::SPHAdaptation(SPHBody &sph_body, Real h_spacing_ratio, Real system_refinement_ratio)
		: SPHAdaptation(sph_body.getSPHSystem().resolution_ref_, h_spacing_ratio, system_refinement_ratio){
		//Added by Haotian Shi from SJTU
		body_name_ = sph_body.getName();
		std::string first_six = body_name_.substr(0, 6);
		if (first_six == "PDBody") {
			kernel_ptr_ = makeUnique<Winfunc>(h_ref_);
		}
		//==============================
	};
	//=================================================================================================//
	Real SPHAdaptation::MostRefinedSpacing(Real coarse_particle_spacing, int refinement_level)
	{
		return coarse_particle_spacing / powerN(2.0, refinement_level);
	}
	//=================================================================================================//
	Real SPHAdaptation::computeReferenceNumberDensity(Vec2d zero)
	{
		Real sigma(0);
		Real cutoff_radius = kernel_ptr_->CutOffRadius();
		Real particle_spacing = ReferenceSpacing();
		int search_depth = int(cutoff_radius / particle_spacing) + 1;
		for (int j = -search_depth; j <= search_depth; ++j)
			for (int i = -search_depth; i <= search_depth; ++i)
			{
				Vec2d particle_location(i * particle_spacing, j * particle_spacing);
				Real distance = particle_location.norm();
				if (distance < cutoff_radius)
					sigma += kernel_ptr_->W(distance, particle_location);
			}
		return sigma;
	}
	//=================================================================================================//
	Real SPHAdaptation::computeReferenceNumberDensity(Vec3d zero)
	{
		Real sigma(0);
		Real cutoff_radius = kernel_ptr_->CutOffRadius();
		Real particle_spacing = ReferenceSpacing();
		int search_depth = int(cutoff_radius / particle_spacing) + 1;
		for (int k = -search_depth; k <= search_depth; ++k)
			for (int j = -search_depth; j <= search_depth; ++j)
				for (int i = -search_depth; i <= search_depth; ++i)
				{
					Vec3d particle_location(i * particle_spacing,
											j * particle_spacing, k * particle_spacing);
					Real distance = particle_location.norm();
					if (distance < cutoff_radius)
						sigma += kernel_ptr_->W(distance, particle_location);
				}
		return sigma;
	}
	//=================================================================================================//
	Real SPHAdaptation::ReferenceNumberDensity(Real smoothing_length_ratio)
	{
		return sigma0_ref_ * powerN(smoothing_length_ratio, Dimensions);
	}
	//=================================================================================================//
	void SPHAdaptation::resetAdaptationRatios(Real h_spacing_ratio, Real new_system_refinement_ratio)
	{
		h_spacing_ratio_ = h_spacing_ratio;
		spacing_ref_ = spacing_ref_ * system_refinement_ratio_ / new_system_refinement_ratio;
		system_refinement_ratio_ = new_system_refinement_ratio;
		h_ref_ = h_spacing_ratio_ * spacing_ref_;
		kernel_ptr_.reset(new KernelWendlandC2(h_ref_));
		sigma0_ref_ = computeReferenceNumberDensity(Vecd());
		spacing_min_ = MostRefinedSpacing(spacing_ref_, local_refinement_level_);
		h_ratio_max_ = h_ref_ * spacing_ref_ / spacing_min_;
	}
	//=================================================================================================//
	UniquePtr<BaseCellLinkedList> SPHAdaptation::
		createCellLinkedList(const BoundingBox &domain_bounds, RealBody &real_body)
	{
		return makeUnique<CellLinkedList>(domain_bounds, kernel_ptr_->CutOffRadius(), real_body, *this);
	}
	//=================================================================================================//
	UniquePtr<BaseLevelSet> SPHAdaptation::createLevelSet(Shape &shape, Real refinement_ratio)
	{
		// estimate the required mesh levels
		size_t total_levels = (int)log10(MinimumDimension(shape.getBounds()) / ReferenceSpacing()) + 2;
		Real coarsest_spacing = ReferenceSpacing() * powerN(2.0, total_levels - 1);
		MultilevelLevelSet coarser_level_sets(shape.getBounds(), coarsest_spacing / refinement_ratio,
											  total_levels - 1, shape, *this);
		// return the finest level set only
		return makeUnique<RefinedLevelSet>(shape.getBounds(), *coarser_level_sets.getMeshLevels().back(), shape, *this);
	}
	//=================================================================================================//
	ParticleWithLocalRefinement::
		ParticleWithLocalRefinement(SPHBody &sph_body, Real h_spacing_ratio,
									Real system_refinement_ratio, int local_refinement_level)
		: SPHAdaptation(sph_body, h_spacing_ratio, system_refinement_ratio)
	{
		local_refinement_level_ = local_refinement_level;
		spacing_min_ = MostRefinedSpacing(spacing_ref_, local_refinement_level_);
		h_ratio_max_ = powerN(2.0, local_refinement_level_);
		// To ensure that the adaptation strictly within all level set and mesh cell linked list levels
		finest_spacing_bound_ = spacing_min_ + Eps;
		coarsest_spacing_bound_ = spacing_ref_ - Eps;
	}
	//=================================================================================================//
	size_t ParticleWithLocalRefinement::getCellLinkedListTotalLevel()
	{
		return size_t(local_refinement_level_);
	}
	//=================================================================================================//
	size_t ParticleWithLocalRefinement::getLevelSetTotalLevel()
	{
		return getCellLinkedListTotalLevel() + 1;
	}
	//=================================================================================================//
	void ParticleWithLocalRefinement::registerAdaptationVariables(BaseParticles &base_particles)
	{
		SPHAdaptation::registerAdaptationVariables(base_particles);

		base_particles.registerVariable(h_ratio_, "SmoothingLengthRatio", 1.0);
		base_particles.registerSortableVariable<Real>("SmoothingLengthRatio");
		base_particles.addVariableToReload<Real>("SmoothingLengthRatio");
	}
	//=================================================================================================//
	UniquePtr<BaseCellLinkedList> ParticleWithLocalRefinement::
		createCellLinkedList(const BoundingBox &domain_bounds, RealBody &real_body)
	{
		return makeUnique<MultilevelCellLinkedList>(domain_bounds, kernel_ptr_->CutOffRadius(),
													getCellLinkedListTotalLevel(), real_body, *this);
	}
	//=================================================================================================//
	UniquePtr<BaseLevelSet> ParticleWithLocalRefinement::createLevelSet(Shape &shape, Real refinement_ratio)
	{
		return makeUnique<MultilevelLevelSet>(shape.getBounds(), ReferenceSpacing() / refinement_ratio,
											  getLevelSetTotalLevel(), shape, *this);
	}
	//=================================================================================================//
	Real ParticleRefinementByShape::smoothedSpacing(const Real &measure, const Real &transition_thickness)
	{
		Real ratio_ref = measure / (2.0 * transition_thickness);
		Real target_spacing = coarsest_spacing_bound_;
		if (ratio_ref < kernel_ptr_->KernelSize())
		{
			Real weight = kernel_ptr_->W_1D(ratio_ref) / kernel_ptr_->W_1D(0.0);
			target_spacing = weight * finest_spacing_bound_ + (1.0 - weight) * coarsest_spacing_bound_;
		}
		return target_spacing;
	}
	//=================================================================================================//
	Real ParticleRefinementNearSurface::getLocalSpacing(Shape &shape, const Vecd &position)
	{
		Real phi = fabs(shape.findSignedDistance(position));
		return smoothedSpacing(phi, spacing_ref_);
	}
	//=================================================================================================//
	Real ParticleRefinementWithinShape::getLocalSpacing(Shape &shape, const Vecd &position)
	{
		Real phi = shape.findSignedDistance(position);
		return phi < 0.0 ? finest_spacing_bound_ : smoothedSpacing(phi, 2.0 * spacing_ref_);
	}
	//=================================================================================================//
	ParticleSplitAndMerge::ParticleSplitAndMerge(SPHBody &sph_body, Real h_spacing_ratio,
												 Real system_resolution_ratio, int local_refinement_level)
		: ParticleWithLocalRefinement(sph_body, h_spacing_ratio,
									  system_resolution_ratio, local_refinement_level)
	{
		spacing_min_ = MostRefinedSpacing(spacing_ref_, local_refinement_level_);
		h_ratio_max_ = spacing_ref_ / spacing_min_;
		minimum_volume_ = powerN(spacing_min_, Dimensions);
		maximum_volume_ = powerN(spacing_ref_, Dimensions);
	};
	//=================================================================================================//
	bool ParticleSplitAndMerge::isSplitAllowed(Real current_volume)
	{
		return current_volume - 2.0 * minimum_volume_ > -Eps ? true : false;
	}
	//=================================================================================================//
	bool ParticleSplitAndMerge::mergeResolutionCheck(Real volume)
	{
		return volume - 1.2 * powerN(spacing_min_, Dimensions) < Eps ? true : false;
	}
	//=================================================================================================//
	void ParticleSplitAndMerge::
		resetAdaptationRatios(Real h_spacing_ratio, Real new_system_refinement_ratio)
	{
		ParticleWithLocalRefinement::resetAdaptationRatios(h_spacing_ratio, new_system_refinement_ratio);
		minimum_volume_ = powerN(spacing_min_, Dimensions);
		maximum_volume_ = powerN(spacing_ref_, Dimensions);
	}
	//=================================================================================================//
	Real ParticleSplitAndMerge::MostRefinedSpacing(Real coarse_particle_spacing, int local_refinement_level)
	{
		Real minimum_spacing_particles = powerN(2.0, local_refinement_level);
		Real spacing_ratio = pow(minimum_spacing_particles, 1.0 / Dimensions);
		return coarse_particle_spacing / spacing_ratio;
	}
	//=================================================================================================//
	size_t ParticleSplitAndMerge::getCellLinkedListTotalLevel()
	{
		return 1 + (int)floor(log2(spacing_ref_ / spacing_min_));
	}
	//=================================================================================================//
	Vec2d ParticleSplitAndMerge::splittingPattern(Vec2d pos, Real particle_spacing, Real delta)
	{
		return {pos[0] + 0.5 * particle_spacing * cos(delta), pos[1] + 0.5 * particle_spacing * sin(delta)};
	}
	//=================================================================================================//
	Vec3d ParticleSplitAndMerge::splittingPattern(Vec3d pos, Real particle_spacing, Real delta)
	{
		return {pos[0] + 0.5 * particle_spacing * cos(delta), pos[1] + 0.5 * particle_spacing * sin(delta), pos[2]};
	}
	//=================================================================================================//
}
