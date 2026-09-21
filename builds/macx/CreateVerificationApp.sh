#!/bin/bash
#
# Create a self-contained macOS verification app from the local build.
# Usage:
#   builds/macx/CreateVerificationApp.sh [source-app] [destination-app] [--launch]
#
# The script is intentionally separate from CreateDmg.sh. It does not build,
# download assets, create a DMG, or require CI variables. It only packages the
# already-built app so it can be double-clicked without DYLD_FRAMEWORK_PATH or
# QT_PLUGIN_PATH.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

SOURCE_APP="${1:-${ROOT_DIR}/mainApp/64bit_release/myLPub3D.app}"
DEST_APP="${2:-${ROOT_DIR}/release/myLPub3D-verification.app}"
LAUNCH="${3:-}"

VERIFY_SCRIPT="${ROOT_DIR}/builds/macx/verify_bundle.py"
APP_EXE_NAME="myLPub3D"

if [[ ! -d "${SOURCE_APP}/Contents/MacOS" ]]; then
  echo "ERROR: source app not found: ${SOURCE_APP}" >&2
  exit 1
fi

if [[ ! -f "${SOURCE_APP}/Contents/MacOS/${APP_EXE_NAME}" ]]; then
  echo "ERROR: executable not found: ${SOURCE_APP}/Contents/MacOS/${APP_EXE_NAME}" >&2
  exit 1
fi

if [[ ! -f "${VERIFY_SCRIPT}" ]]; then
  echo "ERROR: bundle verifier not found: ${VERIFY_SCRIPT}" >&2
  exit 1
fi

QMAKE_BIN="${QMAKE_BIN:-$(command -v qmake || true)}"
if [[ -z "${QMAKE_BIN}" ]]; then
  echo "ERROR: qmake was not found in PATH." >&2
  exit 1
fi

QT_BIN_DIR="$("${QMAKE_BIN}" -query QT_INSTALL_BINS)"
MACDEPLOYQT="${MACDEPLOYQT:-${QT_BIN_DIR}/macdeployqt}"

if [[ ! -x "${MACDEPLOYQT}" ]]; then
  echo "ERROR: macdeployqt not found or not executable: ${MACDEPLOYQT}" >&2
  exit 1
fi

echo "Source app : ${SOURCE_APP}"
echo "Output app : ${DEST_APP}"
echo "macdeployqt: ${MACDEPLOYQT}"

# Build in a clean destination. This prevents an old packaged bundle from
# contributing stale Frameworks and PlugIns to the new executable.
rm -rf "${DEST_APP}"
mkdir -p "$(dirname "${DEST_APP}")"
/usr/bin/ditto "${SOURCE_APP}" "${DEST_APP}"
rm -f "${DEST_APP}/Contents/MacOS/log.out"

DEPLOY_ARGS=("${DEST_APP}" -verbose=1 "-executable=${DEST_APP}/Contents/MacOS/${APP_EXE_NAME}" -always-overwrite)

echo "Deploying Qt dependencies (pass 1)..."
"${MACDEPLOYQT}" "${DEPLOY_ARGS[@]}"

echo "Checking bundle (pass 1)..."
if ! python3 "${VERIFY_SCRIPT}" "${DEST_APP}"; then
  echo "Bundle is not self-contained; re-running macdeployqt..."
  "${MACDEPLOYQT}" "${DEPLOY_ARGS[@]}"
  echo "Stripping foreign rpaths and re-checking..."
  python3 "${VERIFY_SCRIPT}" --fix "${DEST_APP}"
fi

echo "Verifying bundle is self-contained..."
python3 "${VERIFY_SCRIPT}" "${DEST_APP}"

echo "Applying ad-hoc signature..."
/usr/bin/codesign --force --deep --sign - "${DEST_APP}"
/usr/bin/codesign --verify --deep --verbose "${DEST_APP}"

echo "Verification app is ready:"
echo "  ${DEST_APP}"

if [[ "${LAUNCH}" == "--launch" ]]; then
  echo "Launching without Qt environment overrides..."
  /usr/bin/open "${DEST_APP}"
fi
