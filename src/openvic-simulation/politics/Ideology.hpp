#pragma once

#include "types/Date.hpp"
#include "types/IdentifierRegistry.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	struct IdeologyManager;

	struct IdeologyGroup : HasIdentifier {
		friend struct IdeologyManager;

	private:
		IdeologyGroup(const std::string_view new_identifier);
	
	public:
		IdeologyGroup(IdeologyGroup&&) = default;
	};

	struct Ideology : HasIdentifierAndColour {
		friend struct IdeologyManager;

	private:
		IdeologyGroup const& group;
		const bool uncivilised;
		const Date spawn_date;

		//TODO - willingness to repeal/pass reforms (and its modifiers)

		Ideology(const std::string_view new_identifier, colour_t new_colour, IdeologyGroup const& new_group, bool uncivilised, Date spawn_date);

	public:
		Ideology(Ideology&&) = default;
		bool is_uncivilised() const;
		Date const& get_spawn_date() const;
	};

	struct IdeologyManager {
	private:
		IdentifierRegistry<IdeologyGroup> ideology_groups;
		IdentifierRegistry<Ideology> ideologies;

	public:
		IdeologyManager();
		
		bool add_ideology_group(const std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(IdeologyGroup, ideology_group)

		bool add_ideology(const std::string_view identifier, colour_t colour, IdeologyGroup const* group, bool uncivilised, Date spawn_date);
		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(Ideology, ideology, ideologies)

		bool load_ideology_file(ast::NodeCPtr root);
	};
}