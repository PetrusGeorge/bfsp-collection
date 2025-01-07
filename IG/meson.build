project(
  'template',
  'cpp',
  default_options: [
    'cpp_std=c++17',
    'buildtype=release',
    'warning_level=3',
    'b_ndebug=if-release',
    'b_lto=true',
  ],
)

argparse_dep = dependency('argparse')

extra_args = ['-march=native']

subdir('src')

executable(
  'template',
  sources,
  include_directories: include_directories('include'),
  cpp_args: extra_args,
  dependencies: argparse_dep,
)
