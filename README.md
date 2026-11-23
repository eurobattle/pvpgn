# PvPGN

This is Eurobattle.net's fork of PvPGN software.

Install the build dependencies (Debian 13 "trixie"):
```
sudo apt update
sudo apt install build-essential cmake pkg-config \
    zlib1g-dev default-libmysqlclient-dev \
    libdrogon-dev libjsoncpp-dev uuid-dev libssl-dev libbrotli-dev libc-ares-dev \
    libyaml-cpp-dev \
    libreflectcpp-dev libyyjson-dev libctre-dev libutfcpp-dev
```

`default-libmysqlclient-dev` and `pkg-config` are needed for `WITH_MYSQL`; the packages from `libdrogon-dev` on are needed for the REST API (`WITH_API`).

Build:
```
cd pvpgn
cmake -B build -DWITH_MYSQL=ON -DWITH_API=ON
cd build
make
sudo make install
```

`WITH_API=ON` (the default) only compiles the REST API in. To turn it on, set `api_enable = true` in `bnetd.conf`; read the comments there before exposing it beyond localhost.

Everything is installed under `/usr/local` by default. To change that, set `CMAKE_INSTALL_PREFIX` when configuring. For example, for `/opt/eurobattle/pvpgn`:

```
cmake -B build -DWITH_MYSQL=ON -DWITH_API=ON -DCMAKE_INSTALL_PREFIX=/opt/eurobattle/pvpgn
cd build
make
sudo make install
```

| Directory | Contents |
|---|---|
| `/opt/eurobattle/pvpgn/bin` | client tools and utilities |
| `/opt/eurobattle/pvpgn/sbin` | servers (`bnetd`, `d2cs`, `d2dbs`, `bntrackd`) |
| `/opt/eurobattle/pvpgn/etc` | configuration |
| `/opt/eurobattle/pvpgn/var` | data and files sent to clients |
| `/opt/eurobattle/pvpgn/share/man` | man pages |

Set the prefix with `cmake`, not with `make install`: it is compiled into the servers (default configuration file path) and written into the stock configuration files.

`make install` keeps existing configuration files and installs the stock version of each one as `<name>.dist`, so after an upgrade diff the two and merge what you need. Existing files in `var/files` are kept as well. The `sql_DB_layout*.conf` files are always overwritten, as they have to match the installed bnetd.
