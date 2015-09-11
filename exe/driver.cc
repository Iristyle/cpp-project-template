#include <cpp_project_template/project.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/nowide/args.hpp>
#include <leatherman/logging/logging.hpp>

// boost includes are not always warning-clean. Disable warnings that
// cause problems before including the headers, then re-enable the warnings.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <boost/program_options.hpp>
#pragma GCC diagnostic pop

#include <windows.h>

using namespace std;
using namespace leatherman::logging;
namespace po = boost::program_options;

static HANDLE halt_event;

BOOL ctrl_handler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
            LOG_DEBUG("Received Ctrl-C, shutting down");
            SetEvent(halt_event);
            return TRUE;
        case CTRL_CLOSE_EVENT:
            LOG_DEBUG("Received close event, shutting down");
            SetEvent(halt_event);
            return TRUE;
        case CTRL_SHUTDOWN_EVENT:
            LOG_DEBUG("Received shutdown event, shutting down");
            SetEvent(halt_event);
            return TRUE;
        default:
            return FALSE;
    }
}

void help(po::options_description& desc)
{
    boost::nowide::cout <<
        "Synopsys\n"
        "========\n"
        "\n"
        "Example command-line utility.\n"
        "\n"
        "Usage\n"
        "=====\n"
        "\n"
        "  driver [options]\n"
        "\n"
        "Options\n"
        "=======\n\n" << desc <<
        "\nDescription\n"
        "===========\n"
        "\n"
        "Displays its own version string." << endl;
}

int main(int argc, char **argv) {
    try {
        // Fix args on Windows to be UTF-8
        boost::nowide::args arg_utf8(argc, argv);

        // Setup logging
        setup_logging(boost::nowide::cerr);

        po::options_description command_line_options("");
        command_line_options.add_options()
            ("help,h", "produce help message")
            ("log-level,l", po::value<log_level>()->default_value(log_level::warning, "warn"), "Set logging level.\nSupported levels are: none, trace, debug, info, warn, error, and fatal.")
            ("version,v", "print the version and exit");

        po::variables_map vm;

        try {
            po::store(po::parse_command_line(argc, argv, command_line_options), vm);

            if (vm.count("help")) {
                help(command_line_options);
                return EXIT_SUCCESS;
            }

            po::notify(vm);
        } catch (exception& ex) {
            colorize(boost::nowide::cerr, log_level::error);
            boost::nowide::cerr << "error: " << ex.what() << "\n" << endl;
            colorize(boost::nowide::cerr);
            help(command_line_options);
            return EXIT_FAILURE;
        }

        // Get the logging level
        auto lvl = vm["log-level"].as<log_level>();
        set_level(lvl);

        if (vm.count("version")) {
            boost::nowide::cout << cpp_project_template::version() << endl;
            return EXIT_SUCCESS;
        }
    } catch (exception& ex) {
        colorize(boost::nowide::cerr, log_level::fatal);
        boost::nowide::cerr << "unhandled exception: " << ex.what() << "\n" << endl;
        colorize(boost::nowide::cerr);
        return EXIT_FAILURE;
    }

    halt_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (SetConsoleCtrlHandler(static_cast<PHANDLER_ROUTINE>(ctrl_handler), TRUE)) {
        LOG_DEBUG("Control handler installed");
    } else {
        LOG_ERROR("Could not set control handler");
        return EXIT_FAILURE;
    }

    boost::nowide::cout << "Service starting up" << endl;

    DWORD wait_status;
    while((wait_status = WaitForSingleObject(halt_event, 1000)) == WAIT_TIMEOUT) {
        boost::nowide::cout << "Hello!" << endl;
    }
    if (wait_status != WAIT_OBJECT_0) {
        LOG_ERROR("Wait failed with error %1%", wait_status);
        return EXIT_FAILURE;
    }

    boost::nowide::cout << "Service shutting down" << endl;

    return error_has_been_logged() ? EXIT_FAILURE : EXIT_SUCCESS;
}
