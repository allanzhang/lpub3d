#!/bin/bash
#
# bundle_povray_deps.sh - 为 myLPub3D (macOS/arm64) 组装开箱即用的三方渲染器包。
#
# 为什么需要它：
#   LPub3D 的 POV-Ray 渲染器就是 'lpub3d_trace_cui'（LPub3D-Trace，POV-Ray 3.8 的改名定制版，
#   见其 docs/CUI_README）。官方 macOS arm64 二进制是按 Homebrew 的 openexr 3.3.x / imath 3.1.x
#   编译的。当本机 Homebrew 升级到 openexr 3.4 / imath 3.2 后，OpenEXR 的内联命名空间由
#   Imf_3_3 变为 Imf_3_4——这是真正的 C++ ABI 断裂（二进制里有 26 个 Imf_3_3::* 未定义符号），
#   单改加载路径救不了。因此这里把二进制编译时链接的那一版库取回随包携带，并用
#   @executable_path / @loader_path 引用，使其不再依赖 /opt/homebrew 的 openexr/imath。
#
# 用法：
#   bundle_povray_deps.sh --source <官方 3rdParty 目录> --target <myLPub3D 的 Contents/3rdParty>
#
#   --source  官方 LPub3D.app/Contents/3rdParty（提供 lpub3d_trace_cui-3.8/{bin,resources,docs}、
#             ldglite-1.3、ldview-4.5）。由官方 dmg 解包而来。
#   --target  要填充的 myLPub3D.app/Contents/3rdParty。
#
# 依赖：curl、tar、install_name_tool、codesign（Xcode CLT）、本机 Homebrew 的 libdeflate。
#
# 重要：本脚本必须在 macdeployqt 之后运行。
#   macdeployqt 会递归扫描 bundle 内的 Mach-O 并为 /opt/homebrew 引用做部署。它会把
#   openexr 依赖的 libdeflate 拷进 Contents/Frameworks/、改写引用，却**不给拷贝过来的
#   Homebrew 库补签名**（bottle 本身未签名），于是该库签名无效；arm64 内核会直接
#   SIGKILL 掉加载它的进程（表现：rc=137、零输出、无任何报错）。
#   因此这里把 libdeflate 一并收进我们自己的 lib/ 并改用 @loader_path 引用，
#   macdeployqt 就再也看不到需要它插手的路径。

set -euo pipefail

# 官方二进制链接的库版本。必须与二进制一致，否则会重新踩 ABI 断裂。
VER_OPENEXR_BOTTLE=3.3.1
VER_IMATH_BOTTLE=3.1.12
BOTTLE_PLATFORM=arm64_sonoma
CACHE_DIR="${HOME}/Library/Caches/myLPub3D/3rdparty-deps"

SOURCE_DIR=""
TARGET_DIR=""
while [ $# -gt 0 ]; do
  case "$1" in
    --source) SOURCE_DIR="$2"; shift 2;;
    --target) TARGET_DIR="$2"; shift 2;;
    -h|--help) sed -n '2,22p' "$0"; exit 0;;
    *) echo "未知参数: $1" >&2; exit 2;;
  esac
done
[ -n "$SOURCE_DIR" ] && [ -n "$TARGET_DIR" ] || { echo "用法: $0 --source <dir> --target <dir>" >&2; exit 2; }
[ -d "$SOURCE_DIR" ] || { echo "source 不存在: $SOURCE_DIR" >&2; exit 1; }

# 二进制链接的 6 个库（文件名必须与二进制记录的完全一致）
OEXR_FILES="libOpenEXR-3_3.32.dylib libOpenEXRUtil-3_3.32.dylib libOpenEXRCore-3_3.32.dylib libIex-3_3.32.dylib libIlmThread-3_3.32.dylib"
IMATH_FILE="libImath-3_1.29.dylib"
LIBDEFLATE_FILE="libdeflate.0.dylib"
DYLIB_FILES="$OEXR_FILES $IMATH_FILE $LIBDEFLATE_FILE"
LIBDEFLATE="$(brew --prefix libdeflate 2>/dev/null || echo /opt/homebrew/opt/libdeflate)/lib/$LIBDEFLATE_FILE"
[ -e "$LIBDEFLATE" ] || { echo "缺少 libdeflate: $LIBDEFLATE" >&2; exit 1; }

# ---- 1. 从 ghcr.io 取 Homebrew 旧版 bottle（index -> manifest -> blob）-------------
ghcr_bottle() { # <formula> <version> <platform> <out.tar.gz>
  local formula="$1" ver="$2" plat="$3" out="$4"
  [ -f "$out" ] && { echo "  已缓存 $(basename "$out")"; return 0; }
  local tok idx mdig bdig
  tok=$(curl -sf --max-time 30 "https://ghcr.io/token?scope=repository:homebrew/core/$formula:pull&service=ghcr.io" \
        | sed -n 's/.*"token":"\([^"]*\)".*/\1/p')
  curl -sf --max-time 60 -H "Authorization: Bearer $tok" -H 'Accept: application/vnd.oci.image.index.v1+json' \
       "https://ghcr.io/v2/homebrew/core/$formula/manifests/$ver" -o "$CACHE_DIR/$formula.idx.json" \
    || { echo "  取 $formula $ver 清单失败（ghcr 可能已清理该版本）" >&2; return 1; }
  mdig=$(python3 - "$CACHE_DIR/$formula.idx.json" "$ver.$plat" <<'PY'
import json,sys
d=json.load(open(sys.argv[1]))
for m in d.get("manifests",[]):
    if m.get("annotations",{}).get("org.opencontainers.image.ref.name","")==sys.argv[2]:
        print(m["digest"]); break
PY
)
  [ -n "$mdig" ] || { echo "  未找到 $formula $ver 的 $plat bottle" >&2; return 1; }
  curl -sf --max-time 60 -H "Authorization: Bearer $tok" -H 'Accept: application/vnd.oci.image.manifest.v1+json' \
       "https://ghcr.io/v2/homebrew/core/$formula/manifests/$mdig" -o "$CACHE_DIR/$formula.mf.json"
  bdig=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['layers'][0]['digest'])" "$CACHE_DIR/$formula.mf.json")
  echo "  下载 $formula $ver ($plat)"
  curl -sfL --max-time 300 -H "Authorization: Bearer $tok" \
       "https://ghcr.io/v2/homebrew/core/$formula/blobs/$bdig" -o "$out"
}

mkdir -p "$CACHE_DIR"
echo "== 1/5 取旧版依赖库 (openexr $VER_OPENEXR_BOTTLE / imath $VER_IMATH_BOTTLE)"
ghcr_bottle openexr "$VER_OPENEXR_BOTTLE" "$BOTTLE_PLATFORM" "$CACHE_DIR/openexr.tar.gz"
ghcr_bottle imath   "$VER_IMATH_BOTTLE"   "$BOTTLE_PLATFORM" "$CACHE_DIR/imath.tar.gz"

# ---- 2. 摊开并取出那 6 个真实 dylib（bottle 里是符号链接，需 -L 解引用）------------
echo "== 2/5 解出依赖库"
STAGE="$CACHE_DIR/stage"; LIBDIR="$CACHE_DIR/lib"
rm -rf "$STAGE" "$LIBDIR"; mkdir -p "$STAGE" "$LIBDIR"
tar xzf "$CACHE_DIR/openexr.tar.gz" -C "$STAGE"
tar xzf "$CACHE_DIR/imath.tar.gz"   -C "$STAGE"
for f in $OEXR_FILES; do
  cp -L "$STAGE/openexr/$VER_OPENEXR_BOTTLE/lib/$f" "$LIBDIR/"
done
cp -L "$STAGE/imath/$VER_IMATH_BOTTLE/lib/$IMATH_FILE" "$LIBDIR/"
cp -L "$LIBDEFLATE" "$LIBDIR/$LIBDEFLATE_FILE"
chmod u+w "$LIBDIR"/*.dylib

# ---- 3. 重写加载路径：库之间用 @loader_path，可执行文件用 @executable_path/../lib ----
# bottle 未安装过，记的是占位符 @@HOMEBREW_PREFIX@@/opt/...；库之间还有 @rpath/ 形式。
# 三种形式统一改成 @loader_path，彻底不依赖 rpath 搜索顺序。
echo "== 3/5 重写依赖引用"
for f in $DYLIB_FILES; do
  lib="$LIBDIR/$f"
  for g in $OEXR_FILES; do
    install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/openexr/lib/$g" "@loader_path/$g" "$lib" 2>/dev/null || true
    install_name_tool -change "@rpath/$g"                            "@loader_path/$g" "$lib" 2>/dev/null || true
  done
  install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/imath/lib/$IMATH_FILE" "@loader_path/$IMATH_FILE" "$lib" 2>/dev/null || true
  install_name_tool -change "@rpath/$IMATH_FILE"                            "@loader_path/$IMATH_FILE" "$lib" 2>/dev/null || true
  install_name_tool -id "@loader_path/$f" "$lib"
done
# libdeflate：bottle 里的占位符 Homebrew 安装时才改写，此处必须手动接管。收进我们自己的
# lib/ 而不是指向 /opt/homebrew，否则 macdeployqt 会拷一份未签名的到 Frameworks 里（见文件头）。
for old in "@@HOMEBREW_PREFIX@@/opt/libdeflate/lib/$LIBDEFLATE_FILE" \
           "$LIBDEFLATE" \
           "@loader_path/../../../Frameworks/$LIBDEFLATE_FILE"; do
  install_name_tool -change "$old" "@loader_path/$LIBDEFLATE_FILE" \
                    "$LIBDIR/libOpenEXRCore-3_3.32.dylib" 2>/dev/null || true
done

# ---- 4. 组装目标包 --------------------------------------------------------------
echo "== 4/5 组装 $TARGET_DIR"
TC="$TARGET_DIR/lpub3d_trace_cui-3.8"
rm -rf "$TC" "$TARGET_DIR/ldglite-1.3" "$TARGET_DIR/ldview-4.6"
mkdir -p "$TC/bin" "$TC/lib"
cp "$SOURCE_DIR/lpub3d_trace_cui-3.8/bin/lpub3d_trace_cui" "$TC/bin/"
cp "$LIBDIR"/*.dylib "$TC/lib/"
cp -R "$SOURCE_DIR/lpub3d_trace_cui-3.8/resources" "$TC/"
[ -d "$SOURCE_DIR/lpub3d_trace_cui-3.8/docs" ] && cp -R "$SOURCE_DIR/lpub3d_trace_cui-3.8/docs" "$TC/"

# 可执行文件：官方记的是已改写的绝对路径（与库的占位符形式不同，必须分别处理）
BIN="$TC/bin/lpub3d_trace_cui"
for f in $OEXR_FILES; do
  install_name_tool -change "/opt/homebrew/opt/openexr/lib/$f" "@executable_path/../lib/$f" "$BIN" 2>/dev/null || true
done
install_name_tool -change "/opt/homebrew/opt/imath/lib/$IMATH_FILE" "@executable_path/../lib/$IMATH_FILE" "$BIN" 2>/dev/null || true
chmod +x "$BIN"

# ldglite：官方布局与代码期望一致
cp -R "$SOURCE_DIR/ldglite-1.3" "$TARGET_DIR/"
chmod +x "$TARGET_DIR/ldglite-1.3/bin/ldglite"
# ldview：代码在 macOS 上找 ldview-4.6/bin/LDView；官方包给的是 ldview-4.5/bin/ldview。
# 只做重命名（零代码改动）。注：官方 4.5 二进制以 4.6 目录名存放，版本号差异待验证。
cp -R "$SOURCE_DIR/ldview-4.5" "$TARGET_DIR/ldview-4.6"
mv "$TARGET_DIR/ldview-4.6/bin/ldview" "$TARGET_DIR/ldview-4.6/bin/LDView"
chmod +x "$TARGET_DIR/ldview-4.6/bin/LDView"

# ---- 5. arm64 要求签名有效：改过的每个文件都要 ad-hoc 重签 -------------------------
echo "== 5/5 重签"
for f in "$TC"/lib/*.dylib "$BIN" "$TARGET_DIR/ldglite-1.3/bin/ldglite" "$TARGET_DIR/ldview-4.6/bin/LDView"; do
  codesign --force --sign - "$f" >/dev/null 2>&1 || echo "  签名失败: $f" >&2
done

# ---- 自检：可执行文件必须能起来，且不再引用 /opt/homebrew 的 openexr/imath ----------
echo "== 自检"
ver_out=$("$BIN" -version 2>&1 || true)
if printf '%s' "$ver_out" | grep -q 'Ray Tracer Version'; then
  echo "  OK  $(printf '%s' "$ver_out" | grep 'Ray Tracer Version' | sed 's/^ *//')"
else
  echo "  失败：$BIN 无法启动" >&2; printf '%s\n' "$ver_out" >&2; exit 1
fi
left=$(otool -L "$BIN" "$TC"/lib/*.dylib | grep -cE '/opt/homebrew/(opt|Cellar)/(openexr|imath|libdeflate)' || true)
[ "$left" = "0" ] && echo "  OK  已无 openexr/imath/libdeflate 的 Homebrew 引用" \
                  || { echo "  仍有 $left 处 openexr/imath/libdeflate 引用" >&2; exit 1; }
echo "完成。渲染器仍在以下位置依赖本机 Homebrew（上游官方包亦然）：sdl2 / libx11 / X11 / libtiff / jpeg-turbo / libpng / boost"
