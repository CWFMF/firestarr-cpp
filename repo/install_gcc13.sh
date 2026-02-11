sudo apt-get install gcc-13 g++-13

sudo update-alternatives --remove-all gcc
sudo update-alternatives --remove-all g++

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 20

sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 10
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 20

sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 30
sudo update-alternatives --set cc /usr/bin/gcc

sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 30
sudo update-alternatives --set c++ /usr/bin/g++

# sudo update-alternatives --config gcc
# sudo update-alternatives --config g++
