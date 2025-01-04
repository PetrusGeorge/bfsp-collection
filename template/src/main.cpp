#include "CLI.h"
#include "Log.h"
#include "Instance.h"

#include "argparse/argparse.hpp"
#include <cstdlib>

int main(int argc, char *argv[]) {

    argparse::ArgumentParser cli("cli", "1.0", argparse::default_arguments::help);
    Parse(cli, argc, argv);

    bool verbose = cli.get<bool>("-v");

    try{
        const Instance instance(cli.get<std::string>("instance"));
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << '\n';
        exit(EXIT_FAILURE);
    }

    DEBUG_EXTRA << "Showing normal debug macro";
    DEBUG_EXTRA << "Showing extra debug macro";

    VERBOSE(verbose) << "Showing verbose macro";
}
