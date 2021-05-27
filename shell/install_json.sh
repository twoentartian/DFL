#!/bin/bash

cd ..
mkdir "3rd"
sudo chmod 777 ./3rd
cd "3rd" || exit

git clone https://github.com/nlohmann/json.git

