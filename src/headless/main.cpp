#include <cstring>

#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/testing/Testing.hpp>
#include <openvic-simulation/utility/Logger.hpp>

using namespace OpenVic;

static void print_help(std::ostream& stream, char const* program_name) {
	stream
		<< "Usage: " << program_name << " [-h] [-t] [-b <path>] [path]+\n"
		<< "    -h : Print this help message and exit the program.\n"
		<< "    -t : Run tests after loading defines.\n"
		<< "    -b : Use the following path as the base directory (instead of searching for one).\n"
		<< "    -s : Use the following path as a hint to search for a base directory.\n"
		<< "Any following paths are read as mod directories, with priority starting at one above the base directory.\n"
		<< "(Paths with spaces need to be enclosed in \"quotes\").\n";
}

static bool headless_load(GameManager& game_manager, Dataloader const& dataloader) {
	bool ret = true;

	if (!dataloader.load_defines(game_manager)) {
		Logger::error("Failed to load defines!");
		ret = false;
	}
	if (!game_manager.load_hardcoded_defines()) {
		Logger::error("Failed to load hardcoded defines!");
		ret = false;
	}
	if (!dataloader.load_localisation_files(
		[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
			return true;
		}
	)) {
		Logger::error("Failed to load localisation!");
		ret = false;
	}

	return ret;
}

static bool run_headless(Dataloader::path_vector_t const& roots, bool run_tests) {
	bool ret = true;

	Dataloader dataloader;
	if (!dataloader.set_roots(roots)) {
		Logger::error("Failed to set dataloader roots!");
		ret = false;
	}

	GameManager game_manager { []() {
		Logger::info("State updated");
	} };

	ret &= headless_load(game_manager, dataloader);

	if (run_tests) {
		Testing testing = Testing(&game_manager);
		std::cout << std::endl << "Testing Loaded" << std::endl << std::endl;
		testing.execute_all_scripts();
		testing.report_results();
		std::cout << "Testing Executed" << std::endl << std::endl;
	}

	return ret;
}

/*
	$ program [-h] [-t] [-b] [path]+
*/

int main(int argc, char const* argv[]) {
	Logger::set_logger_funcs();

	char const* program_name = StringUtils::get_filename(argc > 0 ? argv[0] : nullptr, "<program>");
	fs::path root;
	bool run_tests = false;
	int argn = 0;

	/* Reads the next argument and converts it to a path via path_transform. If reading or converting fails, an error
	 * message and the help text are displayed, along with returning false to signify the program should exit.
	 */
	const auto _read = [&root, &argn, argc, argv, program_name](
		std::string_view command, std::string_view path_use, auto path_transform) -> bool {
		if (root.empty()) {
			if (++argn < argc) {
				char const* path = argv[argn];
				root = path_transform(path);
				if (!root.empty()) {
					return true;
				} else {
					std::cerr << "Empty path after giving \"" << path << "\" to " << path_use
						<< " command line argument \"" << command << "\"." << std::endl;
				}
			} else {
				std::cerr << "Missing path after " << path_use << " command line argument \"" << command << "\"." << std::endl;
			}
		} else {
			std::cerr << "Duplicate " << path_use << " command line argument \"-b\"." << std::endl;
		}
		print_help(std::cerr, program_name);
		return false;
	};

	while (++argn < argc) {
		char const* arg = argv[argn];
		if (strcmp(arg, "-h") == 0) {
			print_help(std::cout, program_name);
			return 0;
		} else if (strcmp(arg, "-t") == 0) {
			run_tests = true;
		} else if (strcmp(arg, "-b") == 0) {
			if (!_read("-b", "base directory", std::identity {})) {
				return -1;
			}
		} else if (strcmp(arg, "-s") == 0) {
			if (!_read("-s", "search hint", Dataloader::search_for_game_path)) {
				return -1;
			}
		} else {
			break;
		}
	}
	if (root.empty()) {
		root = Dataloader::search_for_game_path();
		if (root.empty()) {
			std::cerr << "Search for base directory path failed!" << std::endl;
			print_help(std::cerr, program_name);
			return -1;
		}
	}
	Dataloader::path_vector_t roots = { root };
	while (argn < argc) {
		roots.emplace_back(root / argv[argn++]);
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	const bool ret = run_headless(roots, run_tests);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret ? "SUCCESS" : "FAILURE") << std::endl;

	return ret ? 0 : -1;
}
