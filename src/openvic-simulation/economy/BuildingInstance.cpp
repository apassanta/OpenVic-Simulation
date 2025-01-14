#include "BuildingInstance.hpp"

using namespace OpenVic;

BuildingInstance::BuildingInstance(BuildingType const& new_building_type, level_t new_level)
	: HasIdentifier { new_building_type.get_identifier() }, building_type { new_building_type }, level { new_level },
	expansion_state { ExpansionState::CannotExpand } {}

bool BuildingInstance::_can_expand() const {
	return level < building_type.get_max_level();
}

bool BuildingInstance::expand() {
	if (expansion_state == ExpansionState::CanExpand) {
		expansion_state = ExpansionState::Preparing;
		expansion_progress = 0.0f;
		return true;
	}
	return false;
}

/* REQUIREMENTS:
 * MAP-71, MAP-74, MAP-77
 */
void BuildingInstance::update_state(Date today) {
	switch (expansion_state) {
	case ExpansionState::Preparing:
		start_date = today;
		end_date = start_date + building_type.get_build_time();
		break;
	case ExpansionState::Expanding:
		expansion_progress = static_cast<double>(today - start_date) / static_cast<double>(end_date - start_date);
		break;
	default: expansion_state = _can_expand() ? ExpansionState::CanExpand : ExpansionState::CannotExpand;
	}
}

void BuildingInstance::tick(Date today) {
	if (expansion_state == ExpansionState::Preparing) {
		expansion_state = ExpansionState::Expanding;
	}
	if (expansion_state == ExpansionState::Expanding) {
		if (end_date <= today) {
			level++;
			expansion_state = ExpansionState::CannotExpand;
		}
	}
}
