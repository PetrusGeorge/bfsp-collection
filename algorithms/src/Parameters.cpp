#include "Parameters.h"

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    // Config the arguments to be received
    cli.add_argument("instance").help("instance path");

    cli.add_argument("-s", "--seed").help("set random number generator seed").metavar("SEED").scan<'i', size_t>();

    cli.add_argument("-v", "--verbose").help("set program verbosity").metavar("VERBOSE").default_value(false).flag();

    cli.add_argument("-r", "--ro")
        .help("set the ro parameter which delimits the time limit of the program")
        .metavar("RO")
        .default_value(size_t(30))
        .scan<'i', size_t>();
}
} // namespace

Parameters::Parameters(int argc, char **argv) {

    argparse::ArgumentParser cli("template", "1.0", argparse::default_arguments::help);

    config_argparse(cli);

    // Parse arguments
    try {
        cli.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cout << err.what() << '\n';
        std::cout << cli;
        exit(EXIT_FAILURE);
    }

    // Set members
    m_instance_path = cli.get<std::string>("instance");
    m_verbose = cli.get<bool>("--verbose");
    m_seed = cli.present<size_t>("--seed");
    m_ro = cli.get<size_t>("--ro");
}
