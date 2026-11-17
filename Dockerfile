FROM debian:trixie-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake pkg-config \
        zlib1g-dev default-libmysqlclient-dev \
        libdrogon-dev libjsoncpp-dev uuid-dev libssl-dev libbrotli-dev libc-ares-dev \
        libyaml-cpp-dev \
        libreflectcpp-dev libyyjson-dev libctre-dev libutfcpp-dev \
    && rm -rf /var/lib/apt/lists/*

COPY pvpgn /src/pvpgn
RUN cmake -S /src/pvpgn -B /build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DWITH_MYSQL=ON -DWITH_API=ON \
        -DWITH_D2CS=OFF -DWITH_D2DBS=OFF \
    && cmake --build /build --parallel "$(nproc)" \
    && cmake --install /build


FROM debian:trixie-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        libdrogon1t64 libreflectcpp0.18 libyyjson0 libmariadb3 zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /usr/local/ /usr/local/
COPY docker/bnetd-entrypoint.sh /usr/local/bin/bnetd-entrypoint.sh

# The client support files listed in supportfile.conf (*.mpq, icons, ...) are
# not part of the repository and bnetd refuses to start without them, so the
# check is disabled: this image is for API/integration tests, not game clients.
RUN : > /usr/local/etc/supportfile.conf

# 6112: game protocol, 6200: Warcraft III routing, 6180: REST API
EXPOSE 6112/tcp 6112/udp 6200/tcp 6180/tcp

ENTRYPOINT ["/usr/local/bin/bnetd-entrypoint.sh"]
