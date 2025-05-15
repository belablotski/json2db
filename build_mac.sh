#!/bin/bash
# This script builds the json2db tool on macOS using Clang.
# It installs the necessary dependencies using Homebrew and compiles the source code.

brew install clang
brew install libpqxx
brew install libpq

/usr/bin/clang++ \
  -arch arm64 \
  -std=c++20 \
  -I/opt/homebrew/Cellar/nlohmann-json/3.12.0/include/ \
  -I/opt/homebrew/Cellar/libpqxx/7.10.1/include/ \
  -I/opt/homebrew/Cellar/libpq/17.5/include/ \
  -L/opt/homebrew/Cellar/libpqxx/7.10.1/lib/ \
  -L/opt/homebrew/Cellar/libpq/17.5/lib/ \
  json2db.cpp -o json2db \
  -lpqxx -lpq

echo "Build complete. You can run the program with ./json2db"
