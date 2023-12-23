FROM alpine:3 as base

RUN apk add --no-cache \
    bash \
    curl \
    gzip \
    clang \
    libc++ \
    libc++abi \
    libc++dev \
    cmake \
    make \
    boost-dev

FROM base as builder

WORKDIR /usr/src/app

COPY . .

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

FROM base as runner

WORKDIR /usr/src/app

COPY --from=builder /usr/src/app/build/ .

ENTRYPOINT ["./main"]