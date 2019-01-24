#!/usr/bin/env bash

scriptdir="$(dirname "$0")"
export NOAUTODEPS=1
export SNMP_VERBOSE=1
case $(uname) in
    MINGW64*|MSYS*)
	pacman --noconfirm --sync --refresh
	pacman --noconfirm --sync --needed diffutils
	pacman --noconfirm --sync --needed make
	pacman --noconfirm --sync --needed openssl-devel
	pacman --noconfirm --sync --needed mingw-w64-x86_64-gcc
	pacman --noconfirm --sync --needed mingw-w64-x86_64-openssl
	;;
esac
case "${BUILD}" in
    MSYS2)
	"${scriptdir}"/net-snmp-configure master CC=/usr/bin/gcc || exit $?
	;;
    MinGW64)
	"${scriptdir}"/net-snmp-configure master CC=/mingw64/bin/gcc || exit $?
	;;
    *)
	"${scriptdir}"/net-snmp-configure master || exit $?
	;;
esac
make -s                                  || exit $?
case "$MODE" in
    disable-set|mini*|read-only)
        exit 0;;
esac
[ -n "$APPVEYOR" ]			 && exit 0
"${scriptdir}"/net-snmp-run-tests        || exit $?
