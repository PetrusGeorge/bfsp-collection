#include "Parameters.h"
#include <iostream>

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    // Config the arguments to be received
    cli.add_argument("instance")
        .help("instance path");

    cli.add_argument("-s", "--seed")
        .help("set random number generator seed")
        .metavar("SEED")
        .scan<'i', size_t>();
    
    cli.add_argument("-t", "--time")
        .help("set the time limit for the program")
        .metavar("TIME_LIMIT")
        .scan<'i', size_t>();

    cli.add_argument("-v", "--verbose")
        .help("set program verbosity")
        .metavar("VERBOSE")
        .default_value(false)
        .flag();

    cli.add_argument("-b", "--benchmark")
        .help("set program to benchmark mode")
        .metavar("BENCHMARK")
        .default_value(false)
        .flag();
    
    cli.add_argument("-bt", "--beta")
        .help("set the beta parameter indicate probability of a local search")
        .metavar("BETA")
        .default_value((double)1)
        .scan<'f', double>();

    cli.add_argument("-a", "--alpha")
        .help("set the alpha parameter determine de acceptance criterion")
        .metavar("ALPHA")
        .default_value((double)0.5)
        .scan<'f', double>();

    cli.add_argument("-k", "--k")
        .help("set the k parameter which delimits the time limit of the program")
        .metavar("K")
        .default_value(size_t(30))
        .scan<'i', size_t>();
    
    cli.add_argument("-d", "--destroy")
        .help("set the d parameter the number of nodes to remove with in a destroy")
        .metavar("DESTROY")
        .default_value(size_t(6))
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
    m_benchmark = cli.get<bool>("--benchmark");
    m_seed = cli.present<size_t>("--seed");
    m_alpha = cli.get<double>("--alpha");
    m_beta = cli.get<double>("--beta");
    m_k = cli.get<size_t>("--k");
    m_d = cli.get<size_t>("--destroy");
}