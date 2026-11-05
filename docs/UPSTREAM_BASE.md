# 作者上游源码基线

本项目的目标底座不是当前随意滚动的 upstream `main`，而是先以作者仓库中与用户固件命名最吻合的版本进行复现。

## 固定上游版本

- 上游仓库： https://github.com/losehu/uv-k5-firmware-custom
- 标签：`0.13.2`
- 标签 commit：`021656ea13226d349e31acd6eb21079c5bab64c7`
- 该 commit 日期：2024-09-21
- 本项目安装脚本：`tools/install_overlay.py`
- 后续不得静默改用 upstream `main` 或更新标签；版本变更必须先对比代码和构建结果。

## 用户提供的固件镜像对应关系

用户上传至本仓库根目录的 `LOSEHU132HS(MDC尾音).bin`：
- GitHub 文件大小：61,458 bytes。
- 作者 `0.13.2` 的 `Makefile` 默认打包前缀为 `LOSEHU132`。
- 作者 README 说明 `HS` 后缀表示 `H + S`：中文全字库/拼音输入相关配置，加 SI4732 支持。
- 作者 Makefile 的后缀逻辑可复现为 `ENABLE_CHINESE_FULL=4`、`ENABLE_PINYIN=1`、`ENABLE_4732=1`。
- 作者 `fw-pack.py` 会在原始 firmware binary 中间插入 16 字节版本字段，并在尾部增加 2 字节 CRC，因此打包文件比原始 `firmware.bin` 多 18 bytes。61,458 - 18 = 61,440 bytes，恰好为 60 KiB。
- 同标签 `firmware.ld` 的 FLASH 长度是 60 KiB。因此该镜像文件长度与 60 KiB 原始固件加 18 字节打包开销吻合。

注意：以上是文件名、打包格式和容量上的强对应关系，尚不能证明用户 BIN 的每个机器码字节都由该标签未经修改地编译产生。文件名中“ M DC尾音”可能代表额外定制，必须继续做二进制/构建配置比对。

## LBJ 专用镜像的构建配置方向

LBJ 固件是独立接收系统，不是把当前 132HS 所有功能原样保留后再塞进 LBJ：
- 保留中文界面所需配置，首选 `ENABLE_CHINESE_FULL=4`。
- 关闭拼音输入 `ENABLE_PINYIN=0`。
- 关闭 SI4732 `ENABLE_4732=0`（该 LBJ 接收链使用 BK4819；SI4732 功能不是 LBJ 解码的依赖）。
- 关闭与 FSK FIFO 冲突的 Messenger、MDC1200、Aircopy。
- 由链接器强制检查 60 KiB FLASH 上限；不能以打包后文件尺寸代替原始固件尺寸。
- 是否需要进一步裁剪其他 LOSEHU 功能，以完整链接结果和中文显示测试决定，不凭估算。

## 已核对的 0.13.2 集成锚点

以下主要锚点在作者标签 `0.13.2` 中存在：
- Makefile：`OBJS += app/messenger.o`、`LDFLAGS =`。
- main.c：`#include "app/app.h"`、唯一的 `RADIO_SelectVfos();`、唯一的 `RADIO_SetupRegisters(true);`、`GUI_SelectNextDisplay(DISPLAY_MAIN);`。
- app/app.c：状态栏 include、读取 `BK4819_REG_02` 中断状态、`APP_Update`、ProcessKey 定义、扫描器按键表。
- ui/ui.h / ui/ui.c：DISPLAY_SCANNER 枚举和显示/按键映射。
- radio.c：三个主要 TX 入口 `RADIO_SetTxParameters`、`RADIO_PrepareTX`、`RADIO_PrepareCssTX`。

安装脚本已调整为用正则匹配 main.c 的 VFO 初始化调用对，兼容该标签实际空行格式。仍须在真实 checkout 上执行 dry-run、安装、构建；锚点核对不能代替编译。

## 构建入口

`.github/workflows/build-lbj.yml` 从固定标签 clone 上游源码，在临时工作目录应用 overlay，并构建 LBJ 专用固件。构建产物作为 GitHub Actions artifact 保存，不把生成 BIN 混入源码提交。

本工作流的首次成功结果才算完整 LOSEHU 工程构建；在此之前，已有的 host tests / overlay stub syntax / ARM object checks 不应称为整机编译。
