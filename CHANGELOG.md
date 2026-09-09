# Changelog

## [v2.5.1] - 2026-09-09

### 新增
- 全新 myLPub3D macOS App 图标（圆角底板 + 投影，16～1024 全尺寸图标集）

### 修复
- macOS 构建未应用 App 图标：非 `CONFIG+=dmg` 构建下 `ICON` 为空，导致 Finder/Dock 显示通用图标；现已在 macx 构建中显式指定图标

### 下载
- 本地构建，未发布到 GitHub

## [v2.5.0] - 2026-09-07

myLPub3D 首个统一版本。以下全部定制修改自上游 LPub3D fork，合并为单一 V2.5.0。

### 新增
- 产品重命名 myLPub3D，隔离 DoubleEagle 应用身份（org/bundle id）
- CSI 标注支持 ARROW / STEP_BADGE / INSERT ARROW / INSERT PICTURE，含完整 PLACEMENT 定位（PAGE_HEADER/FOOTER relativeTo），标注烘焙进渲染图
- STEP_BADGE 圆角矩形改为正圆形徽标

### 修复
- 渲染清晰度：PLI 零件清单与 CSI 装配图边线宽度统一跟随全局设置（LineWidth=3），移除高亮步骤对渲染线宽的覆盖（此前 PLI 恒为 1）
- 深色零件描边方案定稿：乌蓝/军绿/深棕/橄榄绿白色描边，其余零件纯黑描边；禁用 GL_LINE_SMOOTH 与出图 MSAA，放大不糊、锐利硬边
- CSI 标注 ARROW/STEP_BADGE 渲染不生效修复；ARROW 保证锚点位于零件边缘时仍有可见指向线
- PDF/导出文档在 macOS/Linux 打开失败修复（改用 QDesktopServices 打开本地文件）
- 导出页面范围误报修复：DialogExportPages 解析对齐 processPageRange，支持 "1 of 2" 格式
- Qt 6.11 编译错误修复（QString::arg(QAtomicInt)）与后台线程弹窗崩溃修复
- fade/highlight 不稳定渲染修复：step 为空时保持 LPUB_FADE/LPUB_HIGHLIGHT 装配 meta

### 下载
- macOS（Apple Silicon）：myLPub3D-v2.5.0-macOS.zip
