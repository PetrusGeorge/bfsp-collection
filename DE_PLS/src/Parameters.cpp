#include "Parameters.h"

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    cli.add_argument("instance").help("instance path");

    cli.add_argument("-s", "--seed").help("set random number generator seed").metavar("SEED").scan<'i', size_t>();

    cli.add_argument("-v", "--verbose").help("set program verbosity").default_value(false).flag();

    cli.add_argument("-b", "--benchmark")
        .help("set program to benchmark mode")
        .metavar("BENCHMARK")
        .default_value(false)
        .flag();

    cli.add_argument("-tl", "--time").help("set the time limit").metavar("TIME LIMIT").scan<'i', size_t>();

    cli.add_argument("-r", "--ro")
        .help("set the ro parameter which delimits the time limit of the program")
        .metavar("RO")
        .default_value(size_t(100))
        .scan<'i', size_t>();

    cli.add_argument("-p", "--np")
        .help("set the np parameter which delimits the population size")
        .metavar("NP")
        .default_value(size_t(10))
        .scan<'i', size_t>();

    cli.add_argument("-d", "--delta").help("").metavar("DELTA").default_value(size_t(20)).scan<'i', size_t>();

    cli.add_argument("-x", "--x").help("").metavar("DELTA").default_value(size_t(15)).scan<'i', size_t>();

    cli.add_argument("-r", "--cr").help("").metavar("PM").default_value(0.1).scan<'f', double>();

    cli.add_argument("--beta").help("").metavar("BETA").default_value(5e-4).scan<'f', double>();

    cli.add_argument("-f", "--f").help("").metavar("F").default_value(0.1).scan<'f', double>();

    cli.add_argument("--ds-min").help("").metavar("DS-MIN").default_value(size_t(4)).scan<'i', size_t>();

    cli.add_argument("--ds-max").help("").metavar("DS-MAX").default_value(size_t(8)).scan<'i', size_t>();

    cli.add_argument("--ps-min").help("").metavar("PS-MIN").default_value(size_t(1)).scan<'i', size_t>();

    cli.add_argument("--ps-max").help("").metavar("PS-MAX").default_value(size_t(4)).scan<'i', size_t>();

    cli.add_argument("--tau-min").help("").metavar("TAU-MIN").default_value(0.1).scan<'f', double>();

    cli.add_argument("--tau-max").help("").metavar("TAU-MAX").default_value(1.0).scan<'f', double>();

    cli.add_argument("--jp-min").help("").metavar("JP-MIN").default_value(0.0).scan<'f', double>();

    cli.add_argument("--jp-max").help("").metavar("JP-MAX").default_value(1.0).scan<'f', double>();
}
} // namespace

Parameters::Parameters(int argc, char **argv) {
    argparse::ArgumentParser cli("template", "1.0", argparse::default_arguments::help);
    config_argparse(cli);

    try {
        cli.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cout << err.what() << "\n\n" << cli << '\n';
        std::exit(EXIT_FAILURE);
    }

    m_instance_path = cli.get<std::string>("instance");
    m_verbose = cli.get<bool>("--verbose");
    m_seed = cli.present<size_t>("--seed");
    m_benchmark = cli.get<bool>("--benchmark");
    m_time_limit = cli.present<size_t>("--time");
    m_ro = cli.get<size_t>("--ro");
    m_np = cli.get<size_t>("--np");
    m_delta = cli.get<size_t>("--delta");
    m_x = cli.get<size_t>("--x");
    m_cr = cli.get<double>("--cr");
    m_beta = cli.get<double>("--beta");
    m_f = cli.get<double>("--f");
    m_ds_min = cli.get<size_t>("--ds-min");
    m_ds_max = cli.get<size_t>("--ds-max");
    m_ps_min = cli.get<size_t>("--ps-min");
    m_ps_max = cli.get<size_t>("--ps-max");
    m_tau_min = cli.get<double>("--tau-min");
    m_tau_max = cli.get<double>("--tau-max");
    m_jp_min = cli.get<double>("--jp-min");
    m_jp_max = cli.get<double>("--jp-max");
}