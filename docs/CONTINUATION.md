# K6 LBJRX / TDCSRX 原版移植 — 当前交接记录

更新时间：2026-11-06
仓库：https://github.com/rzwcl/K6-LBJ-RX
默认分支：`main`
当前 main HEAD：`223accd95dc3be1dd9b7d30455691e4fb05b66cc`

## 目标（不能偏离）

用户要求的是把 **原版 LBJRX 和原版 TDCSRX** 分别适配到老款泉盛 K6（DP32G030 + BK4819），最后生成两份独立、可通过现有 LOSEHU 多固件 Bootloader 选择启动的完整 K6 固件镜像。

- LBJRX 与 TDCSRX 是两个分开的完整固件目标，不合并成一个 App。
- 不重写协议层、不把其他项目中的实验代码冒充原版实现。
- 不基于本仓库上一轮错误的 `K6-LBJ-RX` 实验解码器继续开发。
- 不修改基础 Bootloader、`L_BL002` 或用户已有固件条目。
- 每个代码修改或可独立验证阶段完成后都要提交 GitHub，并报告提交号与验证结果。
- 任何 CI 编译成功、CRC/尺寸通过，都不能说成 K6 实机已正常工作。

## 重要安全状态

**当前仓库没有可刷入的 LBJRX-K6 或 TDCSRX-K6 固件。** 上一轮错误实验代码生成的 BIN 不要刷。相关源码、专属 build 脚本、测试、对象文件、Actions 工作流和 BIN 已在当前相关分支清理为删除提交；Git 历史仍保留旧提交，没有执行历史重写。不要从旧 SHA 或旧 Actions artifact 下载旧 BIN。

## 当前源码资料

### LBJRX 0.4

公开归档位于 `troilus/lbjradioweb/archive/apps/LBJRX-cn.v5.5.6.app`。

上游源码候选：
- 仓库：`troilus/k1lbj`
- 源码目录：`App/apps/lbj/`
- 源码 commit：`de6f77dd274f62c460b953d4ac639310906c256c`
- build.sh 标记：App version `0.4`，API min `2`。
- LBJ app 文件原样副本已提交到 `research/original_apps/LBJRX-RX04/`，共 6 个文件。6 个源码文件的 Git blob SHA 与 upstream 完全一致。
- 发布包版本/API 与 build.sh 匹配；尚未用原作者工具链重建并比较代码 CRC，因此不能说已完成 1:1 可复现构建。

重要源文件：
- `lbj_app.c`：原版 APP UI 和硬件采样入口；PA4 ADC 音频采样 19.2 ksample/s，16 samples/bit。
- `lbj_dec.c/.h`：`lbj_rx_bit()` 的 POCSAG 1200 bitstream 解码器。
- `lbj_dec.h` 明确说明 decoder 是从模型/test 和 Sdr-Is-Fun RTL_SDR_LBJ_RECEIVER 转写，不能称为旧二进制的直接反汇编。
- K6 适配应该保留原 decoder 逻辑，单独替换 ADC/discriminator 入口，把 BK4819 FIFO bits 送给 `lbj_rx_bit()`。

### TDCSRX 0.3

公开归档位于 `troilus/lbjradioweb/archive/apps/TDCSRX-cn.v5.5.6.app`：
- FAP1，4,140 bytes，code 4,076 bytes，TDCS RX 0.3，API 1/3，link VMA `0x20000280`。
- Git blob SHA：`af2142508e75b9ef2d2d2c4963be33ace5ee8523`。
- 当前没有找到原作者完整 C/H 源码。
- `research/tdcsrx_rx03/` 保存部分机器码逆向恢复的 protocol/stream C、synthetic tests、反汇编报告。
- 只恢复了一部分帧解析、BCH、sync/26-bit word collector；完整音频 DSP/固定点阈值/帧 lifecycle/UI 还没完成。
- 这不是完整原作者源代码，不是 RF receiver，也不是 K6 固件。

## TDCSRX 地址校正

FAP1 code link VMA 是 `0x20000280`；下列是相对于 code section 的文件偏移。旧文档曾将 runtime address 误写为 code offset，已在 2026-11-06 提交修正。

| 文件偏移 | 运行地址 | 作用 |
|---:|---:|---|
| `0x0164` | `0x200003E4` | SWAR popcount，sync Hamming distance |
| `0x01D4` | `0x20000454` | BCH(26,16) syndrome / remainder |
| `0x0204` | `0x20000484` | 26 个 single-bit syndrome，one-bit correction |
| `0x061C` | `0x2000089C` | 45-byte application frame parser |
| `0x0800` | `0x20000A80` | sync/polarity、26-bit collection、frame assembly |
| `0x0950` | `0x20000BD0` | 9.6 ksample/s audio DSP |
| `0x0D0A` | `0x20000F8A` | sample loop 调用 `0x0950` |

正确调用链：`0x0D0A -> 0x0950 -> 0x0800 -> (0x0204 BCH, 0x061C frame parser)`。
runtime `0x20000A80` 是 code offset `0x0800`；runtime `0x20000484` 是 code offset `0x0204`。

## K6 硬件/底座基线

- LOSEHU 上游：`losehu/uv-k5-firmware-custom`
- 固定 tag：`0.13.2`
- commit：`021656ea13226d349e31acd6eb21079c5bab64c7`
- 目标芯片：DP32G030；射频芯片：BK4819。
- 在该基线中 `GPIOA_PIN_KEYBOARD_1 = 4`；键盘矩阵使用 PA4。DP32G030 `portcon.def` 未为 PA4 列出 SARADC 功能；SARADC 通道从 PA5/PA6 等引脚开始。
- 原版 LBJ/TDCS 的 PA4 ADC 采样都不能直接复制到 K6。

固定 0.13.2 BK4819 参考：
- `BK4819_SetupAircopy()` 使用直 FSK 1.2K RX 配置 `REG_58=0x00C1`，但该函数还包含 Aircopy 的 CRC/包长参数，不能不加审查直接用。
- `enable_msg_rx(true)` 配置 FFSK 1200/1800 RX；`REG_58` 组合值 `0x3F03`，同步字、长度、FIFO threshold 等必须按 TDCS 原协议重设。
- `REG_5F` 是 16-bit FIFO；原固件读取时低字节先、高字节后。MDC1200 代码用 `REG_0B[7]` 检查负极性并 XOR FIFO word。
- BK4819v3 RX FIFO 为 8 words，`REG_5E[2:0]` 设置 almost-full threshold；`REG_5D` 数据长度字段共 11 bits，最多 2,048 bytes。
- 这说明 K6 有硬件 FSK/FFSK 通路可研究，不代表同步、极性和位序已经实机验证。

目标模式：
- LBJ：821.2375 MHz、POCSAG 1200；优先验证 direct FSK FIFO 方案。
- TDCS：457.550 MHz、FFSK 1200/1800；优先验证 BK4819 FFSK FIFO 替代 PA4 软件音频采样。
- 硬件 sync 是否设置为协议 sync，或在 preamble 阶段启动 FIFO、再由原软件层容错搜完整 sync，尚未决定，必须用受控信号源实测。

## 用户已确认的显示要求

- 开机页显示 `LBJ RX 821.2375` 约 1.5 秒，然后转主页面，频率不常驻。
- 主页第一行：车次 + 上行/下行/待定 + 速度；方向夹在车次和速度之间，不加方向前缀。
- 第二行：公里标独占一行，例如 `K201`，显示整数公里，内部保留小数用于方向判断。
- 底部：机车型号/机车号、线路。
- 补充页：新版车次类型、经纬度原始数字，不显示 A/B 端位。
- UP 新一些的历史、DOWN 旧一些的历史、MENU 切换主/补充页、EXIT 返回普通电台页面。
- 历史 10 条，仅 RAM，不写 EEPROM。
- 不增加长按功能，不抢现有 Bootloader 按键组合。

## Bootloader 约束

用户现有 `losehu/uv-k5-bootloader-custom` Release `0.02`：
- 512 KiB EEPROM 保存多固件目录和镜像。
- 目录最多 64 项，每项 32 bytes，位于 EEPROM `0x40000` 一带。
- 二级引导 `L_BL002.bin` 在 `0x41000`，选择固件后加载到 MCU Flash。
- 每个完整固件 raw image 不超过 60 KiB。
- 目标是新增 LBJRX 和 TDCSRX 两条完整固件目录项；保留现有固件、Bootloader 和救援项。

## 错误实现清理

上一轮错误路线把本仓库自己的实验解码器误当成原版 LBJRX/TDCSRX，错误代码已经从相关当前分支删除。清理涉及旧实验 LBJ/TDCS sources、对应 host tests/stubs、专属 build/install scripts、对象文件和错误 BIN；具体清理提交应以各分支 git log/tree 为准。

保留内容：
- TDCSRX 原版 FAP1 的部分逆向恢复资料；
- LBJRX 0.4 上游 source copy；
- 通用 K6 APP runtime/EEPROM研究代码（在 feature branch，属于另一条通用工作，不用于本次完整固件移植）；
- 用户已有 EEPROM/Bootloader 研究和 UI 规格。

不恢复、不引用旧的 `FirmwareOverlay/app/lbj_*` 或 `tdcs_*` 实验实现。

## 关键 Git 提交

- main 当前 source archive commit：`223accd95dc3be1dd9b7d30455691e4fb05b66cc`。
- main TDCS 地址/信号路径修正文档：`e2c9dd87d954e1b5fdf1fe487c7498afc735ce53`。
- feature branch TDCS 地址/信号路径修正：`c2c447943bb2703093a813667dd28afff5ac2257`。
- 错误实验模块清理提交位于各相关分支历史；删除文件仍能在旧 commit 中查看，本仓库没有做 force-push 或历史重写。

## 下一步工作顺序

1. 在主机端确认 LBJRX 0.4 source copy 与公开 `.app` 的 metadata/CRC；尽可能用上游固定工具链复现源代码。保留原始文件不改。
2. 继续逆向 TDCSRX 0.3：补全 `0x0950` fixed-point detector、`0x0800` frame lifecycle、callback 和 UI。不要仅凭当前 synthetic tests 声称全部恢复。
3. 从 LOSEHU 0.13.2 新建干净工作副本；只移植原版 bit/protocol logic，另写 receive-only BK4819 FIFO 前端。不要恢复旧 K6-LBJ-RX 实验解码器。
4. 分别构建 LBJRX-K6 和 TDCSRX-K6 完整固件镜像，每个目标独立配置/独立输出。
5. 做 host tests、完整 ARM GCC 构建、尺寸/CRC检查，再做 K6 实机无发射、信号源同步/位序/极性/重捕获测试。
6. 每完成一个独立阶段就 commit GitHub，记录 SHA、测试结果和仍未验证的内容。实机验证之前不得称为“可正常使用”，也不得给出刷写建议。
