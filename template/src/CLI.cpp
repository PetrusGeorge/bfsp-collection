#include "CLI.h"

#include "argparse/argparse.hpp"

void parse(argparse::ArgumentParser &cli, int argc, char **argv) {

    cli.add_argument("instance").help("Instance path");

    cli.add_argument("-s", "--seed").help("set random generator seed").metavar("SEED").scan<'i', std::size_t>();

    cli.add_argument("-v", "--verbose").help("set program verbosity").metavar("VERBOSE").default_value(false).flag();

    try {
        cli.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cout << err.what() << '\n';
        std::cout << cli;
        exit(EXIT_FAILURE);
    }
}
