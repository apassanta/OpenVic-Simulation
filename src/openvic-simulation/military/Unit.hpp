#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#define UNIT_PARAMS \
	Unit::icon_t icon, std::string_view sprite, bool active, std::string_view unit_type, \
		bool floating_flag, uint32_t priority, fixed_point_t max_strength, fixed_point_t default_organisation, \
		fixed_point_t maximum_speed, fixed_point_t weighted_value, std::string_view move_sound, \
		std::string_view select_sound, Timespan build_time, Good::good_map_t &&build_cost, \
		fixed_point_t supply_consumption, Good::good_map_t &&supply_cost

#define LAND_PARAMS \
	bool primary_culture, std::string_view sprite_override, std::string_view sprite_mount, \
		std::string_view sprite_mount_attach_node, fixed_point_t reconnaissance, fixed_point_t attack, fixed_point_t defence, \
		fixed_point_t discipline, fixed_point_t support, fixed_point_t maneuver, fixed_point_t siege

#define NAVY_PARAMS \
	Unit::icon_t naval_icon, bool sail, bool transport, bool capital, fixed_point_t colonial_points, bool build_overseas, \
		uint32_t min_port_level, int32_t limit_per_port, fixed_point_t supply_consumption_score, fixed_point_t hull, \
		fixed_point_t gun_power, fixed_point_t fire_range, fixed_point_t evasion, fixed_point_t torpedo_attack

namespace OpenVic {
	struct Unit : HasIdentifier {
		using icon_t = uint32_t;

		enum struct type_t {
			LAND,
			NAVAL
		};

	private:
		const type_t type;
		const icon_t icon;
		const std::string sprite;
		const bool active;
		const std::string unit_type;
		const bool floating_flag;

		const uint32_t priority;
		const fixed_point_t max_strength;
		const fixed_point_t default_organisation;
		const fixed_point_t maximum_speed;
		const fixed_point_t weighted_value;

		const std::string move_sound;
		const std::string select_sound;

		const Timespan build_time;
		const Good::good_map_t build_cost;
		const fixed_point_t supply_consumption;
		const Good::good_map_t supply_cost;

	protected:
		Unit(std::string_view identifier, type_t type, UNIT_PARAMS);

	public:
		Unit(Unit&&) = default;

		icon_t get_icon() const;
		type_t get_type() const;
		std::string_view get_sprite() const;
		bool is_active() const;
		std::string_view get_unit_type() const;
		bool has_floating_flag() const;

		uint32_t get_priority() const;
		fixed_point_t get_max_strength() const;
		fixed_point_t get_default_organisation() const;
		fixed_point_t get_maximum_speed() const;
		fixed_point_t get_weighted_value() const;

		std::string_view get_move_sound() const;
		std::string_view get_select_sound() const;

		Timespan get_build_time() const;
		Good::good_map_t const& get_build_cost() const;
		fixed_point_t get_supply_consumption() const;
		Good::good_map_t const& get_supply_cost() const;
	};

	struct LandUnit : Unit {
		friend struct UnitManager;

	private:
		const bool primary_culture;
		const std::string sprite_override, sprite_mount, sprite_mount_attach_node;
		const fixed_point_t reconnaissance;
		const fixed_point_t attack;
		const fixed_point_t defence;
		const fixed_point_t discipline;
		const fixed_point_t support;
		const fixed_point_t maneuver;
		const fixed_point_t siege;

		LandUnit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS);

	public:
		LandUnit(LandUnit&&) = default;

		bool get_primary_culture() const;
		std::string_view get_sprite_override() const;
		std::string_view get_sprite_mount() const;
		std::string_view get_sprite_mount_attach_node() const;

		fixed_point_t get_reconnaissance() const;
		fixed_point_t get_attack() const;
		fixed_point_t get_defence() const;
		fixed_point_t get_discipline() const;
		fixed_point_t get_support() const;
		fixed_point_t get_maneuver() const;
		fixed_point_t get_siege() const;
	};

	struct NavalUnit : Unit {
		friend struct UnitManager;

	private:
		const icon_t naval_icon;
		const bool sail;
		const bool transport;
		const bool capital;
		const fixed_point_t colonial_points;
		const bool build_overseas;
		const uint32_t min_port_level;
		const int32_t limit_per_port;
		const fixed_point_t supply_consumption_score;

		const fixed_point_t hull;
		const fixed_point_t gun_power;
		const fixed_point_t fire_range;
		const fixed_point_t evasion;
		const fixed_point_t torpedo_attack;

		NavalUnit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS);

	public:
		NavalUnit(NavalUnit&&) = default;

		icon_t get_naval_icon() const;
		bool can_sail() const;
		bool is_transport() const;
		bool is_capital() const;
		fixed_point_t get_colonial_points() const;
		bool can_build_overseas() const;
		uint32_t get_min_port_level() const;
		int32_t get_limit_per_port() const;
		fixed_point_t get_supply_consumption_score() const;

		fixed_point_t get_hull() const;
		fixed_point_t get_gun_power() const;
		fixed_point_t get_fire_range() const;
		fixed_point_t get_evasion() const;
		fixed_point_t get_torpedo_attack() const;
	};

	struct UnitManager {
	private:
		IdentifierRegistry<Unit> units;

		bool _check_shared_parameters(std::string_view identifier, UNIT_PARAMS);

	public:
		UnitManager();

		bool add_land_unit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS);
		bool add_naval_unit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS);
		IDENTIFIER_REGISTRY_ACCESSORS(unit)

		bool load_unit_file(GoodManager const& good_manager, ast::NodeCPtr root);
	};
}