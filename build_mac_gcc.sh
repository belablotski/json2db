#!/bin/bash
# This script builds the json2db tool on macOS using GCC 12. 
# It installs the necessary dependencies using Homebrew and compiles the source code.
# You might want to try using clang instead of gcc, as it is the default compiler on macOS (see build_mac.sh)

brew install gcc@12
brew install libpqxx
brew install libpq

export CXXFLAGS="-std=c++20 -O3 -D_GLIBCXX_USE_CXX11_ABI=1"

/opt/homebrew/Cellar/gcc@12/12.4.0/bin/g++-12 \
  -arch arm64 \
  -I/opt/homebrew/Cellar/gcc@12/12.4.0/include/c++/12/ \
  -I/opt/homebrew/Cellar/nlohmann-json/3.12.0/include/ \
  -I/opt/homebrew/Cellar/libpqxx/7.10.1/include/ \
  -I/opt/homebrew/Cellar/libpq/17.5/include/ \
  -L/opt/homebrew/Cellar/gcc@12/12.4.0/lib/gcc/12/ \
  -L/opt/homebrew/Cellar/libpqxx/7.10.1/lib/ \
  -L/opt/homebrew/Cellar/libpq/17.5/lib/ \
  json2db.cpp -o json2db

echo "Build complete. You can run the program with ./json2db"
