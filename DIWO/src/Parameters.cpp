#include "Parameters.h"

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    // Config the arguments to be received
    cli.add_argument("instance").help("instance path");

    cli.add_argument("-s", "--seed").help("set random number generator seed").metavar("SEED").scan<'i', size_t>();

    cli.add_argument("-v", "--verbose").help("set program verbosity").metavar("VERBOSE").default_value(false).flag();
    cli.add_argument("-b", "--benchmark")
        .help("set program to benchmark mode")
        .metavar("BENCHMARK")
        .default_value(false)
        .flag();

    cli.add_argument("-r", "--ro")
        .help("set the ro parameter which delimits the time limit of the program")
        .metavar("RO")
        .default_value(size_t(30))
        .scan<'i', size_t>();

    cli.add_argument("--pls")
        .help("set the probability of running local search")
        .metavar("PLS")
        .default_value(0.15)
        .scan<'f', double>();

    cli.add_argument("--p_max")
        .help("set max population size")
        .metavar("P_MAX")
        .default_value(size_t(10))
        .scan<'i', size_t>();

    cli.add_argument("--s_min")
        .help("set minimum number of seeds")
        .metavar("S_MIN")
        .default_value(size_t(0))
        .scan<'i', size_t>();

    cli.add_argument("--s_max")
        .help("set max number of seeds")
        .metavar("S_MAX")
        .default_value(size_t(7))
        .scan<'i', size_t>();

    cli.add_argument("--sigma_min")
        .help("set minimum number of nodes destroyed at spatial dispersal")
        .metavar("SIGMA_MIN")
        .default_value(size_t(0))
        .scan<'i', size_t>();

    cli.add_argument("--sigma_max")
        .help("set max number of nodes destroyed at spatial dispersal")
        .metavar("SIGMA_MAX")
        .default_value(size_t(5))
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
    m_ro = cli.get<size_t>("--ro");

    m_pls = cli.get<double>("--pls");
    m_p_max = cli.get<size_t>("--p_max");
    m_s_min = cli.get<size_t>("--s_min");
    m_s_max = cli.get<size_t>("--s_max");
    m_sigma_min = cli.get<size_t>("--sigma_min");
    m_sigma_max = cli.get<size_t>("--sigma_max");
}
