#!/bin/sh
# Applies the container's environment to bnetd.conf and starts bnetd in the
# foreground, logging to stdout.
set -eu

CONF=/usr/local/etc/bnetd.conf

: "${BNETD_DB_HOST:=db}"
: "${BNETD_DB_NAME:=pvpgn}"
: "${BNETD_DB_USER:=pvpgn}"
: "${BNETD_DB_PASS:=pvpgn}"
: "${BNETD_API_ENABLE:=true}"
: "${BNETD_API_LISTEN_ADDR:=0.0.0.0}"
: "${BNETD_API_LISTEN_PORT:=6180}"

# set_conf KEY VALUE: replaces the uncommented "KEY = ..." line
set_conf() {
    if ! grep -q "^$1[[:space:]]*=" "$CONF"; then
        echo "bnetd-entrypoint: no \"$1\" setting in $CONF" >&2
        exit 1
    fi
    # '|' as delimiter: the values contain '/' and ';'
    sed -i "s|^$1[[:space:]]*=.*|$1 = $2|" "$CONF"
}

set_conf storage_path "\"sql:mode=mysql;host=${BNETD_DB_HOST};name=${BNETD_DB_NAME};user=${BNETD_DB_USER};pass=${BNETD_DB_PASS};default=0;prefix=pvpgn_\""
set_conf api_enable "${BNETD_API_ENABLE}"
set_conf api_listen_addr "${BNETD_API_LISTEN_ADDR}"
set_conf api_listen_port "${BNETD_API_LISTEN_PORT}"
# never announce a test server to the public tracking servers
set_conf track 0

# -D: stay in the foreground and log to stdout
exec /usr/local/sbin/bnetd -D -c "$CONF" "$@"
