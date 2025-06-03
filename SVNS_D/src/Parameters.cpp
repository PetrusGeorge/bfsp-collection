#include "Parameters.h"

#include "argparse/argparse.hpp"
#include <cstddef>

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    // Config the arguments to be received
    cli.add_argument("instance").help("instance path");

    cli.add_argument("-s", "--seed").help("set random number generator seed").metavar("SEED").scan<'i', size_t>();
    cli.add_argument("-r", "--ro").help("set ro").metavar("ro").scan<'i', size_t>();
    cli.add_argument("-v", "--verbose").help("set programverbosity").metavar("VERBOSE").default_value(false).flag();
    cli.add_argument("-b", "--beta").help("set beta").metavar("beta").scan<'g', double>().default_value(0.75);
    cli.add_argument("-d", "--d").help("set d").metavar("destroy").scan<'i', size_t>();
    cli.add_argument("-b", "--benchmark")
        .help("set program to benchmark mode")
        .metavar("BENCHMARK")
        .default_value(false)
        .flag();
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
    m_beta = cli.get<double>("--beta");
    m_benchmark = cli.get<bool>("--benchmark");

    if (m_beta == 0) {
        m_alpha = 0.75;
        m_d = 8;
    } else if (m_beta == 0.25) {
        m_alpha = 0;
        m_d = 4;
    } else if (m_beta == 0.5) {
        m_alpha = 0;
        m_d = 4;
    } else if (m_beta == 0.75) {
        m_alpha = 0;
        m_d = 4;
    } else if (m_beta == 1) {
        m_alpha = 0.75;
        m_d = 8;
    }
}
