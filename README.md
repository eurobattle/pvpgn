# PvPGN

This is Eurobattle.net's fork of PvPGN software.

Build:
```
cd pvpgpn
cmake -B build -DWITH_MYSQL=ON
cd build
make
sudo make install
```

Everything is installed under `/usr/local` by default. To change that, set `CMAKE_INSTALL_PREFIX` when configuring. For example, for `/opt/eurobattle/pvpgn`:

```
cmake -B build -DWITH_MYSQL=ON -DCMAKE_INSTALL_PREFIX=/opt/eurobattle/pvpgn
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
