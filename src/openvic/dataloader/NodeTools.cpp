#include "NodeTools.hpp"

#include <charconv>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

template<typename T>
static node_callback_t _expect_type(std::function<return_t(T const&)> callback) {
	return [callback](ast::NodeCPtr node) -> return_t {
		if (node != nullptr) {
			T const* cast_node = node->cast_to<T>();
			if (cast_node != nullptr) {
				return callback(*cast_node);
			}
			Logger::error("Invalid node type ", node->get_type(), " when expecting ", T::get_type_static());
		} else {
			Logger::error("Null node when expecting ", T::get_type_static());
		}
		return FAILURE;
	};
}

template<typename T = ast::AbstractStringNode>
requires(std::derived_from<T, ast::AbstractStringNode>)
static std::function<return_t(T const&)> abstract_string_node_callback(std::function<return_t(std::string_view)> callback) {
	return [callback](T const& node) -> return_t {
		return callback(node._name);
	};
}

node_callback_t NodeTools::expect_identifier(std::function<return_t(std::string_view)> callback) {
	return _expect_type<ast::IdentifierNode>(abstract_string_node_callback<ast::IdentifierNode>(callback));
}

node_callback_t NodeTools::expect_string(std::function<return_t(std::string_view)> callback) {
	return _expect_type<ast::StringNode>(abstract_string_node_callback<ast::StringNode>(callback));
}

node_callback_t NodeTools::expect_identifier_or_string(std::function<return_t(std::string_view)> callback) {
	return [callback](ast::NodeCPtr node) -> return_t {
		if (node != nullptr) {
			ast::AbstractStringNode const* cast_node = node->cast_to<ast::IdentifierNode>();
			if (cast_node == nullptr) {
				cast_node = node->cast_to<ast::StringNode>();
			}
			if (cast_node != nullptr) {
				return abstract_string_node_callback(callback)(*cast_node);
			}
			Logger::error("Invalid node type ", node->get_type(), " when expecting ", ast::IdentifierNode::get_type_static(), " or ", ast::StringNode::get_type_static());
		} else {
			Logger::error("Null node when expecting ", ast::IdentifierNode::get_type_static(), " or ", ast::StringNode::get_type_static());
		}
		return FAILURE;
	};
}

node_callback_t NodeTools::expect_bool(std::function<return_t(bool)> callback) {
	return expect_identifier(
		[callback](std::string_view identifier) -> return_t {
			if (identifier == "yes") {
				return callback(true);
			} else if (identifier == "no") {
				return callback(false);
			}
			Logger::error("Invalid bool identifier text: ", identifier);
			return FAILURE;
		}
	);
}

node_callback_t NodeTools::expect_int(std::function<return_t(int64_t)> callback) {
	return expect_identifier(
		[callback](std::string_view identifier) -> return_t {
			bool successful = false;
			const int64_t val = StringUtils::string_to_int64(identifier, &successful, 10);
			if (successful) {
				return callback(val);
			}
			Logger::error("Invalid int identifier text: ", identifier);
			return FAILURE;
		}
	);
}

node_callback_t NodeTools::expect_uint(std::function<return_t(uint64_t)> callback) {
	return expect_identifier(
		[callback](std::string_view identifier) -> return_t {
			bool successful = false;
			const uint64_t val = StringUtils::string_to_uint64(identifier, &successful, 10);
			if (successful) {
				return callback(val);
			}
			Logger::error("Invalid uint identifier text: ", identifier);
			return FAILURE;
		}
	);
}

node_callback_t NodeTools::expect_fixed_point(std::function<return_t(FP)> callback) {
	return expect_identifier(
		[callback](std::string_view identifier) -> return_t {
			bool successful = false;
			const FP val = FP::parse(identifier.data(), identifier.length(), &successful);
			if (successful) {
				return callback(val);
			}
			Logger::error("Invalid fixed point identifier text: ", identifier);
			return FAILURE;
		}
	);
}

node_callback_t NodeTools::expect_colour(std::function<return_t(colour_t)> callback) {
	return [callback](ast::NodeCPtr node) -> return_t {
		colour_t col = NULL_COLOUR;
		uint32_t components = 0;
		return_t ret = expect_list_of_length(3,
			expect_fixed_point(
				[&col, &components](FP val) -> return_t {
					return_t ret = SUCCESS;
					if (val < 0 || val > 255) {
						Logger::error("Invalid colour component: ", val);
						val = FP::_0();
						ret = FAILURE;
					}
					if (val <= 1) val *= 255;
					col = (col << 8) | val.to_int32_t();
					components++;
					return ret;
				}
			)
		)(node);
		if (components < 3) col <<= 8 * (3 - components);
		if (callback(col) != SUCCESS) ret = FAILURE;
		return ret;
	};
}

node_callback_t NodeTools::expect_date(std::function<return_t(Date)> callback) {
	return expect_identifier(
		[callback](std::string_view identifier) -> return_t {
			bool successful = false;
			const Date date = Date::from_string(identifier, &successful);
			if (successful) {
				return callback(date);
			}
			Logger::error("Invalid date identifier text: ", identifier);
			return FAILURE;
		}
	);
}

node_callback_t NodeTools::expect_assign(key_value_callback_t callback) {
	return _expect_type<ast::AssignNode>(
		[callback](ast::AssignNode const& assign_node) -> return_t {
			return callback(assign_node._name, assign_node._initializer.get());
		}
	);
}

node_callback_t NodeTools::expect_list_and_length(length_callback_t length_callback, node_callback_t callback) {
	return _expect_type<ast::AbstractListNode>(
		[length_callback, callback](ast::AbstractListNode const& list_node) -> return_t {
			std::vector<ast::NodeUPtr> const& list = list_node._statements;
			return_t ret = SUCCESS;
			size_t size = length_callback(list.size());
			if (size > list.size()) {
				Logger::error("Trying to read more values than the list contains: ", size, " > ", list.size());
				size = list.size();
				ret = FAILURE;
			}
			std::for_each(list.begin(), list.begin() + size,
				[callback, &ret](ast::NodeUPtr const& sub_node) -> void {
					if (callback(sub_node.get()) != SUCCESS) ret = FAILURE;
				}
			);
			return ret;
		}
	);
}

node_callback_t NodeTools::expect_list_of_length(size_t length, node_callback_t callback) {
	return [length, callback](ast::NodeCPtr node) -> return_t {
		return_t ret = SUCCESS;
		if (expect_list_and_length(
			[length, &ret](size_t size) -> size_t {
				if (size != length) {
					Logger::error("List length ", size, " does not match expected length ", length);
					ret = FAILURE;
					if (length < size) return length;
				}
				return size;
			},
			callback
		)(node) != SUCCESS) ret = FAILURE;
		return ret;
	};
}

node_callback_t NodeTools::expect_list(node_callback_t callback) {
	return expect_list_and_length(default_length_callback, callback);
}

node_callback_t NodeTools::expect_dictionary_and_length(length_callback_t length_callback, key_value_callback_t callback) {
	return expect_list_and_length(length_callback, expect_assign(callback));
}

node_callback_t NodeTools::expect_dictionary(key_value_callback_t callback) {
	return expect_dictionary_and_length(default_length_callback, callback);
}

node_callback_t NodeTools::_expect_dictionary_keys_and_length(length_callback_t length_callback, bool allow_other_keys, key_map_t&& key_map) {
	return [length_callback, allow_other_keys, key_map = std::move(key_map)](ast::NodeCPtr node) mutable -> return_t {
		return_t ret = expect_dictionary_and_length(
			length_callback,
			[&key_map, allow_other_keys](std::string_view key, ast::NodeCPtr value) -> return_t {
				const key_map_t::iterator it = key_map.find(key);
				if (it == key_map.end()) {
					if (allow_other_keys) return SUCCESS;
					Logger::error("Invalid dictionary key: ", key);
					return FAILURE;
				}
				dictionary_entry_t& entry = it->second;
				if (++entry.count > 1 && !entry.can_repeat()) {
					Logger::error("Invalid repeat of dictionary key: ", key);
					return FAILURE;
				}
				return entry.callback(value);
			}
		)(node);
		for (key_map_t::value_type const& key_entry : key_map) {
			dictionary_entry_t const& entry = key_entry.second;
			if (entry.must_appear() && entry.count < 1) {
				Logger::error("Mandatory dictionary key not present: ", key_entry.first);
				ret = FAILURE;
			}
		}
		return ret;
	};
}

node_callback_t NodeTools::name_list_callback(std::vector<std::string>& list) {
	return expect_list_reserve_length(
		list,
		expect_identifier_or_string(
			[&list](std::string_view str) -> return_t {
				if (!str.empty()) {
					list.push_back(std::string { str });
					return SUCCESS;
				}
				Logger::error("Empty identifier or string");
				return FAILURE;
			}
		)
	);
}
