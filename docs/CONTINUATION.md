# K6-LBJ-RX 项目交接记录

更新时间：2026-11-05
仓库： https://github.com/rzwcl/K6-LBJ-RX
默认分支：main

## 当前仓库状态

本仓库是 K6 LBJ 独立接收固件的长期源码与交接位置。后续会话应优先读取本仓库，不要依赖旧会话中临时目录或旧 ZIP。

当前 main 分支包含：
- `FirmwareOverlay/app/`：LBJ 应用、POCSAG 解码、业务解析、车型表、车次分类、BK4819 FSK 接收适配器。
- `FirmwareOverlay/ui/`：K6 LCD 页面。
- `src/`、`include/`：主机测试版本和接口头文件。
- `tests/`：主机测试、业务测试、应用测试、射频 FIFO mock 测试及接口 stub。
- `tools/install_overlay.py`：把 overlay 安装进独立的 LOSEHU 源码副本。
- `tools/build_arm_overlay_objects.py`：只编译 LBJ overlay 的 Cortex-M0 ARM EABI 对象文件。
- `build-arm-obj/`、`build-arm-includes/`：上次对象编译输出和本地构建 stub 头文件。
- `docs/`：射频适配设计、验证记录、对象大小记录和 LCD 预览。
- `K6_LBJ_RX_v0.2-RF-integration-candidate.zip`：v0.2 RF 集成候选包。
- `MANIFEST-SHA256.txt`：当前源码包文件校验清单。

## 已确认的基线

- UI 基准：rev3。常规统一字体；车次、方向、速度在第一行；公里标独占一行，显示 K201 形式；型和线放在下方；不显示 A/B 端位。
- 目标设备：老款泉盛 K6，DP32G030 + BK4819。
- LBJ 接收频率：821.2375 MHz；LOSEHU 频率单位按 10 Hz，运行时值 82123750。
- 当前候选 RF 路径：BK4819 直 FSK 1200 + FIFO → MCU POCSAG 软件解码。
- 历史记录暂定 10 条，仅 RAM。
- LBJ 固件必须是独立多系统槽位；不得覆盖日常 LOSEHU、救援固件或修改现有 Bootloader/EEPROM 槽位元数据。

## 用户补充上传的当前固件镜像

仓库根目录新增：`LOSEHU132HS(MDC尾音).bin`
- GitHub blob SHA：`92862b51dd340880f86dd5b4f6ffe99b5d336c6b`
- 文件大小：61,458 bytes。
- 这是编译后的固件二进制镜像，不是 C 源码工程。
- 可以用它作为当前固件的静态分析基准（字符串、版本线索、镜像布局、可识别模块等）；不能直接从中获得可维护的原始 Makefile/main.c/radio.c/app/driver/ui 源文件，也不能据此重新链接出带 LBJ 的固件。
- 暂时只确认该文件已存在于仓库，尚未独立验证其内容是否与电台当前运行版本逐字节一致。

因此，“当前固件 BIN”已补充；“当前固件对应的完整源代码工程”仍未在仓库中发现。

## 已有验证（不要扩大解释）

仓库中的 `docs/VERIFICATION.txt` 记录：
- 主机端 POCSAG、业务解析、应用逻辑、BK4819 FIFO mock 测试通过。
- overlay C 语法检查通过，但使用本仓库 tests/stubs。
- Clang 17 为 7 个 LBJ overlay translation units 生成 ARM Cortex-M0/Thumb 对象文件。
- 安装脚本在 synthetic LOSEHU-shaped tree 上完成 dry-run、安装和幂等性测试。

这些不等于完整固件成功链接。对象编译使用 `build-arm-includes/` 和 `tests/stubs/`，不能代替生产 LOSEHU 头文件及完整链接。

## 仍缺少的内容

仓库当前没有完整的 `losehu/uv-k5-firmware-custom` 基础源码树，特别是实际版本的：
- Makefile、linker script（例如 firmware.ld）
- main.c、radio.c
- app/app.c
- ui/ui.h、ui/ui.c
- driver、bsp、hardware、external 等完整目录
- 该版本配套的打包脚本和工具链配置

因此目前没有可信的完整 LOSEHU 工程链接结果、最终 firmware.bin，也没有实际 BIN 大小与 60 KiB 单槽限制核验结果。

## 下一步工作顺序

1. 获取用户实际使用版本的完整 LOSEHU 源码，放在独立工作副本；不要修改日常固件工作目录。
2. 记录基础仓库 URL、分支/commit SHA、工具链版本。
3. 先审查安装脚本与真实源码的锚点、函数签名、BK4819 寄存器定义及中断处理流程；dry-run 成功后再安装到独立副本。
4. 用完整工程工具链构建 `ENABLE_LBJ=1`，关闭 Messenger/MDC1200/Aircopy 等会争用 FSK FIFO 的功能。
5. 检查链接 map、各段大小、最终 firmware.bin 大小；必须实测不超过既有多系统引导器的 60 KiB 单槽限制。
6. RF 上台前先用已知 POCSAG IQ/音频或信号源验证前导码、首次同步、同步字是否被硬件消费、FIFO 字节顺序、位序、正常/反相极性、报文长度和接收窗口结束后的重捕获。
7. 只有在 RF 验证和整机尺寸检查完成后，才考虑独立备用槽位测试。不得写日常槽位、救援槽位或 EEPROM 元数据。

## 关键风险提醒

- `lbj_radio.c` 目前的 BK4819 packet/FSK 连续捕获假设仍未实机确认。寄存器值和 FIFO mock 测试通过，不代表真实芯片会按预期捕获完整 POCSAG burst。
- 2048 字节数据窗口的时长估算不代表每个 LBJ burst 都能完整落在该窗口内，也不代表窗口结束后可以可靠重捕获。
- 不要把当前 ZIP 或对象文件当成可刷写固件。
- 不得声称已完成 K6 实机接收、灵敏度、屏幕或多系统槽位测试；这些测试尚未执行。

## 后续会话约定

每次开始：
1. 读取本文件、README.md、docs/VERIFICATION.txt、docs/FSK_ADAPTER_DESIGN.md。
2. 检查 GitHub main 当前 commit，确认文件没有被用户更新。
3. 从仓库当前源码继续，不从旧会话临时文件猜测版本。

每次结束：
1. 把新增/修改源码和测试放回本仓库。
2. 更新 docs/VERIFICATION.txt 和本文件中的状态。
3. 只有确实生成并验证的文件才记录为产物；不上传工具 stub 冒充生产头文件。
4. 明确区分主机测试、stub 语法检查、ARM 对象编译、完整链接、RF 实机测试和刷写测试。
