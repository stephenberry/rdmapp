FROM alpine:3.17

RUN apk update \
     && apk add --no-cache g++ gdb cmake make git

# Build and install libibverbs from source
RUN git clone https://github.com/linux-rdma/rdma-core.git /tmp/rdma-core \
    && cd /tmp/rdma-core \
    && ./build.sh \
    && make install \
    && cd / \
    && rm -rf /tmp/rdma-core