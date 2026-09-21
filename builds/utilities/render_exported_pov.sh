#!/bin/bash
#
# render_exported_pov.sh — 渲染 LPub3D「导出 POVRay 场景文件」得到的 .pov
#
# 用途：myLPub3D 的「File → Export As → PovRay Scene Files...」导出的 .pov 里已经
#       带有 Studio 贴纸（mesh2 + uv_vectors + image_map）。应用自己的页面渲染走的是
#       另一条不含贴图的导出路径，所以要看贴纸就得渲染这些 .pov —— 本脚本做这件事。
#
# 用法:
#   render_exported_pov.sh <pov文件或目录> [输出目录] [宽] [高]
#
# 默认输出目录 = 与输入相同；默认宽高 1240x1753（与应用的 DPI 设置一致）。
# 渲染器用应用内自带的 lpub3d_trace_cui（即 LPub3D 的 POV-Ray）。
#
# 说明：POV-Ray 的 +I/+O/+L 不接受未加引号的空格，而本仓库路径含空格，
#       因此这里把场景与 include/ini 都暂存到无空格的临时目录再渲染。

set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
APP="${MYLPUB3D_APP:-$REPO/mainApp/64bit_release/myLPub3D.app}"
BIN="$APP/Contents/3rdParty/lpub3d_trace_cui-3.8/bin/lpub3d_trace_cui"
RES="$APP/Contents/3rdParty/lpub3d_trace_cui-3.8/resources"

INPUT="${1:-}"
OUTDIR="${2:-}"
W="${3:-1240}"
H="${4:-1753}"

[ -n "$INPUT" ] || { echo "用法: $0 <pov文件或目录> [输出目录] [宽] [高]" >&2; exit 2; }
[ -x "$BIN" ]   || { echo "找不到渲染器: $BIN" >&2; exit 1; }

# 收集待渲染的 .pov（目录则取该目录下的 *.pov，不递归）
POVS=()
if [ -d "$INPUT" ]; then
  while IFS= read -r -d '' f; do POVS+=("$f"); done < <(find "$INPUT" -maxdepth 1 -name '*.pov' -print0)
elif [ -f "$INPUT" ]; then
  POVS=("$INPUT")
else
  echo "输入不存在: $INPUT" >&2; exit 1
fi
[ ${#POVS[@]} -gt 0 ] || { echo "没找到 .pov 文件" >&2; exit 1; }

[ -n "$OUTDIR" ] || OUTDIR="$(dirname "${POVS[0]}")"
mkdir -p "$OUTDIR"

# 暂存到无空格路径（POV-Ray 的 +I/+L 不吃未加引号的空格）
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cp -R "$RES/include" "$WORK/inc"
cp -R "$RES/ini"     "$WORK/ini"

echo "渲染器: $BIN"
echo "输出到: $OUTDIR"
echo

ok=0; fail=0
for pov in "${POVS[@]}"; do
  base="$(basename "$pov" .pov)"
  cp "$pov" "$WORK/scene.pov"
  out="$OUTDIR/$base.png"
  if "$BIN" \
        +I"$WORK/scene.pov" +O"$WORK/out.png" \
        +W"$W" +H"$H" +UA +Q11 +R3 +A0.1 +J0.5 -d \
        +L"$WORK/inc" +L"$WORK/ini" > "$WORK/render.log" 2>&1 \
     && [ -s "$WORK/out.png" ]; then
    cp "$WORK/out.png" "$out"
    printf "  OK   %s\n" "$(basename "$out")"
    ok=$((ok+1))
  else
    printf "  失败 %s\n" "$base"
    tail -3 "$WORK/render.log" | sed 's/^/       /'
    fail=$((fail+1))
  fi
  rm -f "$WORK/out.png"
done

echo
echo "完成: 成功 $ok，失败 $fail"
