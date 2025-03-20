#include "Parameters.h"

#include "argparse/argparse.hpp"

namespace {
void config_argparse(argparse::ArgumentParser &cli) {
  // Config the arguments to be received
  cli.add_argument("instance").help("instance path");

  cli.add_argument("-s", "--seed")
      .help("set random number generator seed")
      .metavar("SEED")
      .scan<'i', size_t>();

  cli.add_argument("-v", "--verbose")
      .help("set program verbosity")
      .metavar("VERBOSE")
      .default_value(false)
      .flag();

  cli.add_argument("-tl", "--time")
      .help("set program verbosity")
      .metavar("TIME LIMIT")
      .scan<'i', size_t>();

  cli.add_argument("-t", "--threshold")
      .help("set the threshold parameter which delimits the time limit of the program")
      .metavar("D_THRESHOLD")
      .default_value(size_t(5))
      .scan<'i', size_t>();

  cli.add_argument("-g", "--gt")
      .help("set the gt parameter which delimits the time limit of the program")
      .metavar("Gt")
      .default_value(size_t(4000))
      .scan<'i', size_t>();

  cli.add_argument("-n", "--nc")
      .help("set the nc parameter which delimits the time limit of the program")
      .metavar("Nc")
      .default_value(size_t(6))
      .scan<'i', size_t>();

  cli.add_argument("-a", "--alpha")
      .help("set the alpha parameter")
      .metavar("ALPHA")
      .default_value(0.97)
      .scan<'f', double>();

}
} // namespace

Parameters::Parameters(int argc, char **argv) {

  argparse::ArgumentParser cli("template", "1.0",
                               argparse::default_arguments::help);

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
  m_d_threshold = cli.get<size_t>("-t");
  m_Gt = cli.get<size_t>("-g");
  m_nc = cli.get<size_t>("-n");
  m_alpha = cli.get<double>("-a");
  m_time_limit = cli.present<size_t>("--time");

}
