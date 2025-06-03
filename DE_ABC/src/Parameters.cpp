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

    cli.add_argument("-t", "--time")
        .help("set the time limit for the program")
        .metavar("TIME_LIMIT")
        .scan<'i', size_t>();

    cli.add_argument("-r", "--ro")
        .help("set the ro parameter which delimits the time limit of the program")
        .metavar("RO")
        .default_value(size_t(100))
        .scan<'i', size_t>();

    cli.add_argument("-s", "--ps")
        .help("set the ps parameter which delimits the size of the population")
        .metavar("PS")
        .default_value(size_t(20))
        .scan<'i', size_t>();

    cli.add_argument("-m", "--pmu")
        .help("set the pmu parameter ")
        .metavar("PMU")
        .default_value(0.9)
        .scan<'f', double>();

    cli.add_argument("-c", "--pc").help("set the pc parameter").metavar("PC").default_value(0.1).scan<'f', double>();

    cli.add_argument("-ls", "--pls")
        .help("set the pld parameter")
        .metavar("PLS")
        .default_value(0.2)
        .scan<'f', double>();

    cli.add_argument("-t", "--theta")
        .help("set the theta parameter")
        .metavar("THETA")
        .default_value(0.75)
        .scan<'f', double>();

    cli.add_argument("-i", "--it").help("").metavar("IT").default_value(size_t(1)).scan<'i', size_t>();
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
    m_benchmark = cli.get<bool>("--benchmark");
    m_tl = cli.present<size_t>("--time");
    m_ro = cli.get<size_t>("--ro");
    m_ps = cli.get<size_t>("--ps");
    m_pmu = cli.get<double>("--pmu");
    m_pc = cli.get<double>("--pc");
    m_pls = cli.get<double>("--pls");
    m_theta = cli.get<double>("--theta");
    m_it = cli.get<size_t>("--it");
}
