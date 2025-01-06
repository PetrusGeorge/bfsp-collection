#include <cstdlib>

#include "argparse/argparse.hpp"

#include "CLI.h"
#include "Instance.h"
#include "Log.h"

int main(int argc, char *argv[]) {

    argparse::ArgumentParser cli("template", "1.0", argparse::default_arguments::help);
    parse(cli, argc, argv);

    const bool verbose = cli.get<bool>("-v");

    try {
        const Instance instance(cli.get<std::string>("instance"));
    } catch (const std::runtime_error &err) {
        std::cout << err.what() << '\n';
        exit(EXIT_FAILURE);
    }

    DEBUG_EXTRA << "Showing normal debug macro";
    DEBUG_EXTRA << "Showing extra debug macro";

    VERBOSE(verbose) << "Showing verbose macro";
}
