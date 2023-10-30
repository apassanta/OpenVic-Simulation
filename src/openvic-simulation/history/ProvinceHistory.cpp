#include "ProvinceHistory.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ProvinceHistory::ProvinceHistory(
	Country const* new_owner, Country const* new_controller, uint8_t new_colonial, bool new_slave,
	std::vector<Country const*>&& new_cores, Good const* new_rgo, uint8_t new_life_rating, TerrainType const* new_terrain_type,
	std::map<Building const*, uint8_t>&& new_buildings, std::map<Ideology const*, uint8_t>&& new_party_loyalties
) : owner { new_owner }, controller { new_controller }, colonial { new_colonial }, slave { new_slave },
	cores { std::move(new_cores) }, rgo { new_rgo }, life_rating { new_life_rating }, terrain_type { new_terrain_type },
	buildings { std::move(new_buildings) }, party_loyalties { std::move(new_party_loyalties) } {}

Country const* ProvinceHistory::get_owner() const {
	return owner;
}

Country const* ProvinceHistory::get_controller() const {
	return controller;
}

uint8_t ProvinceHistory::get_colony_status() const {
	return colonial;
}

bool ProvinceHistory::is_slave() const {
	return slave;
}

std::vector<Country const*> const& ProvinceHistory::get_cores() const {
	return cores;
}

bool ProvinceHistory::is_core_of(Country const* country) const {
	return std::find(cores.begin(), cores.end(), country) != cores.end();
}

Good const* ProvinceHistory::get_rgo() const {
	return rgo;
}

uint8_t ProvinceHistory::get_life_rating() const {
	return life_rating;
}

TerrainType const* ProvinceHistory::get_terrain_type() const {
	return terrain_type;
}

std::map<Building const*, uint8_t> const& ProvinceHistory::get_buildings() const {
	return buildings;
}

std::map<Ideology const*, uint8_t> const& ProvinceHistory::get_party_loyalties() const {
	return party_loyalties;
}

bool ProvinceHistoryManager::add_province_history_entry(
	Province const* province, Date date, Country const* owner, Country const* controller, std::optional<uint8_t>&& colonial,
	std::optional<bool>&& slave, std::vector<Country const*>&& cores, std::vector<Country const*>&& remove_cores,
	Good const* rgo, std::optional<uint8_t>&& life_rating, TerrainType const* terrain_type,
	std::optional<std::map<Building const*, uint8_t>>&& buildings,
	std::optional<std::map<Ideology const*, uint8_t>>&& party_loyalties
) {
	if (locked) {
		Logger::error("Cannot add new history entry to province history registry: locked!");
		return false;
	}

	/* combine duplicate histories, priority to current (defined later) */
	auto& province_registry = province_histories[province];
	const auto existing_entry = province_registry.find(date);

	if (existing_entry != province_registry.end()) {
		if (owner != nullptr) {
			existing_entry->second.owner = owner;
		}
		if (controller != nullptr) {
			existing_entry->second.controller = controller;
		}
		if (rgo != nullptr) {
			existing_entry->second.rgo = rgo;
		}
		if (terrain_type != nullptr) {
			existing_entry->second.terrain_type = terrain_type;
		}
		if (colonial) {
			existing_entry->second.colonial = *colonial;
		}
		if (slave) {
			existing_entry->second.slave = *slave;
		}
		if (life_rating) {
			existing_entry->second.life_rating = *life_rating;
		}
		if (buildings) {
			existing_entry->second.buildings = std::move(*buildings);
		}
		if (party_loyalties) {
			existing_entry->second.party_loyalties = std::move(*party_loyalties);
		}
		// province history cores are additive
		existing_entry->second.cores.insert(existing_entry->second.cores.end(), cores.begin(), cores.end());
		for (const auto which : remove_cores) {
			const auto core = std::find(cores.begin(), cores.end(), which);
			if (core == cores.end()) {
				Logger::error(
					"In history of province ", province->get_identifier(), " tried to remove nonexistant core of country: ",
					which->get_identifier(), " at date ", date.to_string()
				);
				return false;
			}
			existing_entry->second.cores.erase(core);
		}
	} else {
		province_registry.emplace(date, ProvinceHistory {
			owner, controller, *colonial, *slave, std::move(cores), rgo, *life_rating,
			terrain_type, std::move(*buildings), std::move(*party_loyalties)
		});
	}
	return true;
}

void ProvinceHistoryManager::lock_province_histories() {
	for (auto const& entry : province_histories) {
		if (entry.second.size() == 0) {
			Logger::error(
				"Attempted to lock province histories - province ", entry.first->get_identifier(), " has no history entries!"
			);
		}
	}
	Logger::info("Locked province history registry after registering ", province_histories.size(), " items");
	locked = true;
}

bool ProvinceHistoryManager::is_locked() const {
	return locked;
}

ProvinceHistory const* ProvinceHistoryManager::get_province_history(Province const* province, Date entry) const {
	Date closest_entry;
	auto province_registry = province_histories.find(province);

	if (province_registry == province_histories.end()) {
		Logger::error("Attempted to access history of undefined province ", province->get_identifier());
		return nullptr;
	}

	for (auto const& current : province_registry->second) {
		if (current.first == entry) {
			return &current.second;
		}
		if (current.first > entry) {
			continue;
		}
		if (current.first > closest_entry && current.first < entry) {
			closest_entry = current.first;
		}
	}

	auto entry_registry = province_registry->second.find(closest_entry);
	if (entry_registry != province_registry->second.end()) {
		return &entry_registry->second;
	}
	/* warned about lack of entries earlier, return nullptr */
	return nullptr;
}

inline ProvinceHistory const* ProvinceHistoryManager::get_province_history(
	Province const* province, Bookmark const* bookmark
) const {
	return get_province_history(province, bookmark->get_date());
}

inline bool ProvinceHistoryManager::_load_province_history_entry(
	GameManager& game_manager, std::string_view province, Date const& date, ast::NodeCPtr root
) {
	Country const* owner = nullptr;
	Country const* controller = nullptr;
	std::vector<Country const*> cores;
	std::vector<Country const*> remove_cores;
	Good const* rgo;
	std::optional<uint8_t> life_rating, colonial;
	std::optional<bool> slave;
	TerrainType const* terrain_type;
	std::optional<std::map<Building const*, uint8_t>> buildings;
	std::optional<std::map<Ideology const*, uint8_t>> party_loyalties;

	bool ret = expect_dictionary_keys_and_default(
		[&game_manager, &buildings](std::string_view key, ast::NodeCPtr value) -> bool {
			// used for province buildings like forts or railroads
			if (game_manager.get_economy_manager().get_building_manager().has_building_identifier(key)) {
				Building const* building;
				uint8_t level;

				bool ret = game_manager.get_economy_manager().get_building_manager()
					.expect_building_str(assign_variable_callback_pointer(building))(key);
				ret &= expect_uint(assign_variable_callback(level))(value);

				if(!buildings.has_value())
					buildings = decltype(buildings)::value_type {};
				buildings->emplace(building, level);
				return ret;
			}

			bool is_date;
			Date().from_string(key, &is_date, true);
			if (is_date) {
				return true;
			}

			return key_value_invalid_callback(key, value);
		},
		"owner", ZERO_OR_ONE, game_manager.get_country_manager()
			.expect_country_identifier(assign_variable_callback_pointer(owner)),
		"controller", ZERO_OR_ONE, game_manager.get_country_manager()
			.expect_country_identifier(assign_variable_callback_pointer(controller)),
		"add_core", ZERO_OR_MORE, [&game_manager, &cores](ast::NodeCPtr node) -> bool {
			Country const* core;

			bool ret = game_manager.get_country_manager()
				.expect_country_identifier(assign_variable_callback_pointer(core))(node);
			cores.push_back(core);
			return ret;
		},
		"remove_core", ZERO_OR_MORE, [&game_manager, &remove_cores](ast::NodeCPtr node) -> bool {
			Country const* remove;

			bool ret = game_manager.get_country_manager()
				.expect_country_identifier(assign_variable_callback_pointer(remove))(node);
			remove_cores.push_back(remove);
			return ret;
		},
		"colonial", ZERO_OR_ONE, expect_uint<uint8_t>([&colonial](uint8_t colony_level) -> bool {
			colonial = colony_level;

			return true;
		}),
		"colony", ZERO_OR_ONE, expect_uint<uint8_t>([&colonial](uint8_t colony_level) -> bool {
			colonial = colony_level;

			return true;
		}),
		"is_slave", ZERO_OR_ONE, expect_bool([&slave](bool is_slave) -> bool {
			if (is_slave) {
				slave = true;
			} else {
				slave = false;
			}

			return true;
		}),
		"trade_goods", ZERO_OR_ONE, game_manager.get_economy_manager().get_good_manager()
			.expect_good_identifier(assign_variable_callback_pointer(rgo)),
		"life_rating", ZERO_OR_ONE, expect_uint<uint8_t>([&life_rating](uint8_t rating) -> bool {
			life_rating = rating;

			return true;
		}),
		"terrain", ZERO_OR_ONE, game_manager.get_map().get_terrain_type_manager()
			.expect_terrain_type_identifier(assign_variable_callback_pointer(terrain_type)),
		"party_loyalty", ZERO_OR_MORE, [&game_manager, &party_loyalties](ast::NodeCPtr node) -> bool {
			Ideology const* ideology;
			uint8_t amount; // percent I do believe

			bool ret = expect_dictionary_keys(
				"ideology", ONE_EXACTLY, game_manager.get_politics_manager().get_ideology_manager()
					.expect_ideology_identifier(assign_variable_callback_pointer(ideology)),
				"loyalty_value", ONE_EXACTLY, expect_uint(assign_variable_callback(amount))
			)(node);
			if(!party_loyalties.has_value())
				party_loyalties = decltype(party_loyalties)::value_type {};
			party_loyalties->emplace(ideology, amount);
			return ret;
		},
		"state_building", ZERO_OR_MORE, [&game_manager, &buildings](ast::NodeCPtr node) -> bool {
			Building const* building;
			uint8_t level;

			bool ret = expect_dictionary_keys(
				"level", ONE_EXACTLY, expect_uint(assign_variable_callback(level)),
				"building", ONE_EXACTLY, game_manager.get_economy_manager().get_building_manager()
					.expect_building_identifier(assign_variable_callback_pointer(building)),
				"upgrade", ZERO_OR_ONE, success_callback // doesn't appear to have an effect
			)(node);
			if(!buildings.has_value())
				buildings = decltype(buildings)::value_type {};
			buildings->emplace(building, level);
			return ret;
		}
	)(root);

	ret &= add_province_history_entry(
		game_manager.get_map().get_province_by_identifier(province), date, owner, controller, std::move(colonial), std::move(slave),
		std::move(cores), std::move(remove_cores), rgo, std::move(life_rating), terrain_type, std::move(buildings),
		std::move(party_loyalties)
	);
	return ret;
}

bool ProvinceHistoryManager::load_province_history_file(GameManager& game_manager, std::string_view name, ast::NodeCPtr root) {
	bool ret = _load_province_history_entry(game_manager, name, game_manager.get_define_manager().get_start_date(), root);

	ret &= expect_dictionary(
		[this, &game_manager, &name](std::string_view key, ast::NodeCPtr value) -> bool {
			bool is_date = false;
			Date entry = Date().from_string(key, &is_date, true);
			if (!is_date) {
				return true;
			}

			Date const& end_date = game_manager.get_define_manager().get_end_date();
			if (entry > end_date) {
				Logger::error(
					"History entry ", entry.to_string(), " of province ", name, " defined after defined end date ",
					end_date.to_string()
				);
				return false;
			}

			return _load_province_history_entry(game_manager, name, entry, value);
		}
	)(root);

	return ret;
}
