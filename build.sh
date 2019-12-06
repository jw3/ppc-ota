#!/usr/bin/env bash

readonly sourcedir="${SOURCE_DIR:-${PWD}}"
readonly builddir="${BUILD_DIR:-${sourcedir}/build}"

readonly conanpath="${CONAN_PATH:-${sourcedir}}"
readonly conanuser="${CONAN_USER:-jw3}"
readonly conanchannel="${CONAN_CHANNEL:-stable}"

readonly cross_compiler_root=${CROSS_COMPILER_ROOT:-/usr/local/gcc-arm}
readonly compiler_major_version=$("${cross_compiler_root}/bin/arm-none-eabi-gcc" -dumpspecs | grep *version -A1 | tail -n1 | cut -d. -f1)

export FW_SRC_DIR="."
conan export-pkg . "$conanuser/$conanchannel" -s compiler.version="$compiler_major_version" -sf src -f
