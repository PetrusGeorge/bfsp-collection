#include "Parameters.h"

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    // Config the arguments to be received
    cli.add_argument("instance").help("instance path");

    cli.add_argument("-s", "--seed").help("set random number generator seed").metavar("SEED").scan<'i', size_t>();

    cli.add_argument("-v", "--verbose").help("set program verbosity").metavar("VERBOSE").default_value(false).flag();

    cli.add_argument("-tl", "--time")
      .help("set the time limit")
      .metavar("TIME LIMIT")
      .scan<'i', size_t>();

    cli.add_argument("-ro", "--P")
        .help("Adjuste dynamically the time limit")
        .metavar("PS")
        .default_value(size_t(5))
        .scan<'i', size_t>();
    
    cli.add_argument("-p", "--ps")
        .help("set the parameter ps that is the maximum size population")
        .metavar("PS")
        .default_value(size_t(10))
        .scan<'i', size_t>();

    cli.add_argument("-g", "--gamma")
        .help("set the parameter p that limits the number of iterations without improvement in the solution")
        .metavar("GAMMA")
        .default_value(size_t(20))
        .scan<'i', size_t>();

    cli.add_argument("-l", "--lambda")
        .help("set the parameter lambda for PF-NEH")
        .metavar("LAMBDA")
        .default_value(size_t(20))
        .scan<'i', size_t>();

    cli.add_argument("-c", "--pc")
        .help("set the parameter pc which is the probability of a path-relink occurring")
        .metavar("PC")
        .default_value(0.2)
        .scan<'f', double>();

    cli.add_argument("-m", "--pm")
        .help("set the parameter pm which is the probability of a mutation occurring")
        .metavar("PM")
        .default_value(0.8)
        .scan<'f', double>();
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
    m_time_limit = cli.present<size_t>("--time");
    m_p = cli.get<size_t>("--P");
    m_ps = cli.get<size_t>("--ps");
    m_gamma = cli.get<size_t>("--gamma");
    m_lambda = cli.get<size_t>("--lambda");
    m_pc = cli.get<double>("--pc");
    m_pm = cli.get<double>("--pm");
}
