FROM alpine:3.17

# Install necessary packages for ibverbs
RUN apk update && apk add --no-cache libibverbs librdmacm

RUN apk update \
     && apk add --no-cache g++ gdb clang clang-extra-tools cmake make git
