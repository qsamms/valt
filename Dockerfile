FROM ubuntu:22.04 AS dependencies

WORKDIR /app

RUN apt-get update && \
    apt-get install -y \
    libspdlog-dev \
    libssl-dev \
    libev-dev \
    libjemalloc-dev \
    libcli11-dev

FROM dependencies AS builder

WORKDIR /app

COPY src/ /app/src/
COPY CMakeLists.txt /app/CMakeLists.txt

RUN apt-get update && \
    apt-get install -y build-essential cmake pkg-config

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

FROM dependencies AS final

COPY --from=builder /app/build/valt /app/valt

WORKDIR /app

RUN useradd valt
USER valt

EXPOSE 1738
EXPOSE 6767

CMD ["/app/valt"]