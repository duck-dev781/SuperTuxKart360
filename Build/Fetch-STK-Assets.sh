#!/usr/bin/env bash
set -euo pipefail

# Fetch the official SuperTuxKart source and runtime data side by side.
# Override STK_SOURCE_REF and STK_ASSETS_REV to reproduce a known pair.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXTERNAL="${ROOT}/External"
SOURCE_DIR="${EXTERNAL}/stk-code"
ASSETS_DIR="${EXTERNAL}/stk-assets"
SOURCE_REF="${STK_SOURCE_REF:-b2a9f223326422aa2dd4eea28319b0c5115fde03}"
ASSETS_REV="${STK_ASSETS_REV:-HEAD}"
SOURCE_URL="https://github.com/supertuxkart/stk-code.git"
ASSETS_URL="https://svn.code.sf.net/p/supertuxkart/code/stk-assets"

command -v git >/dev/null || { echo "git is required" >&2; exit 1; }
command -v svn >/dev/null || { echo "svn (Subversion) is required" >&2; exit 1; }

mkdir -p "${EXTERNAL}"

if [[ ! -d "${SOURCE_DIR}/.git" ]]; then
  git clone "${SOURCE_URL}" "${SOURCE_DIR}"
fi
git -C "${SOURCE_DIR}" fetch --tags origin
git -C "${SOURCE_DIR}" checkout --detach "${SOURCE_REF}"

if [[ ! -d "${ASSETS_DIR}/.svn" ]]; then
  svn checkout -r "${ASSETS_REV}" "${ASSETS_URL}" "${ASSETS_DIR}"
else
  svn update -r "${ASSETS_REV}" "${ASSETS_DIR}"
fi

# Verify the major runtime asset groups used by STK.
for path in karts tracks textures library music sfx; do
  if [[ ! -d "${ASSETS_DIR}/${path}" ]]; then
    echo "Required asset directory missing: ${ASSETS_DIR}/${path}" >&2
    exit 2
  fi
done

SOURCE_SHA="$(git -C "${SOURCE_DIR}" rev-parse HEAD)"
ASSETS_REV_ACTUAL="$(svn info --show-item revision "${ASSETS_DIR}")"
{
  echo "STK_SOURCE_URL=${SOURCE_URL}"
  echo "STK_SOURCE_SHA=${SOURCE_SHA}"
  echo "STK_ASSETS_URL=${ASSETS_URL}"
  echo "STK_ASSETS_REVISION=${ASSETS_REV_ACTUAL}"
} | tee "${EXTERNAL}/stk-upstream-lock.txt"

echo
echo "Official STK source and runtime assets are ready:"
echo "  Source: ${SOURCE_DIR} (${SOURCE_SHA})"
echo "  Assets: ${ASSETS_DIR} (SVN revision ${ASSETS_REV_ACTUAL})"
echo "The two directories are siblings, as required by the upstream build layout."
