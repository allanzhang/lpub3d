# myLPub3D Windows / macOS 兼容性分析（阶段 A）

- 审计日期：2026-09-18
- 当前版本：`v2.7.0` / `048008d69`
- 上游基线：`8dc814e18`（`Cleanup preferences update`）
- 审计范围：定向静态审计，不改代码、不构建、不安装
- 结论状态：待人工验收

## 1. 结论摘要

Windows/macOS 同步目前不能保证，原因不只是缺少条件编译，而是缺少一条完整、已执行的跨平台交付链：

1. 仓库里虽然已有 Windows AMD/ARM64、macOS Intel/Apple Silicon 的构建矩阵，但 fork 的 GitHub Actions 页面显示 `prod_ci_build.yml` 为 **0 次运行**。因此没有证据证明当前版本在 Windows 或 macOS 上被自动构建、测试和打包过。
2. macOS 发布链被 fork 定制过，而 Windows 发布链仍大量沿用上游产品名、仓库地址、更新 URL 和资源命名，两个平台的发布行为已经分叉。
3. 构建检查脚本仍包含旧产品名路径和文件名。即使 Windows 能编译成功，现有检查也可能检查不到正确产物，或无法证明两端功能一致。
4. Qt 框架中文翻译在 macOS 打包脚本中被显式复制，但 Windows 打包脚本没有对应步骤，标准 Qt 对话框的中文化不能视为同步。
5. 现有检查以“应用是否成功运行/退出码是否为 0”为主，没有统一的输入输出金标准、渲染结果比对、文件兼容和安装升级矩阵，因此只能发现明显崩溃，不能保证功能同步。

在明确支持的 Windows/macOS 版本、编译器和依赖矩阵下，可以把目标提高为“同一规格、同一验收用例、可重复验证的行为同步”。不能承诺任意环境下像素级或二进制级完全一致，尤其不能把字体、GPU、驱动和外部渲染器造成的差异当作普通代码问题。

## 2. 当前平台链路地图

### 2.1 构建体系

- 项目是 qmake/subdirs 工程，不是 CMake 工程。
- 根工程：`LPub3D.pro`
- 公共配置：`common.pri`
- 主程序配置：`mainApp/mainApp.pro`
- Windows 分支包含 MSVC 编译标志、Windows 库、资源、安装和打包配置。
- macOS 分支包含 bundle、Homebrew、Framework 和 DMG 配置。
- 该结构本身可以跨平台，但产品重命名后，构建、检查和发布脚本没有统一从单一产品身份源读取名称。

### 2.2 平台专用实现数量

静态检索显示，主程序、公共库、LDView 集成和第三方代码中有大量 `Q_OS_WIN`、`Q_OS_MACOS`、`Q_OS_LINUX`、`_WIN32`、`__APPLE__` 分支。这说明上游已经为平台差异做了大量工作，不能把“跨平台兼容”简化为修复少量宏。

真正需要做的是区分：

- 上游已有的平台适配：路径、窗口、进程、OpenGL、安装和崩溃处理。
- fork 新增的共享业务/UI/渲染代码：需要在两端共同验证。
- fork 新增的平台发布代码：目前主要针对 macOS，Windows 未同步。

## 3. 关键发现

### P0-1：跨平台 CI 没有实际执行

证据：

- `.github/workflows/prod_ci_build.yml` 定义了 Linux、macOS 和 Windows AMD/ARM64 构建矩阵。
- fork 的 Actions 页面显示该 workflow **0 workflow runs**。
- `appveyor.yml` 存在，但部署目标和条件仍指向上游 `trevorsandy/lpub3d`，不能据此证明当前 fork 的 Windows 构建已运行。

影响：

- 当前 Windows 兼容性没有任何自动证据。
- macOS 自定义发布修复也没有在同一条 CI 中与 Windows 共同回归。
- 每次版本更新后，两端是否仍能构建、安装和启动都只能靠人工发现。

### P0-2：产品重命名后，Windows 与 macOS 发布身份不一致

已确认的分叉：

- 应用目标已改为 `myLPub3D`：`mainApp/mainApp.pro:2`
- macOS 发布脚本已支持 `myLPub3D.app` / `myLPub3D` 可执行文件：`builds/macx/CreateDmg.sh`
- Windows 更新 JSON 仍生成 `LPub3D-...exe`、`LPub3D_...zip` 等下载地址：`builds/windows/CreateExePkg.bat:1104` 起
- Windows 更新 URL 基址仍为 `https://github.com/trevorsandy`：`builds/windows/CreateExePkg.bat:332`
- 应用更新宏仍指向 SourceForge 上游更新 JSON：`mainApp/version.h:232-234`
- macOS 更新 JSON 条目也仍包含 `LPub3D-...dmg` 命名，而 fork README 发布的是 `myLPub3D-...-macOS.zip`。

影响：

- Windows 即使构建出 `myLPub3D.exe`，更新元数据仍可能指向不存在的上游资产。
- macOS 的 in-app 更新检查和实际 release asset 命名可能不匹配。
- 两个平台的自动更新、下载页和安装包命名不能保证同步。

### P0-3：检查脚本仍假设旧应用名，验证链本身已失真

macOS：

- `builds/check/build_checks.sh:172` 仍写死 `LPub3D.app/Contents/MacOS/LPub3D`。
- `builds/check/build_checks.sh:316` 仍抓取 `LPub3D.*`。
- `builds/check/build_checks.sh:365` 仍从 `~/Library/Application Support/LPub3D Software` 查找日志。
- `builds/utilities/ci/github/macos-build.sh:316` 仍查找 `LPub3D-*.dmg`，但自定义打包脚本生成 `myLPub3D-...dmg`。

Windows：

- `builds/check/build_checks.bat:46-48` 仍查找旧配置路径 `LPub3D Software\LPub3D.ini`。
- 该路径在新身份下预计会变为 `DoubleEagle\myLPub3D` 对应的 Qt 配置路径；当前检查至少会产生错误的配置资产警告，严重时会掩盖真实回归。
- `builds/windows/RunBuildCheck.bat:30` 仍写死 `PACKAGE=LPub3D`；主 Windows 自动构建使用 `builds/check/build_checks.bat` 时会动态读取产品名，但手工检查入口仍不一致。

影响：

- “检查通过”不一定代表当前 `myLPub3D` 包通过。
- 日志、配置和哈希资产可能缺失或来自错误目录。
- 两端不能共享同一套验收语义。

### P1-1：Windows 没有同步部署 Qt 框架中文翻译

证据：

- 应用自身的中文 `.qm` 嵌入资源，应用界面翻译本身是跨平台的。
- macOS 打包脚本显式复制 `qt_zh_CN.qm` 和 `qtbase_zh_CN.qm`：`builds/macx/CreateDmg.sh:453-463`
- Windows 构建/打包脚本没有对应的 `qt_*.qm` 部署步骤。
- 应用在 Windows 上只把 `applicationDirPath()/translations` 加入搜索路径：`mainApp/application.cpp:1422-1424`

影响：

- 主界面可能显示中文，但 Windows 标准的文件选择器、字体/颜色对话框、消息框按钮仍可能显示英文。
- 与 macOS 的“完整中文界面”承诺不一致。

### P1-2：Windows 品牌资源没有同步

证据：

- Windows 资源图标仍为 `lpub3d.ico`：`mainApp/mainApp.pro:56`
- Windows RC 文件仍引用 `lpub3d.ico`：`mainApp/lpub3d.rc:20`
- 仓库没有 `mylpub3d.ico`，只有 macOS 的 `mylpub3d.icns`。
- NSIS 安装器仍使用上游 `setup.ico`。

影响：

- 可执行文件、任务栏、安装器和开始菜单可能仍显示旧 LPub3D 图标。
- UI 视觉同步无法成立，最终需人工验收。

### P1-3：用户配置、注册表和旧版本迁移没有统一设计

应用身份由以下调用决定：

- `mainApp/main.cpp:12-15` 设置 organization domain、organization name、application name。
- 产品名从 `LPub3D` 改为 `myLPub3D`，公司名从 `LPub3D Software` 改为 `DoubleEagle`。

影响：

- Windows 上旧的 `HKCU/HKLM` 注册表配置、`LPub3D Software` 数据目录和新的 `DoubleEagle/myLPub3D` 路径不会自动互通。
- macOS 上旧 bundle ID、旧 Preferences 和旧 Application Support 目录也不会自动迁移。
- 需要在发布前明确：是全新安装、兼容迁移，还是二者都支持。当前无法从代码确认。

### P1-4：Windows 没有 macOS 同等级的自包含/发布门禁

证据：

- macOS 新增 `builds/macx/verify_bundle.py`，检查 bundle 内 Qt、动态库和外部 rpath。
- Windows 没有等价的依赖完整性检查、DLL 搜索路径检查、VC Runtime 检查或“干净机器启动”门禁。
- Windows 脚本主要依赖复制文件、NSIS 和哈希生成，不能证明运行时不会加载构建机路径或缺失 DLL。

影响：

- macOS 已修复“加载构建机 Qt 导致闪退”一类问题，但 Windows 仍可能发生同类 DLL、OpenSSL、插件或 VC Runtime 问题。
- 两端的发布可信度不同。

### P1-5：现有功能检查不足以证明“功能同步”

仓库没有独立单元测试目录，主要只有：

- `builds/check/build_checks.bat`
- `builds/check/build_checks.sh`
- 少量测试模型和图片资源

现有检查主要验证：

- 应用是否能启动和返回成功退出码。
- 若依赖存在，执行若干渲染器路径的烟测。

缺少：

- 同一输入在两端的结构化输出对比。
- 导出 PDF/PNG/HTML 的页数、尺寸、文本、文件结构对比。
- 字体、DPI、系统缩放、中文路径、空格路径、长路径测试。
- 文件关联、双击打开、拖放、最近文件和命令行参数矩阵。
- 安装、卸载、升级、端口版和 per-user/per-machine 安装测试。
- 渲染异常的容差标准和人工视觉验收记录。

### P2-1：Windows 编译可移植性风险点

静态检查发现 fork 新增代码在 `mainApp/application.cpp:352` 使用 `std::stable_sort`，但该文件没有显式包含 `<algorithm>`。

- 在 clang/libc++ 上可能被 Qt 头间接包含而通过。
- 在 MSVC 的标准库实现下不应依赖这种传递包含。
- 这是 Windows 首次实机编译时最值得优先验证的低成本风险项之一。

### P2-2：源 `Info.plist` 仍有旧身份

`mainApp/Info.plist` 仍包含：

- `CFBundleExecutable` / `CFBundleName` 的 `mylpub3d_debug`
- `CFBundleIconFile` 的 `lpub3d.icns`
- 上游 `trevorsandy.github.io` 信息和旧 Git SHA key

`macosfiledistro.pri` 在构建时覆盖部分字段，因此 release 可能能跑，但源文件不是一致的事实源，后续维护容易再次分叉。

### P2-3：文档和发布说明没有完成跨平台同步

- `README.md` 的下载区只列 macOS 版本。
- Windows 构建/安装说明主要仍是上游 README，且包含上游链接。
- 发布说明和应用帮助中大量路径、URL、安装方式仍使用 LPub3D 或上游仓库。

这不会直接导致 Windows 编译失败，但会让“支持 Windows”的交付定义不清晰，用户也无法自行复现。

### P2-4：换行符和属性策略增加维护风险

- `.gitattributes` 将大量 `mainApp` 源文件标记为 `-text`。
- 当前同时存在 LF、CRLF 和混合换行文件。
- Windows 编译本身通常可以处理这些换行，但这会增加补丁、代码审查、脚本执行和跨平台 diff 的不确定性。
- 应在稳定版本节点统一策略，不建议在功能修复中顺带大规模重写换行。

## 4. 能否完全保证两个平台功能同步

不能无条件保证。可以实现并验证以下目标：

> 对定义好的 Windows/macOS 版本、Qt/编译器/第三方依赖矩阵和发布方式，同一版本的软件通过同一套功能规格、自动化检查、安装升级检查和人工视觉验收。

建议把“同步”定义成四层：

1. 源码同步：共享业务逻辑，平台差异集中在适配层。
2. 行为同步：相同操作产生相同的业务结果和错误语义。
3. 文件同步：相同输入在两端生成兼容的文件结构、页数、尺寸和可读取内容。
4. 视觉同步：同一视觉规格，但允许字体、抗锯齿、GPU 驱动造成的非语义差异，并由人工按容差验收。

对于渲染结果，建议使用：

- 结构断言：页数、分辨率、图层/页面数量、关键内容是否存在。
- 数值断言：尺寸、边界、颜色范围、关键像素区域和可接受误差。
- 人工视觉验收：箭头、徽标、引线、裁切、字体和排版最终由人工确认。

## 5. 建议改动顺序

### P0：先让两端“确实跑起来”

1. 在 fork 中启用 GitHub Actions，使用 `workflow_dispatch` 运行完整 `prod_ci_build.yml`，先拿到 Windows AMD、Windows ARM64、macOS Intel、macOS Apple Silicon 的真实结果。
2. 将产品名、应用 ID、公司名、仓库 URL、release asset 前缀统一为单一配置来源，构建脚本不得各自硬编码。
3. 修复 `build_checks.sh`、`macos-build.sh`、`BuildCheck.bat` 中的旧应用名、旧路径和旧文件名。
4. 修正 Windows 更新 JSON 和 in-app 更新源；若暂不支持 Windows 自动更新，必须明确禁用，而不是指向错误资产。
5. 禁用或修正 `appveyor.yml` 的上游部署目标，避免把产物部署到错误仓库。

### P1：让两端交付合格

6. Windows 打包增加 Qt 框架翻译，至少覆盖 `qt_zh_CN.qm`、`qtbase_zh_CN.qm`。
7. 新增 `mylpub3d.ico`，同步可执行文件、NSIS 安装器和文件关联图标。
8. 增加 Windows 依赖自包含检查：DLL、Qt 插件、OpenSSL、VC Runtime、运行路径和延迟加载。
9. 建立配置迁移规则：旧 LPub3D 配置/注册表/Application Support 到 myLPub3D 的迁移或兼容读取。
10. 将构建检查升级为跨平台验收矩阵，而不是只看退出码。

### P2：降低后续维护成本

11. 消除新增代码中的传递头文件依赖，例如 `application.cpp` 显式包含 `<algorithm>`。
12. 整理 `Info.plist`、README、发布说明、About 和更新 URL，使它们引用 fork 的正确身份。
13. 统一换行和 `.gitattributes` 策略，但单独提交，避免污染功能 diff。
14. 给外部渲染器、路径、进程启动、Unicode 和长路径增加专门的回归用例。

## 6. 建议验收矩阵

| 类别 | Windows 必测 | macOS 必测 | 通过判定 |
|---|---|---|---|
| 构建 | MSVC x64、ARM64 | Intel、Apple Silicon | 全矩阵构建成功 |
| 启动 | 干净系统、端口版、安装版 | 干净系统、DMG 安装版 | 无缺失依赖、无构建机路径 |
| 文件打开保存 | LDR/MPD、中文路径、空格、长路径 | 同左 | 内容往返不丢失 |
| 导出 | PDF、PNG、HTML、图片格式 | 同左 | 页数/尺寸/文本结构符合规格 |
| 渲染 | native、LDView、LDGLite、POV-Ray 可用路径 | 同左 | 结构断言通过，差异在容差内 |
| 本地化 | 主界面和 Qt 标准对话框 | 同左 | 中文无漏译、无乱码 |
| UI | DPI、缩放、高对比度、菜单 | Retina、菜单、暗色模式 | 人工视觉验收 |
| 发布 | NSIS、端口版、卸载、升级 | DMG、签名、安装、升级 | 安装卸载无残留且可回滚 |
| 配置 | 注册表、AppData、旧配置迁移 | Preferences、Application Support 迁移 | 行为一致或有明确迁移策略 |

## 7. 客观预检（agent 已做）

- 确认当前版本、分支、上游基线和 fork-only 提交范围。
- 确认 Windows/macOS qmake 分支和发布脚本存在。
- 确认 GitHub Actions workflow 在该 fork 上为 0 次运行。
- 确认产品重命名后 Windows/macOS 发布脚本和更新元数据存在名称、路径、URL 分叉。
- 确认 macOS 有 bundle 自包含门禁，Windows 没有等价门禁。
- 确认 Windows 未同步 Qt 框架中文翻译和 myLPub3D Windows 图标。
- 确认现有检查脚本仍存在旧产品名路径和文件名。
- 未执行 Windows 实机编译，未重跑 macOS release，未读取任何外部远程工作流日志正文。

## 8. 视觉验收（人工必须回填）

- Windows 原版图标、安装器图标、任务栏图标是否与 macOS 品牌一致。
- 中文菜单、中文 Qt 标准对话框、字体和排版是否无裁切、重叠、乱码。
- 同一模型在两端导出的页面、箭头、STEP_BADGE、Callout 和引线是否符合预期。
- Windows 100%/125%/150%/200% DPI 与 macOS Retina 下的版式是否可接受。
- PDF、PNG、HTML 在两端打开后的视觉效果和页序是否一致。
- 安装、卸载、升级后的快捷方式、文件关联和用户数据体验是否可接受。

以上项目在人工明确回填前，状态保持“待人工验收”。

## 9. 总体判断

Windows/macOS 兼容不需要推翻现有架构，主要缺少的是：

1. 统一的跨平台产品身份和发布元数据。
2. 对 fork 新增代码的 Windows 实机构建与运行证据。
3. 对旧应用名、旧路径、旧更新 URL 的全链路清理。
4. Windows 与 macOS 对等的依赖、打包、翻译和自包含门禁。
5. 从“能启动”升级为“同一规格可比较”的功能和视觉验收体系。

完成这些工作后，可以做到“在定义矩阵内可验证的同步”；在此之前，不能保证两个平台功能已经完全同步。
