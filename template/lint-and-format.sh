clang-tidy -p build {src/*.cpp,include/*.h} # Maybe receive build dir as a argument 
clang-format -i {src/*.cpp,include/*.h}
