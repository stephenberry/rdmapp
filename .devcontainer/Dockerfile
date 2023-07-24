FROM alpine:3.17

RUN apk update \
     && apk add --no-cache g++ gdb cmake make git

# Clone the rdma-core repository from GitHub
RUN git clone https://github.com/linux-rdma/rdma-core.git /tmp/rdma-core

# Install additional dependencies for building rdma-core
RUN apk add --no-cache libibverbs-dev librdmacm-dev libibumad-dev libmlx5-dev libmnl-dev

# Build and install rdma-core
WORKDIR /tmp/rdma-core
RUN ./build.sh
RUN make install

# Cleanup
WORKDIR /
RUN rm -rf /tmp/rdma-core