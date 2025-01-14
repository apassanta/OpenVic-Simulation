#pragma once

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct NationalValueManager;

	struct NationalValue : HasIdentifier {
		friend struct NationalValueManager;

	private:
		const ModifierValue PROPERTY(modifiers);

		NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers);

	public:
		NationalValue(NationalValue&&) = default;
	};

	struct NationalValueManager {
	private:
		IdentifierRegistry<NationalValue> national_values;

	public:
		NationalValueManager();

		bool add_national_value(std::string_view identifier, ModifierValue&& modifiers);
		IDENTIFIER_REGISTRY_ACCESSORS(national_value)

		bool load_national_values_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic
