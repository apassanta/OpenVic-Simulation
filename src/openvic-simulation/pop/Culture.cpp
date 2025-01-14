#include "Culture.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GraphicalCultureType::GraphicalCultureType(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

CultureGroup::CultureGroup(
	std::string_view new_identifier, std::string_view new_leader, GraphicalCultureType const& new_unit_graphical_culture_type,
	bool new_is_overseas
) : HasIdentifier { new_identifier }, leader { new_leader }, unit_graphical_culture_type { new_unit_graphical_culture_type },
	is_overseas { new_is_overseas } {}

Culture::Culture(
	std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group,
	std::vector<std::string>&& new_first_names, std::vector<std::string>&& new_last_names
) : HasIdentifierAndColour { new_identifier, new_colour, false, false }, group { new_group },
	first_names { std::move(new_first_names) }, last_names { std::move(new_last_names) } {}

CultureManager::CultureManager()
	: graphical_culture_types { "graphical culture types" }, culture_groups { "culture groups" }, cultures { "cultures" } {}

bool CultureManager::add_graphical_culture_type(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return false;
	}
	return graphical_culture_types.add_item({ identifier });
}

bool CultureManager::add_culture_group(
	std::string_view identifier, std::string_view leader, GraphicalCultureType const* graphical_culture_type, bool is_overseas
) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot register culture groups until graphical culture types are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return false;
	}
	if (leader.empty()) {
		Logger::error("Invalid culture group leader - empty!");
		return false;
	}
	if (graphical_culture_type == nullptr) {
		Logger::error("Null graphical culture type for ", identifier);
		return false;
	}
	return culture_groups.add_item({ identifier, leader, *graphical_culture_type, is_overseas });
}

bool CultureManager::add_culture(
	std::string_view identifier, colour_t colour, CultureGroup const& group, std::vector<std::string>&& first_names,
	std::vector<std::string>&& last_names
) {
	if (!culture_groups.is_locked()) {
		Logger::error("Cannot register cultures until culture groups are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture identifier - empty!");
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid culture colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	return cultures.add_item({ identifier, colour, group, std::move(first_names), std::move(last_names) });
}

bool CultureManager::load_graphical_culture_type_file(ast::NodeCPtr root) {
	const bool ret = expect_list_reserve_length(graphical_culture_types,
		expect_identifier(
			std::bind(&CultureManager::add_graphical_culture_type, this, std::placeholders::_1)
		)
	)(root);
	lock_graphical_culture_types();
	return ret;
}

bool CultureManager::_load_culture_group(
	size_t& total_expected_cultures, GraphicalCultureType const* default_unit_graphical_culture_type,
	std::string_view culture_group_key, ast::NodeCPtr culture_group_node
) {

	std::string_view leader;
	GraphicalCultureType const* unit_graphical_culture_type = default_unit_graphical_culture_type;
	bool is_overseas = true;

	bool ret = expect_dictionary_keys_and_default(
		increment_callback(total_expected_cultures),
		"leader", ONE_EXACTLY, expect_identifier(assign_variable_callback(leader)),
		"unit", ZERO_OR_ONE,
			expect_graphical_culture_type_identifier(assign_variable_callback_pointer(unit_graphical_culture_type)),
		"union", ZERO_OR_ONE, success_callback,
		"is_overseas", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_overseas))
	)(culture_group_node);
	ret &= add_culture_group(culture_group_key, leader, unit_graphical_culture_type, is_overseas);
	return ret;
}

bool CultureManager::_load_culture(
	CultureGroup const& culture_group, std::string_view culture_key, ast::NodeCPtr culture_node
) {

	colour_t colour = NULL_COLOUR;
	std::vector<std::string> first_names, last_names;

	bool ret = expect_dictionary_keys(
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"first_names", ONE_EXACTLY, name_list_callback(move_variable_callback(first_names)),
		"last_names", ONE_EXACTLY, name_list_callback(move_variable_callback(last_names)),
		"radicalism", ZERO_OR_ONE, success_callback,
		"primary", ZERO_OR_ONE, success_callback
	)(culture_node);
	ret &= add_culture(culture_key, colour, culture_group, std::move(first_names), std::move(last_names));
	return ret;
}

/* REQUIREMENTS:
 * POP-59,  POP-60,  POP-61,  POP-62,  POP-63,  POP-64,  POP-65,  POP-66,  POP-67,  POP-68,  POP-69,  POP-70,  POP-71,
 * POP-72,  POP-73,  POP-74,  POP-75,  POP-76,  POP-77,  POP-78,  POP-79,  POP-80,  POP-81,  POP-82,  POP-83,  POP-84,
 * POP-85,  POP-86,  POP-87,  POP-88,  POP-89,  POP-90,  POP-91,  POP-92,  POP-93,  POP-94,  POP-95,  POP-96,  POP-97,
 * POP-98,  POP-99,  POP-100, POP-101, POP-102, POP-103, POP-104, POP-105, POP-106, POP-107, POP-108, POP-109, POP-110,
 * POP-111, POP-112, POP-113, POP-114, POP-115, POP-116, POP-117, POP-118, POP-119, POP-120, POP-121, POP-122, POP-123,
 * POP-124, POP-125, POP-126, POP-127, POP-128, POP-129, POP-130, POP-131, POP-132, POP-133, POP-134, POP-135, POP-136,
 * POP-137, POP-138, POP-139, POP-140, POP-141, POP-142, POP-143, POP-144, POP-145, POP-146, POP-147, POP-148, POP-149,
 * POP-150, POP-151, POP-152, POP-153, POP-154, POP-155, POP-156, POP-157, POP-158, POP-159, POP-160, POP-161, POP-162,
 * POP-163, POP-164, POP-165, POP-166, POP-167, POP-168, POP-169, POP-170, POP-171, POP-172, POP-173, POP-174, POP-175,
 * POP-176, POP-177, POP-178, POP-179, POP-180, POP-181, POP-182, POP-183, POP-184, POP-185, POP-186, POP-187, POP-188,
 * POP-189, POP-190, POP-191, POP-192, POP-193, POP-194, POP-195, POP-196, POP-197, POP-198, POP-199, POP-200, POP-201,
 * POP-202, POP-203, POP-204, POP-205, POP-206, POP-207, POP-208, POP-209, POP-210, POP-211, POP-212, POP-213, POP-214,
 * POP-215, POP-216, POP-217, POP-218, POP-219, POP-220, POP-221, POP-222, POP-223, POP-224, POP-225, POP-226, POP-227,
 * POP-228, POP-229, POP-230, POP-231, POP-232, POP-233, POP-234, POP-235, POP-236, POP-237, POP-238, POP-239, POP-240,
 * POP-241, POP-242, POP-243, POP-244, POP-245, POP-246, POP-247, POP-248, POP-249, POP-250, POP-251, POP-252, POP-253,
 * POP-254, POP-255, POP-256, POP-257, POP-258, POP-259, POP-260, POP-261, POP-262, POP-263, POP-264, POP-265, POP-266,
 * POP-267, POP-268, POP-269, POP-270, POP-271, POP-272, POP-273, POP-274, POP-275, POP-276, POP-277, POP-278, POP-279,
 * POP-280, POP-281, POP-282, POP-283, POP-284
 */
bool CultureManager::load_culture_file(ast::NodeCPtr root) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot load culture groups until graphical culture types are locked!");
		return false;
	}

	static constexpr std::string_view default_unit_graphical_culture_type_identifier = "Generic";
	GraphicalCultureType const* const default_unit_graphical_culture_type =
		get_graphical_culture_type_by_identifier(default_unit_graphical_culture_type_identifier);
	if (default_unit_graphical_culture_type == nullptr) {
		Logger::error("Failed to find default unit graphical culture type: ", default_unit_graphical_culture_type_identifier);
	}

	size_t total_expected_cultures = 0;
	bool ret = expect_dictionary_reserve_length(culture_groups,
		[this, default_unit_graphical_culture_type, &total_expected_cultures](
			std::string_view key, ast::NodeCPtr value) -> bool {
			return _load_culture_group(total_expected_cultures, default_unit_graphical_culture_type, key, value);
		}
	)(root);
	lock_culture_groups();
	cultures.reserve(cultures.size() + total_expected_cultures);

	ret &= expect_culture_group_dictionary([this](CultureGroup const& culture_group, ast::NodeCPtr culture_group_value) -> bool {
		return expect_dictionary([this, &culture_group](std::string_view key, ast::NodeCPtr value) -> bool {
			static const string_set_t reserved_keys = { "leader", "unit", "union", "is_overseas" };
			if (reserved_keys.contains(key)) {
				return true;
			}
			return _load_culture(culture_group, key, value);
		})(culture_group_value);
	})(root);
	lock_cultures();
	return ret;
}
