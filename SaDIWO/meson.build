project(
  'SaDIWO',
  'cpp',
  default_options: [
    'cpp_std=c++17',
    'buildtype=release',
    'warning_level=3',
    'b_ndebug=if-release',
    'b_lto=true',
  ],
  license: 'MIT',
)

argparse_dep = subproject('argparse').get_variable('argparse_dep')

extra_args = ['-march=native']

subdir('src')

executable(
  'SaDIWO',
  sources,
  include_directories: include_directories('include'),
  cpp_args: extra_args,
  dependencies: argparse_dep,
)
