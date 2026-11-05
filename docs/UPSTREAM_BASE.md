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

进一步对用户 BIN 做了真实解包校验：按作者 0.13.2 的 128-byte XOR 混淆表解码，恢复出头部向量表；0x2000 处的 16-byte 版本字段为 `*OEFW-LOSEHU`；尾部 CRC-16/XMODEM 计算值 `0xEE94` 与文件保存值一致。由此确认该文件采用作者 fw-pack.py 的打包格式且未发生传输损坏。由于版本字段写的是 AUTHOR_STRING（默认 LOSEHU），并不包含 HS 后缀，因此仍不能仅凭此字段证明源代码 tag 和机器码完全一致；文件名中的“MDC尾音”也可能代表额外定制。

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

## 完整构建结果（2026-11-05）

GitHub Actions run：`41962240610`
- 源码提交：`e9e939fc472b64f8ca9a73b4492f3d85e9aa7fde`
- Artifact：`K6-LBJ-RX-LOSEHU132K`（ID `13385797964`）
- Artifact URL：`https://github.com/rzwcl/K6-LBJ-RX/actions/runs/41962240610/artifacts/13385797964`
- raw firmware：53,908 bytes
- packed LOSEHU132K.bin：53,926 bytes；额外 18 bytes 为版本字段和 CRC
- CRC-16/XMODEM：`0x9DF8`，验证通过
- FLASH：53,908 / 61,440 bytes（87.74%）
- RAM：5,128 / 16,384 bytes（31.30%）
- map / ELF / raw BIN / packed BIN / JSON 报告均在 artifact 中
- 构建工具链：arm-none-eabi-gcc 10.3.1、binutils 2.38、newlib 3.3.0

## 构建入口

`.github/workflows/build-lbj.yml` 从固定标签 clone 上游源码，在临时工作目录应用 overlay，先 dry-run，再安装并做二次 dry-run 幂等性检查，最后构建 LBJ 专用固件。构建产物作为 GitHub Actions artifact 保存，不把生成 BIN 混入源码提交。

完整链接已经通过，但不等于 K6 实机验证。RF 捕获、FIFO 延迟/溢出、LCD、no-TX 和多系统备用槽位测试仍未执行。
