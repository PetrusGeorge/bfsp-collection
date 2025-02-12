cpp_files=$(find include -type f -name "*.h" && find src -type f -name "*.cpp")

clang-tidy -p build $cpp_files --use-color # Maybe receive build dir as a argument 
clang-format -i $cpp_files --verbose
