#!/bin/sh

set -eu

target_os=${1:?usage: build-release.sh <linux|darwin> <x64|arm64> <output>}
target_arch=${2:?usage: build-release.sh <linux|darwin> <x64|arm64> <output>}
output=${3:?usage: build-release.sh <linux|darwin> <x64|arm64> <output>}

root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
binary_name="tweeta-${target_os}-${target_arch}"

case "${target_os}:${target_arch}" in
  linux:x64|linux:arm64|darwin:x64|darwin:arm64)
    ;;
  *)
    echo "unsupported target: ${target_os}/${target_arch}" >&2
    exit 1
    ;;
esac

mkdir -p "$(dirname "$output")"

cd "$root_dir"
rm -f tweeta

libcurl_cflags=${LIBCURL_CFLAGS:-$(pkg-config --cflags libcurl)}

case "$target_os" in
  linux)
    libcurl_libs=${LIBCURL_LIBS:-$(pkg-config --static --libs libcurl)}

    ./configure \
      CC="${CC:-cc}" \
      PKG_CONFIG="${PKG_CONFIG:-pkg-config}" \
      CPPFLAGS="${CPPFLAGS-}" \
      CFLAGS="${CFLAGS:--std=c11 -Wall -Wextra -Wpedantic -O2}" \
      LDFLAGS="${LDFLAGS:-} -static" \
      LIBCURL_CFLAGS="$libcurl_cflags" \
      LIBCURL_LIBS="$libcurl_libs"

    make tweeta

    file tweeta | grep -F "statically linked" >/dev/null
    ;;
  darwin)
    libcurl_libs=${LIBCURL_LIBS:-$(pkg-config --static --libs libcurl)}

    ./configure \
      CC="${CC:-cc}" \
      PKG_CONFIG="${PKG_CONFIG:-pkg-config}" \
      CPPFLAGS="${CPPFLAGS-}" \
      CFLAGS="${CFLAGS:--std=c11 -Wall -Wextra -Wpedantic -O2}" \
      LDFLAGS="${LDFLAGS:-}" \
      LIBCURL_CFLAGS="$libcurl_cflags" \
      LIBCURL_LIBS="$libcurl_libs"

    make tweeta

    if otool -L tweeta | tail -n +2 | awk '{print $1}' | grep -E '^(/opt/homebrew|/usr/local|/opt/local)' >/dev/null; then
      echo "unexpected non-system dynamic dependency in Darwin build" >&2
      otool -L tweeta >&2
      exit 1
    fi
    ;;
esac

mv tweeta "$output"
chmod 0755 "$output"
echo "built ${binary_name} -> ${output}"
