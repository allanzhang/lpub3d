# Changelog

## [v2.6.0] - 2026-09-11

### 新增
- **界面完整汉化（简体中文）**：应用自身界面与 Qt 框架标准对话框全部中文。7,379 条可译条目 100% 完成，覆盖主菜单、工具栏、状态栏、对话框、右键菜单以及全部工具提示 / WhatsThis 帮助正文；语言跟随系统，可用环境变量 `LPUB3D_LANGUAGE=zh_CN` 强制覆盖
- 启动画面副标题改为「LDRAW 拼搭说明书」，与汉化配套

### 修复
- **打开菜单后闪退（EXC_BAD_ACCESS / SIGSEGV，AppKit 菜单跟踪递归爆栈）**：打包后的应用同时加载了两份 Qt —— 应用包内一份、构建机 Homebrew 一份，重复的 Objective-C 类使 AppKit 在菜单事件循环中无限递归。根因是 `macdeployqt` 单次执行未部署插件依赖（`libqpdf`→QtPdf、`libqsvgicon`→QtSvg、虚拟键盘插件），且主程序仍保留 `-rpath /opt/homebrew/lib`，dyld 因此回退到 Homebrew Qt 并把整个 QtCore/QtGui/QtNetwork/QtDBus 再拉一份。现已重新部署并清除全部外部 rpath，新增 `builds/macx/verify_bundle.py` 作为发布门禁（自包含检查不通过即拒绝打包）
- 汉化回归：`excludedParts.lst` 的正则标记行被译成全角冒号，导致自定义正则回读失败并静默回退内置默认值。已改为按源文跨 context 排除，`apply` 现在会强制清除违反排除规则的残留译文，并新增对应护栏
- 发布脚本 `builds/macx/CreateDmg.sh` 仍按 `LPub3D.app` 打包（改名为 myLPub3D 后不可用）；现统一为 `APP_NAME`/`APP_BUNDLE`/`APP_EXE` 变量，并补入 `qt_zh_CN.qm` / `qtbase_zh_CN.qm`（`macdeployqt` 不携带框架翻译，缺失时发布版标准对话框仍为英文）

### 变更
- 移除等待动画依赖（`waitingspinner` 库及其全部调用点），该功能下线

### 下载
- macOS（Apple Silicon）：myLPub3D-v2.6.0-macOS.zip

## [v2.5.1] - 2026-09-09

### 新增
- 全新 myLPub3D macOS App 图标（圆角底板 + 投影，16～1024 全尺寸图标集）

### 修复
- macOS 构建未应用 App 图标：非 `CONFIG+=dmg` 构建下 `ICON` 为空，导致 Finder/Dock 显示通用图标；现已在 macx 构建中显式指定图标

### 下载
- 未发布（图标版本，工作并入 v2.6.0）

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
