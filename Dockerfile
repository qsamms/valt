FROM ubuntu:22.04 AS builder

WORKDIR /app

COPY src/ /app/src/
COPY CMakeLists.txt /app/CMakeLists.txt

RUN apt-get update && \
    apt-get install -y \
    pkg-config \
    build-essential \
    cmake \
    libspdlog-dev \
    libssl-dev \
    libev-dev \
    libjemalloc-dev \
    libcli11-dev

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

FROM ubuntu:22.04 AS final

COPY --from=builder /app/build/valt /app/valt

WORKDIR /app
RUN useradd valt
USER valt
EXPOSE 1738 6767

CMD ["/app/valt"]