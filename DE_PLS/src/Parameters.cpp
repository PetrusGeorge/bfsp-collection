#include "Parameters.h"

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
    cli.add_argument("instance")
       .help("instance path");

    cli.add_argument("-s", "--seed")
       .help("set random number generator seed")
       .metavar("SEED")
       .scan<'i', size_t>();

    cli.add_argument("-v", "--verbose")
       .help("set program verbosity")
       .default_value(false)
       .flag();

    cli.add_argument("-r", "--ro")
       .help("set the ro parameter")
       .metavar("RO")
       .default_value(size_t(100000))
       .scan<'i', size_t>();

    cli.add_argument("--np")
       .help("set the population size")
       .metavar("NP")
       .default_value(size_t(20))
       .scan<'i', size_t>();

    cli.add_argument("--mutation_factor")
       .help("set the mutation factor")
       .metavar("MUTATION_FACTOR")
       .default_value(0.5)
       .scan<'g', double>();

    cli.add_argument("--crossover_rate")
       .help("set the crossover rate")
       .metavar("CROSSOVER_RATE")
       .default_value(0.9)
       .scan<'g', double>();
}
} // namespace

Parameters::Parameters(int argc, char **argv) {
    argparse::ArgumentParser cli("template", "1.0", argparse::default_arguments::help);
    config_argparse(cli);

    try {
        cli.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cout << err.what() << "\n\n" << cli << std::endl;
        std::exit(EXIT_FAILURE);
    }

    m_instance_path   = cli.get<std::string>("instance");
    m_verbose         = cli.get<bool>("--verbose");
    m_seed            = cli.present<size_t>("--seed");
    m_ro              = cli.get<size_t>("--ro");
    m_np              = cli.get<size_t>("--np");
    m_mutation_factor = cli.get<double>("--mutation_factor");
    m_crossover_rate  = cli.get<double>("--crossover_rate");

}