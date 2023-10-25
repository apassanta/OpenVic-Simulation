#pragma once

#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/Unit.hpp"

namespace OpenVic {
	struct MilitaryManager {
	private:
		UnitManager unit_manager;
		LeaderTraitManager leader_trait_manager;
		DeploymentManager deployment_manager;

	public:
		REF_GETTERS(unit_manager)
		REF_GETTERS(leader_trait_manager)
		REF_GETTERS(deployment_manager)
	};
}