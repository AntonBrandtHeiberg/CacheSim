#!/bin/bash

echo "Build CacheSim..."
g++ -std=c++20 main.cpp Cache.cpp Memory.cpp -o CacheSim

echo -e "Running CacheSim... \n"
./CacheSim