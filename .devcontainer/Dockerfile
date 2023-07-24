# Use the official Ubuntu image as the base
FROM ubuntu:latest

# Install required packages
RUN apt-get update && \
    apt-get install -y g++ gdb cmake make git libibverbs-dev && \
    rm -rf /var/lib/apt/lists/*