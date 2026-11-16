#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXTERNAL="${ROOT}/External"
XSDK_DIR="${EXTERNAL}/xsdk"
GLSLANG_DIR="${EXTERNAL}/glslang-xenon"
XSDK_SHA="b08bb27350b67518f986bc60e72f7e4e6ebe6094"
GLSLANG_SHA="d2f00185cf5feaa449f6e15c1894fb232b2b6bcd"
PREFIX="${XSDK_PREFIX:-${HOME}/xsdk-gcc}"
WORK="${XSDK_WORK:-${HOME}/xsdk-gcc-work}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN)}"
TOOLCHAIN="${PREFIX}/bin/powerpc-xsdk-elf-"

for command in git cmake ninja python3 make; do
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

echo "== Build pinned Xbox 360 cross-toolchain =="
PREFIX="${PREFIX}" WORK="${WORK}" JOBS="${JOBS}" \
    bash "${XSDK_DIR}/tools/toolchain/build.sh"

echo "== Build the pinned shader compiler =="
python3 "${GLSLANG_DIR}/update_glslang_sources.py"
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
    "${OUTPUT}/RedrawRendererTest.xex"
cat > "${OUTPUT}/BUILD-INFO.txt" <<INFO
Project: SuperTuxKart360 Redraw renderer bring-up
XEX: RedrawRendererTest.xex
xsdk commit: ${XSDK_SHA}
glslang-xenon commit: ${GLSLANG_SHA}
Target: Xbox 360 XEX2 (unsigned homebrew; xsdk RGH/JTAG environment)
Scope: renderer/input smoke test only; not the STK game
INFO

echo "Xbox 360 Redraw renderer XEX:"
echo "  ${OUTPUT}/RedrawRendererTest.xex"
