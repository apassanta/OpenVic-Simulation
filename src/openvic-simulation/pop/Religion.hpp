#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

	struct ReligionManager;

	struct ReligionGroup : HasIdentifier {
		friend struct ReligionManager;

	private:
		ReligionGroup(std::string_view new_identifier);

	public:
		ReligionGroup(ReligionGroup&&) = default;
	};

	struct Religion : HasIdentifierAndColour {
		friend struct ReligionManager;

		using icon_t = uint8_t;

	private:
		ReligionGroup const& PROPERTY(group);
		const icon_t PROPERTY(icon);
		const bool PROPERTY(pagan);

		Religion(
			std::string_view new_identifier, colour_t new_colour, ReligionGroup const& new_group, icon_t new_icon,
			bool new_pagan
		);

	public:
		Religion(Religion&&) = default;
	};

	struct ReligionManager {
	private:
		IdentifierRegistry<ReligionGroup> religion_groups;
		IdentifierRegistry<Religion> religions;

	public:
		ReligionManager();

		bool add_religion_group(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(religion_group)

		bool add_religion(
			std::string_view identifier, colour_t colour, ReligionGroup const& group, Religion::icon_t icon, bool pagan
		);
		IDENTIFIER_REGISTRY_ACCESSORS(religion)

		bool load_religion_file(ast::NodeCPtr root);
	};
}
