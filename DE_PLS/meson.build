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
  license: 'MIT',
)

add_project_arguments('-D_GLIBCXX_DEBUG', language: 'cpp')
add_project_arguments('-fsanitize=address', language: 'cpp')
add_project_link_arguments('-fsanitize=address', language: 'cpp')

argparse_dep = subproject('argparse').get_variable('argparse_dep')


extra_args = ['-march=native', '-D_GLIBCXX_ASSERTIONS=1']

subdir('src')

executable(
  'algorithms',
  sources,
  include_directories: include_directories('include'),
  cpp_args: extra_args,
  dependencies: argparse_dep,
)
