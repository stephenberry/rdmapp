# Use the official Ubuntu image as the base
FROM ubuntu:latest

# Install required packages
RUN apt-get update && \
    apt-get install -y g++ gdb cmake make git libibverbs-dev && \
    rm -rf /var/lib/apt/lists/*

# Install GCC 13
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Verify the installation
RUN gcc --version && g++ --version
