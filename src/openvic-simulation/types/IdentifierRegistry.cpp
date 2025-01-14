#include "IdentifierRegistry.hpp"

#include <cassert>

using namespace OpenVic;

HasIdentifier::HasIdentifier(std::string_view new_identifier) : identifier { new_identifier } {
	assert(!identifier.empty());
}

std::ostream& OpenVic::operator<<(std::ostream& stream, HasIdentifier const& obj) {
	return stream << obj.get_identifier();
}

std::ostream& OpenVic::operator<<(std::ostream& stream, HasIdentifier const* obj) {
	return obj != nullptr ? stream << *obj : stream << "<NULL>";
}

HasColour::HasColour(colour_t new_colour, bool cannot_be_null, bool can_have_alpha) : colour(new_colour) {
	assert((!cannot_be_null || colour != NULL_COLOUR) && colour <= (!can_have_alpha ? MAX_COLOUR_RGB : MAX_COLOUR_ARGB));
}

std::string HasColour::colour_to_hex_string() const {
	return OpenVic::colour_to_hex_string(colour);
}

HasIdentifierAndColour::HasIdentifierAndColour(
	std::string_view new_identifier, colour_t new_colour, bool cannot_be_null, bool can_have_alpha
) : HasIdentifier { new_identifier }, HasColour { new_colour, cannot_be_null, can_have_alpha } {}
