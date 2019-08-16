#!/bin/bash

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-6
sudo apt-get upgrade libstdc++6
sudo apt-get install libpng16-dev
sudo apt-get install libgts-dev

sudo cp -r modelExplorerLibs /usr/modelExplorerLibs
sudo cp extraLibs/libwebp.so.6.0.1 /usr/lib/x86_64-linux-gnu/
sudo cp extraLibs/libwebp.so.6 /usr/lib/x86_64-linux-gnu/
