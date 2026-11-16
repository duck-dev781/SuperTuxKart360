#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXTERNAL="${ROOT}/External"
XSDK_DIR="${EXTERNAL}/xsdk"
GLSLANG_DIR="${EXTERNAL}/glslang-xenon"
XSDK_SHA="b08bb27350b67518f986bc60e72f7e4e6ebe6094"
GLSLANG_SHA="d2f00185cf5feaa449f6e15c1894fb232b2b6bcd"
STK_SOURCE_SHA="b2a9f223326422aa2dd4eea28319b0c5115fde03"
STK_ASSET_REVISION="18627"
STK_ASSET_BASE="https://svn.code.sf.net/p/supertuxkart/code/stk-assets"
PREFIX="${XSDK_PREFIX:-${HOME}/xsdk-gcc}"
WORK="${XSDK_WORK:-${HOME}/xsdk-gcc-work}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN)}"
TOOLCHAIN="${PREFIX}/bin/powerpc-xsdk-elf-"

for command in git cmake ninja python3 make svn; do
    command -v "${command}" >/dev/null || {
        echo "Missing required build tool: ${command}" >&2
        exit 1
    }
done

mkdir -p "${EXTERNAL}"

if [[ ! -d "${XSDK_DIR}/.git" ]]; then
    git clone https://github.com/xenosworks/xsdk.git "${XSDK_DIR}"
fi
git -C "${XSDK_DIR}" fetch origin "${XSDK_SHA}"
git -C "${XSDK_DIR}" checkout --detach "${XSDK_SHA}"
test "$(git -C "${XSDK_DIR}" rev-parse HEAD)" = "${XSDK_SHA}"

if [[ ! -d "${GLSLANG_DIR}/.git" ]]; then
    git clone https://github.com/xenosworks/glslang-xenon.git "${GLSLANG_DIR}"
fi
git -C "${GLSLANG_DIR}" fetch origin "${GLSLANG_SHA}"
git -C "${GLSLANG_DIR}" checkout --detach "${GLSLANG_SHA}"
test "$(git -C "${GLSLANG_DIR}" rev-parse HEAD)" = "${GLSLANG_SHA}"

echo "== Configure resilient, checksum-verified toolchain downloads =="
python3 - "${XSDK_DIR}/tools/toolchain/build.sh" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
source = path.read_text()
needle = '\t\t[ -f "$archive" ] || curl -sSfL -o "$archive" "$url"'
replacement = r'''        if [ ! -f "$archive" ]; then
            case "$archive" in
                binutils-*.tar.xz)
                    mirrors=("$url"
                        "https://ftpmirror.gnu.org/gnu/binutils/$archive"
                        "https://mirrors.kernel.org/gnu/binutils/$archive")
                    ;;
                gcc-*.tar.xz)
                    version="${archive%.tar.xz}"
                    mirrors=("$url"
                        "https://ftpmirror.gnu.org/gnu/gcc/$version/$archive"
                        "https://mirrors.kernel.org/gnu/gcc/$version/$archive")
                    ;;
                newlib-*.tar.gz)
                    mirrors=("$url")
                    ;;
                *)
                    echo "No download mirrors configured for $archive" >&2
                    return 1
                    ;;
            esac
            downloaded=0
            for mirror in "${mirrors[@]}"; do
                echo "Fetching $archive from $mirror"
                if curl --fail --location --silent --show-error \
                    --connect-timeout 30 --max-time 900 \
                    --retry 3 --retry-all-errors --retry-delay 5 \
                    -o "$archive.part" "$mirror"; then
                    if echo "$sha256  $archive.part" | sha256sum -c --quiet; then
                        mv "$archive.part" "$archive"
                        downloaded=1
                        break
                    fi
                    echo "Checksum mismatch from $mirror; trying next mirror" >&2
                fi
                rm -f "$archive.part"
            done
            if [ "$downloaded" -ne 1 ]; then
                echo "Unable to fetch a verified $archive from configured mirrors" >&2
                return 1
            fi
        fi'''
if needle not in source:
    raise SystemExit("Expected upstream download line not found; refusing to patch")
if "checksum-verified toolchain mirror fallback" not in source:
    source = source.replace(needle, replacement, 1)
    source = source.replace("fetch() {", "# checksum-verified toolchain mirror fallback\nfetch() {", 1)
path.write_text(source)
PY

echo "== Build pinned Xbox 360 cross-toolchain =="
PREFIX="${PREFIX}" WORK="${WORK}" JOBS="${JOBS}" \
    bash "${XSDK_DIR}/tools/toolchain/build.sh"

echo "== Build the pinned shader compiler =="
(cd "${GLSLANG_DIR}" && python3 update_glslang_sources.py)
cmake -S "${GLSLANG_DIR}" -B "${GLSLANG_DIR}/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_OPT=ON \
    -DSPIRV_SKIP_EXECUTABLES=ON \
    -DGLSLANG_TESTS=OFF
cmake --build "${GLSLANG_DIR}/build" --parallel "${JOBS}"
install -m 0755 "${GLSLANG_DIR}/build/StandAlone/glslang" "${PREFIX}/bin/glslang"

echo "== Build xsdk runtime and host shader compiler =="
make -C "${XSDK_DIR}" -j"${JOBS}" TOOLCHAIN="${TOOLCHAIN}"

echo "== Build Redraw's native XEX =="
cmake -S "${ROOT}/Redraw/Xbox360" \
    -B "${XSDK_DIR}/build/redraw-xbox360" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${XSDK_DIR}/cmake/xsdk.cmake" \
    -DXSDK_TOOLCHAIN="${PREFIX}" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "${XSDK_DIR}/build/redraw-xbox360" --parallel "${JOBS}"

OUTPUT="${ROOT}/Build/Xbox360/Artifacts"
mkdir -p "${OUTPUT}"
install -m 0644 "${XSDK_DIR}/build/redraw-xbox360/redraw_xbox360.xex" \
    "${OUTPUT}/SuperTuxKart360.xex"

echo "== Fetch official Adiumy kart and Black Forest track assets =="
rm -rf "${OUTPUT}/assets"
mkdir -p "${OUTPUT}/assets"
svn export --quiet -r "${STK_ASSET_REVISION}" \
    "${STK_ASSET_BASE}/karts/adiumy" "${OUTPUT}/assets/adiumy"
svn export --quiet -r "${STK_ASSET_REVISION}" \
    "${STK_ASSET_BASE}/tracks/black_forest" "${OUTPUT}/assets/black_forest"

cat > "${OUTPUT}/BUILD-INFO.txt" <<INFO
Project: SuperTuxKart360
XEX: SuperTuxKart360.xex
STK source baseline: ${STK_SOURCE_SHA}
STK runtime asset SVN revision: ${STK_ASSET_REVISION}
xsdk commit: ${XSDK_SHA}
glslang-xenon commit: ${GLSLANG_SHA}
Target: Xbox 360 XEX2 (xsdk homebrew target)
Assets: official Adiumy kart and Black Forest track folders
Controls: left stick steers, RT accelerates, LT brakes, Start resets, Back exits
Scope: first playable drive slice using official mesh/track data and prototype arcade movement
Not yet included: STK production physics, texture/material rendering, menus, AI, items, music, sound, and multiplayer
INFO

echo "Xbox 360 SuperTuxKart360 build:"
echo "  ${OUTPUT}/SuperTuxKart360.xex"
echo "  ${OUTPUT}/assets/adiumy/"
echo "  ${OUTPUT}/assets/black_forest/"
