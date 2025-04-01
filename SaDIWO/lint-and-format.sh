clang-tidy -p build {src/*.cpp,include/*.h} --use-color # Maybe receive build dir as a argument 
clang-format -i {src/*.cpp,include/*.h} --verbose
