#!/bin/bash

# Copyright (c) 2022 Alex313031.

YEL='\033[1;33m' # Yellow
CYA='\033[1;96m' # Cyan
RED='\033[1;31m' # Red
GRE='\033[1;32m' # Green
c0='\033[0m' # Reset Text
bold='\033[1m' # Bold Text
underline='\033[4m' # Underline Text

# Error handling
yell() { echo "$0: $*" >&2; }
die() { yell "$*"; exit 111; }
try() { "$@" || die "${RED}Failed $*"; }

# --help
displayHelp () {
	printf "\n" &&
	printf "${bold}${GRE}Script to build htop on Linux.${c0}\n" &&
	printf "${bold}${YEL}Use the --clean flag to run \`make clean\` & \`make distclean\`.${c0}\n" &&
	printf "${bold}${YEL}Use the --debug flag to make a debug build.${c0}\n" &&
	printf "${bold}${YEL}Use the --sse4 flag to make an SSE4.1 build.${c0}\n" &&
	printf "${bold}${YEL}Use the --help flag to show this help.${c0}\n" &&
	printf "\n"
}
case $1 in
	--help) displayHelp; exit 0;;
esac

makeClean () {
printf "\n" &&
printf "${YEL}Running \`make clean\` and \`make distclean\`...\n" &&
printf "${CYA}\n" &&

# Clean artifacts
export NINJA_SUMMARIZE_BUILD=1 &&

export CFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -mavx -maes" &&
export CXXFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -mavx -maes" &&
export CPPFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -mavx -maes" &&
export HWLOC_CFLAGS="-g0 -s -O3 -mavx -maes" &&
export LIBNL3GENL_CFLAGS="-g0 -s -O3 -mavx -maes" &&
export LIBNL3_CFLAGS="-g0 -s -O3 -mavx -maes" &&
export LDFLAGS="-Wl,-O3 -mavx -maes" &&

make clean && make distclean &&

printf "\n" &&
printf "${GRE}${bold}Done.\n" &&
printf "\n" &&
tput sgr0
}
case $1 in
	--clean) makeClean; exit 0;;
esac

buildDebug () {
printf "\n" &&
printf "${YEL}Building htop (Debug Version)...\n" &&
printf "${CYA}\n" &&

# Build debug htop
export NINJA_SUMMARIZE_BUILD=1 &&

./autogen.sh &&

./configure --enable-sensors --enable-hwloc --with-os-release --enable-debug &&

make VERBOSE=1 V=1 &&

printf "\n" &&
printf "${GRE}${bold}Debug Build Completed. ${YEL}${bold}You can now run \`sudo make install\` or \`make install\` to install it.\n" &&
printf "\n" &&
tput sgr0
}
case $1 in
	--debug) buildDebug; exit 0;;
esac

buildSSE41 () {
printf "\n" &&
printf "${YEL}Building htop (SSE4.1 Version)...\n" &&
printf "${CYA}\n" &&

# Build htop
export NINJA_SUMMARIZE_BUILD=1 &&

export CFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -msse4.1 -flto=auto" &&
export CXXFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -msse4.1 -flto=auto" &&
export CPPFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -msse4.1 -flto=auto" &&
export HWLOC_CFLAGS="-g0 -s -O3 -msse4.1 -flto=auto" &&
export LIBNL3GENL_CFLAGS="-g0 -s -O3 -msse4.1 -flto=auto" &&
export LIBNL3_CFLAGS="-g0 -s -O3 -msse4.1 -flto=auto" &&
export LDFLAGS="-Wl,-O3 -msse4.1 -s -flto=auto" &&

./autogen.sh &&

./configure --enable-sensors --enable-hwloc --with-os-release --disable-debug &&

make VERBOSE=1 V=1 &&

printf "\n" &&
printf "${GRE}${bold}Build Completed. ${YEL}${bold}You can now run \`sudo make install\` or \`make install\` to install it.\n" &&
printf "\n" &&
tput sgr0
}
case $1 in
	--sse4) buildSSE41; exit 0;;
esac

printf "\n" &&
printf "${YEL}Building htop...\n" &&
printf "${CYA}\n" &&

# Build htop
export NINJA_SUMMARIZE_BUILD=1 &&

export CFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -mavx -maes -flto=auto" &&
export CXXFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -mavx -maes -flto=auto" &&
export CPPFLAGS="-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DNDEBUG -g0 -s -O3 -mavx -maes -flto=auto" &&
export HWLOC_CFLAGS="-g0 -s -O3 -mavx -maes -flto=auto" &&
export LIBNL3GENL_CFLAGS="-g0 -s -O3 -mavx -maes -flto=auto" &&
export LIBNL3_CFLAGS="-g0 -s -O3 -mavx -maes -flto=auto" &&
export LDFLAGS="-Wl,-O3 -mavx -maes -s -flto=auto" &&

./autogen.sh &&

./configure --enable-sensors --enable-hwloc --with-os-release --disable-debug &&

make VERBOSE=1 V=1 &&

printf "\n" &&
printf "${GRE}${bold}Build Completed. ${YEL}${bold}You can now run \`sudo make install\` or \`make install\` to install it.\n" &&
printf "\n" &&
tput sgr0 &&

exit 0
