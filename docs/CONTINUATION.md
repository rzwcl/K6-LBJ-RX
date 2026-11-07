# K6 LBJRX / TDCSRX 原版移植 — 当前交接记录

更新时间：2026-11-07
仓库：https://github.com/rzwcl/K6-LBJ-RX
默认分支：`main`
当前 main HEAD：40746654a32f1d428b6b768957cda5f302092be4。

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


## 2026-11-07 继续推进记录（最新）

### TDCSRX：硬件同步到软件帧同步的接口验证

- 当前仍没有原作者完整 TDCSRX C/H 源码；tdcsrx_recovered_* 仍属于 0.3 FAP1 的二进制逆向恢复。
- 已核对 LOSEHU 固定基线的 FSK 中断处理：RX_SYNC 时读取 REG_0B[7] 极性状态；FIFO almost-full 时按 16-bit word 读取 REG_5F，现有 MDC 路径低字节先、高字节后。这个现有代码说明硬件同步事件与后续 FIFO payload 分开处理，但不证明 TDCS 的 bit packing。
- 对照公开铁路协议 CN202153314U，信号结构为 51-bit 交替位同步、31-bit frame sync 0x0DD4259F、BCH(26,16)。
- 新增 research/tdcsrx_rx03/K6_BK4819_FIFO_ADAPTER.md。候选实验是 BK4819 用 2-byte 交替位标记（初步 REG_5A=0xAAAA）锁定前导码，预期硬件消费16位，FIFO保留35位交替前导码 + 31位软件帧同步 + 23个26-bit BCH字。
- 这个输入长度计算为664 bits / 83 bytes；如果 BK REG_5D 的长度字段最终确认按 length-1 表示，候选值会是 0x5200。这只是计算假设，不是可以直接写进K6的配置。必须先通过原始 FIFO 捕获确定是否对齐/是否吞掉恰好16位/字节与位序/极性/长度计数。
- b98b059c2c0e36699d71be05d0a178cc41dd8aea 添加上述 TDCS handoff 主机测试，证明软件 collector 能处理35位 residual preamble 后跟31-bit sync。该 commit 的 GitHub Actions run 42340865818 中协议、BCH、45-byte parser、双 collector 和此 handoff test 均通过。
- TDCS ADC-DSP 到整帧测试仍失败：当前模拟9.6 ksample/s音频不能同步，回调为0。最近 CI 42341006065 仍以 test_full_9600_hz_audio_to_frame 的断言失败结束。这个失败没有删掉，也没有把它改成通过。TDCSRX_REVERSE_ENGINEERING.md 与 TDCSRX_RX03_SIGNAL_PATH.md 已修正状态说明。
- 所以 TDCS 的软件帧层可单独主机验证，sample-to-bit DSP 仍未验证；BK4819 FIFO 端到端也未验证。

### LBJRX：原版 POCSAG 解码器的 FIFO 前导码交接

- 原版 lbj_rx_bit() 的行为进一步核实：每个输入bit先进入32-bit shift；它自己搜索 0x7CD215D8，正常极性同步距离 <=2，反相距离 >=30；同步后每32位处理一个POCSAG码字，调用原版 BCH/偶校验/地址/BCD/批次逻辑。
- 新增 test_residual_pocsag_preamble_after_hardware_sync()，直接编译原样 lbj_dec.c，模拟16位硬件同步已被消费、软件侧继续收到560位交替前导码，再输入原始32-bit POCSAG sync、地址字、消息字和idle字；正、反极性两种输入都必须产生RIC 1234000、BCD 12345。
- 测试提交：3326fe2be039f85716bd44369e7c9ba2a45a4f3b；GitHub Actions run 42341138790 success。这只证明原版软件decoder能接住假设的剩余前导码，不证明BK4819真的按此方式锁定/剥离/输出。
- 新增 research/lbjrx_v04/K6_BK4819_FIFO_ADAPTER.md，记录 direct FSK 1.2K候选、2-byte交替位硬件sync思路、原版 lbj_rx_bit() 接口和必须通过实测确认的FIFO问题。
- POCSAG 是重复 batch 流（1个32-bit sync + 16个32-bit codewords），不同于 TDCS 单个83-byte frame。不能简单把一次 FIFO almost-full 当成完整消息，也不能猜一个 REG_5D 长度。必须测连续接收/硬件RX finished行为/重新arm/剩余字读取。

### 本轮新提交

| Commit | 内容 | 当前验证 |
|---|---|---|
| b98b059c2c0e36699d71be05d0a178cc41dd8aea | TDCS residual preamble → frame sync handoff host test | 协议和帧测试通过；整体因 DSP 音频到帧失败 |
| 19c51532a877d1a2eb4c40cdbee3658621b2d0c9 | TDCS K6 BK4819 FIFO adapter 设计记录 | 文档 |
| be2c966b8c42e9770354478bb1d5d97f5a3193bd | 修正 TDCS DSP 未通过说明和 FIFO 策略 | 文档 |
| a98a783b00061f4341fe42f4dfcd6776c5271d2c | 修正 TDCS sample-to-bit 失败说明 | 文档 |
| 3326fe2be039f85716bd44369e7c9ba2a45a4f3b | LBJ residual preamble host test | GitHub Actions 42341138790 success |
| a1c571c86e94f57a28b0f8e3f1a87f75f60bec79 | LBJ K6 BK4819 FIFO adapter 设计记录 | 文档 |

### 现在真正缺少的输入/下一步

1. 拿到可与目标信号同步的受控信号源或一段可关联解码内容的原始 BK4819 FIFO dump。必须包含已知TDCS/POCSAG帧以及 REG_02/REG_0B/REG_5D/REG_5E/REG_5F 原始记录。没有这一步，就无法负责任地固定字节序、bit order、sync stripping和极性。
2. 继续追 TDCSRX 0.3 的 DSP：优先逐条比对 0x0950 Thumb 指令对 I/Q、平滑基线、差分、符号变化、phase accumulator 和调用 0x0800 的条件；不能靠随意调振幅把测试调绿。
3. 在真实FIFO格式确认后，再实现单独的 receive-only BK4819 adapter：LBJ 走原版 lbj_rx_bit()，TDCS走已恢复的 software sync/BCH/frame path（或继续从原二进制复原完整软解调链，不能宣称原始DSP已验证）。
4. 再从干净的 LOSEHU 0.13.2 基础建两个独立完整固件目标，分别完成 UI、RAM历史、频率切换与退出恢复、无发射保护和尺寸限制。
5. 通过 ARM 编译、host tests、尺寸/CRC，再上K6做受控实机接收测试。未完成上述验证前不发布、不刷机。

当前安全结论不变：仍无可刷入的 LBJRX-K6 或 TDCSRX-K6 完整固件。


## 2026-11-07 第二轮进展：真实 TDCS 音频对照、CI 拆分、源码包已提交

### GitHub main 最新提交

当前 main HEAD：4f12c5ef664b2e4f31cf94030c62216e8430c617。

本轮提交到 rzwcl/K6-LBJ-RX 的重要改动：

| Commit | 文件/内容 | 结果 |
|---|---|---|
| 6ca52dd26db000ab833be7f14fd62d03900f18a9 | tdcsrx_recovered_dsp.c：按 Thumb 顺序修正判决减法 | 已合入；不代表 DSP 已通过 |
| 1688ce5b91eab267e4709ebb12832b8fef69996d | tdcsrx_recovered_dsp.c：恢复 baseline 下降分支的正确跳转 | 已合入 |
| 66a0d193ba220cc9458878bb1d5d97f5a3193bd | DSP baseline 数值断言 | 已合入 |
| 583dc30de3c2def7556e0aebb5a9fe7ad796b11a | prepare_real_wav.py：固定真实 WAV 的 hash 校验、重采样/ADC 样本生成 | 已合入 |
| 76e8459851f43fbf11d1691d54e9136b5c1d3cc3 | test_tdcsrx_real_wav.c：真实录音 DSP 探针 | 已合入 |
| 81757d1e2166ed87e03924142c72c24178666ee6 | compare_real_wav_bits.py：与独立参考位流比较 | 已合入 |
| de26c9a20504aefb99049f1be324a88dcfdf0f5f | 真实 WAV 诊断工作流上传逐样本数据和源文件 | 已合入 |
| acdc575603d24c5a08873dead13c2c2816e74093 | Makefile：将 protocol/frame 测试与未通过的 DSP 诊断分开 | 已合入 |
| b7086e5fa07bf243b020b3ce1a0ee2329746d94a | test-tdcsrx-recovered.yml：只运行已通过的协议/帧测试 | 已合入 |
| f7a4746620bfddfe6cd8fb89774e50584c27c71c | package-tdcsrx-recovered.yml：通过协议/帧测试后打包源代码 | 已合入 |
| 2a9df399f7a02e99568b683a1425595dff611bf9 | 更新 research/tdcsrx_rx03/README.md | 已合入 |
| 4f12c5ef664b2e4f31cf94030c62216e8430c617 | 新增 TDCSRX_REAL_WAV_DIAGNOSTIC_2026-11-07.md | 最新 main HEAD |

注意：以上是研究源代码、测试、文档和 CI 文件的提交，不是 K6 固件提交；没有完整 K6 BIN，也没有刷机动作。

### 真实 457.550 MHz WAV 对照结果

固定样本：
- 来源：rzwcl/450-TAx2
- 文件：2026-10-11_17-30-46_audio_457550000Hz.wav
- Git blob SHA-1：8fb409ef803a98c17834fa230410c4579b030a83
- 22,050 Hz mono 16-bit PCM，37,418 samples，时长 1.696961 s。
- 独立 48 kHz 参考解调在 bit 1376 / 1.146667 s 找到 31-bit sync，和原项目约 1.146593 s 的结果对应。
- 参考帧记录：X319、20 km/h、CRC-16/XMODEM 0xB27A。

转换为 9.6 kHz 后得到 16,291 个 ADC 样本。C DSP 对 ADC 增益 0.50–2.00 扫描仍没有完整帧回调。

| 诊断项 | Path A（2:3） | Path B（3:2） |
|---|---:|---:|
| 1.00 增益下符号数 | 1,988 | 1,865 |
| 1.00 增益下 bit=1 数 | 273 | 1,352 |
| 最近参考同步字 Hamming distance | 9 | 7 |
| 最佳对齐 BER | 0.3217 | 0.4038 |

全增益范围内 Path A 最近同步字为 8–10 bit 错误，Path B 为 7 bit 错误；没有任何路径解出 X319 / 20 km/h / CRC 0xB27A。简单 8-sample 正交能量判决可达约 88.6% bit agreement，但这是非原版简化检测器，不可冒充原版 TDCSRX DSP，也不足以用于刷机。

原始逐采样 trace、C 位流、48 kHz 参考位流、原 WAV 和9.6 kHz ADC样本保存在 Actions diagnostic artifact：
- Workflow run：42344201505
- Artifact：tdcsrx-real-wav-diagnostics-42344201505
- 下载压缩包中包含 CSV 和所有 path bitstreams。
- 完整测量和后续步骤见 research/tdcsrx_rx03/TDCSRX_REAL_WAV_DIAGNOSTIC_2026-11-07.md。

### DSP 指令复核中修正的两个问题

1. 判决减法顺序按 0x0A54..0x0A60 指令恢复为：
   mag1200 * smooth1800 * weight1200 - mag1800 * smooth1200 * weight1800。
   之前初版 C 写反，已改。
2. baseline helper 的下降分支在 0x01D2 跳到 0x01C2，跳过上升分支的 ASRS #5。正确公式为 baseline - (baseline >> 10)，不是 >>15。之前的 >>15 试验已撤销，并同步修正测试和文档。

这两项修正后真实 WAV 仍未通过；不要把它们理解成已经修好 sample-to-bit DSP。

### CI 拆分和源代码包状态

由于 DSP 端到端诊断仍失败，不应让它阻断已验证的协议/帧源码包，但也不能删掉失败测试：

- GitHub Actions run 42344319294：Test recovered TDCSRX RX 0.3 protocol and frame path — success。
- GitHub Actions run 42344319283：Package TDCSRX recovered source — success。
- 成功生成源代码工件：TDCSRX-RX03-recovered-source（artifact id 13563360117）。
- 该源代码包包含 20 个文件，含 protocol/stream/receiver/DSP C 源、头文件、测试、Makefile、README、逆向报告和 BK4819 FIFO adapter 文档。
- GitHub Actions run 42344201505：真实 WAV DSP probe — failure；失败是保留的诊断结果，不是协议层测试失败，也未被隐藏。
- 当前 Makefile 的 test / test-protocol 运行已通过的 protocol/frame 层；make test-dsp 单独保留未通过的样本到帧诊断。
- 真实 WAV 探针由独立 test-tdcsrx-real-wav.yml 执行，并上传诊断工件。

这意味着：**源代码和已验证协议/帧层已经提交到 GitHub；DSP 的真实音频端到端仍失败；不能宣称完整接收链完成。**

### 当前下一步（按优先级）

1. 取 run 42344201505 的 CSV，在 sync 周围逐样本重算 target_filter、4 个 I/Q 累积、两个 baseline、两条 diff 和 DPLL phase；逐条对齐 0x0950 的 Thumb 指令，而不是继续改振幅。
2. 重点检查 +0x2E..+0x6D 的 8×4 int16 ring addressing、两张 tone lookup table 的实际字节、各次 STRH/LDRSH 的旧值/新值对应，必要时建立逐指令 Python reference model。
3. 判断 Path A/B 的 false slicer transitions 和 DPLL phase event 漏计，解释为什么接近目标帧时间时分别比参考 bit index 少/多几十个符号。
4. 若未来考虑 8-sample 正交算法作为备用，必须把它作为非原版替代算法隔离，不可以写进“原版 TDCSRX 移植已完成”的结论。
5. 仍需通过受控信号源取得 K6 的 REG_02/REG_0B/REG_5D/REG_5E/REG_5F 原始 FIFO dump，确认 byte/bit order、hardware sync stripping、polarity、长度和重捕获。
6. 在 DSP 与 BK4819 FIFO 两块未验证前，不开始发布/刷入 TDCSRX-K6 BIN。

当前安全结论：原版 TDCSRX 的协议/帧层部分可读 C 重建已提交，protocol/frame 主机测试通过；原版 ADC DSP 的 C 重建仍未通过真实 WAV；BK4819 FIFO 与 K6 实机仍未验证。LBJRX bit decoder 的主机交接测试仍通过，但其 BK4819 FIFO 也尚未实测。没有可刷入的 LBJRX-K6 / TDCSRX-K6 完整固件。


## 2026-11-07 第三轮补充：位流参考对照可重复化

- 新增 compare_real_wav_bits.py 中的独立 9.6 kHz、8-sample 正交能量参考 detector，并将 workflow 改为同时传入 48 kHz 参考 WAV 和 9.6 kHz ADC 样本。
- 新增代码提交：65864061e4cb2fad7ca9a12817ceb7c5d35b35df。
- workflow 调用更新提交：417e565c33f60075001a0d4c4381ae8ed60821d3。
- 最新真实 WAV run：42344820815，输出 SIMPLE_9600_QUADRATURE best_phase=1、bit_agreement=0.8831、ones=920/2036。
- 同一 run 中原版 C DSP 仍未解出帧；Path A 的最近 sync distance 为 8–10 bit，Path B 为 7 bit，对齐 BER 约 0.32–0.40，frame callback=0。该真实音频探针仍为失败状态。
- 更新 report 提交：54ba75b169fac5e527e30333ccf270b244c7a109；README 对应数值更新提交：86128fdf8e4683aaa98f8b978bf37dd2b949305c。
- 独立正交判决仅作为分析基准，不是原版 TDCSRX 0.3 DSP，也没有集成 K6 固件。

当前 main 已提交全部上述源码、诊断脚本、CI、README、逆向报告和交接文档；本轮 protocol/frame CI 通过、source package workflow 通过，真实 DSP probe 保留失败。下一步继续做逐样本指令级差分，优先验证相关器 ring indexing 与 DPLL event timing；不要启动正式 K6 固件集成直到 sample-to-bit 路径和 BK4819 FIFO 交接都得到验证。


## 2026-11-07 第四轮进展：参考位流已可独立通过，原 DSP 仍未通过

### 本轮已提交到 rzwcl/K6-LBJ-RX

- ab75c8e80b4762fd2ca12ba77d2132da587c0a4c：修正 synthetic FFSK fixture 的 1200/1800 幅度方向（原先把“1”的 1200Hz 振幅设置得比“0”的 1800Hz 还小）。
- ea61ea31b5a6672b13f9f3420291c11cfb66f79f：真实 WAV workflow 额外运行独立 test-dsp 诊断，保留失败结果而不阻断后续对比/上传。
- dabe152ae8c0afc3eab7437933d3df3e9f00a43d：将合成输入试验调整为 1000:400。
- 57b460383d76e45a0268a9f8ed59150208849bfd：确保 synthetic DSP test 文件变更也能触发真实音频诊断 workflow。
- 401b4d11824e2cf6a52d9997e900d8bdf46bd246：合成 FFSK fixture 改为 1200:300，并保持独立 WAV reference 对照。
- 8a63b4e89d6370cc5fc28244120724948104bbfd：合成 FFSK fixture 扩大到 1200:150（8:1 输入振幅比）。
- dcf34b4a2d676e23662f438915f3e6f758fd9000：补充真实 WAV 独立 9.6 kHz detector 成功记录和 8:1 synthetic 仍失败的数据。
- 729a76ceaea58e60f376aa424cd134d58be8d8a5：更新 TDCSRX README 的成熟度划分。
- d7d642fcd7fa493dcbeda33979387d09ba15e811：更新 sample-to-bit 路径报告。
- 1a73f4ec417e91231fdfb2ae95ea3694e4ba8637：更新反汇编报告，补入独立 reference frame pass 和 synthetic DSP failure。
- 本轮结束时 main HEAD：1a73f4ec417e91231fdfb2ae95ea3694e4ba8637。

### CI 状态（不能混为一谈）

- 42348715484：Test recovered TDCSRX protocol/frame path — success。
- 42348715565：Package TDCSRX recovered source — success。
- 42348715488：Synthetic + real 457.550MHz DSP probe — failure，失败保留，没有删断言。
- 同一真实 WAV run 中，独立 9.6kHz quadrature reference detector 能解出 BCH/CRC 通过帧；原版 DSP C 路径仍回调 0。
- 8:1 synthetic FFSK fixture 也不能通过：输入逻辑位 694 个，两路 DSP 各输出约831/825个符号；交替前导码早期，Path A 的差分保持负，Path B 保持正，没有形成两条互补的正确位流。

### 现在能确认的边界

1. **协议/帧层**：BCH(26,16)、sync/polarity、最多2-bit sync tolerance、双 collector、45-byte parser、CRC-16/XMODEM 有 host tests；protocol/frame CI 通过。
2. **独立 reference detector**：针对固定 457.550MHz WAV，9.6kHz 的8-sample正交能量判决在相位0、sync bit259处恢复完整帧；BCH纠错0、header/padding有效、CRC B27A/B27A，解析为 X319、20km/h。这不是原版TDCSRX DSP。
3. **原版 DSP 逆向 C 翻译**：仍未通过。真实 WAV 下没有frame callback，C与reference位流的BER约0.32–0.40；8:1合成fixture同样未通过。
4. **BK4819 FIFO**：只是候选适配方案；没有 K6 原始寄存器/FIFO dump，位序、字节序、硬件sync剥离、极性、长度/连续接收尚未实测。

### 下一步（按证据顺序）

1. 建立逐指令 reference model，逐样本复算 0x0950：DC/biquad、8x4 ring history、4组 I/Q 累积、0x0198 两个baseline、cross-normalization、两个权重差分、previous sign、phase correction及collector调用。对 synthetic 和真实 WAV 分别做 C vs reference model 对比。
2. 在 synthetic 测试的 audio_feed_bit() 内部记录每个输入bit的8次 sample：每sample的差分符号、phase before/after、是否wrap/emit；不要再靠扫输入振幅改变结果。
3. 核对 0x0950 调用 0x0800 时第三个参数的语义（调用者传入的是 literal-pool 指针），以及 0x0800 中collector回调/数据生命周期；host parser接口目前只模型了接收到位后的协议部分。
4. 并行维持 BK4819 raw FIFO capture 方案；没有实际寄存器数据前不写生产 tdcsrx_bk4819.c，也不采用猜测的 REG_5D=0x5200。
5. LBJ 继续沿原版 lbj_rx_bit() 做 BK4819 direct-FSK FIFO交接；其残余560-bit POCSAG preamble host test已通过，但同样没有实机fifo格式验证。
6. 之后才从干净 LOSEHU 0.13.2 树分别建完整 LBJRX-K6 和 TDCSRX-K6 固件目标。现在仍无可刷BIN，不能让用户刷试验文件。

**不要把独立9.6k quadrature reference detector称为“原版TDCSRX恢复成功”；也不要将当前DSP失败改成通过或删除。**


## 2026-11-07 第五轮进展：原版 TDCSRX frame callback ABI

- 原始 TDCSRX `0x0DB4` literal word 为 `0x20000815`，作为 Thumb 函数指针传给 `0x0950` DSP；其实际函数入口是 code offset `0x0594`（runtime `0x20000814`）。
- `0x0950` 在帧完成后把保存的第三参数传入 `0x0800` collector；collector调用 frame parser成功后再调用该回调。第三参数因此确认是原应用 frame callback，不是任意的 DSP 上下文指针。
- 回调将解析记录从 r0 复制到 global app object 的 `+4`：三次 12-byte LDM/STM 加一次4-byte LDR/STR，总计 **40 bytes**；之后更新计数/状态并调用 global display hook。
- 先前报告把这个长度写成44 bytes，现已纠正。原因是 parser 的 `cmp r1,#0x2c` 检查的是输入45-byte frame的 last-index（44），不是输出记录大小。
- parser 已确认输出字段偏移：prefix 0..3，train number +4，mileage +8..+0xB，loco serial +0x0C，weight +0x0E，length +0x10，speed +0x14，loco type +0x16，role +0x18，mileage flags +0x19..+0x1B，car count/station/track fields +0x1C onward，signal machine/kind +0x22/+0x23。部分 +0x1C..+0x21 的动态字段映射还需结合 parser 的动态索引和 display读引用继续确认。
- `tdcsrx_message_t` 含 raw payload/frame/CRC 调试字段，不是40-byte app record。不可直接传给原应用 UI callback。完整的40-byte `tdcsrx_app_record_t` 及 conversion routine 尚未添加。
- 本轮报告提交：`f78d130abd54f40196fdd965c1e424f5520f41bf`（reverse report）与 `40746654a32f1d428b6b768957cda5f302092be4`（signal path report）。该两次提交后的 protocol/frame tests 和 source package CI 均成功：runs `42349535251`、`42349535408`。

下一步先完成 parser 的40-byte记录偏移映射，再逐指令对照 `0x0800` collector 怎样在帧解析成功时把40-byte记录交给回调；在此之前不要把host message struct当作原版结构。


## 2026-11-07 第六轮补充：纠正记录长度并完成 parser 固定偏移初图

### 关键纠正

此前把应用回调的输出记录误写成44字节。按原始 Thumb 指令重新数寄存器传输：三次 `LDM/STM {r2,r5,r6}`（36字节）加一次 `LDR/STR`（4字节），回调实际从解析器输出指针复制 **40字节** 到全局 app object 的 `+4`。解析器开头的 `cmp r1,#0x2c` 是检查45字节输入帧的最后索引44，绝不是输出结构长度。技术报告已修正。

### 已恢复的原版调用链

1. `0x0DB4` literal word = `0x20000815`，是 Thumb 函数指针，指向 code offset `0x0594` 的应用 frame callback。
2. `0x0950` 入口将第三参数保存在栈 `[sp+0x14]`。
3. `0x0950` 在 DPLL/frame collector 分支调用 `0x0800`；若 collector 返回非零、回调指针非空，则以 `sp+8`（parser生成的记录指针）作为 r0 调用回调。
4. callback从 global app object 取当前记录区，将40字节复制到 object+4，更新计数/记录状态并调用显示 hook。

### parser 40-byte 输出偏移（部分确认）

| 输出偏移 | 目前直接观察到的来源/含义 |
|---|---|
| `0x00..03` | 4个车次前缀字节 |
| `0x04..07` | 24-bit 车次数字扩展到32-bit |
| `0x08..0B` | 公里标 magnitude；标志单独存 |
| `0x0C..0D` | 机车号 |
| `0x0E..0F` | 总重 |
| `0x10..11` | 计长 |
| `0x12..13` | `frame[0x28]` 与 `frame[0x28+r1]`，动态含义未确认 |
| `0x14..15` | 10-bit速度 |
| `0x16..17` | 机车型号 |
| `0x18` | 本补角色 |
| `0x19..1B` | 公里标存在/负号/方向标志 |
| `0x1C` | 辆数 |
| `0x1D..1E` | 车站号两个字节 |
| `0x1F` / `0x1F+r1` | 来自`frame[0x26]`/`frame[0x27]`，动态索引可能重叠 |
| `0x21` | `frame[0x2A]`，业务含义未确认 |
| `0x22..23` | 信号机/信号种类 |

`r1` 由车次号码通过阈值表和重复减法产生，尚未确认它如何影响 `0x12..13`、`0x1F+r1` 的业务含义；输出 `0x20` 也未见固定写入。完整记录布局不能在这些字节未查清之前硬编码。

### 本次文档提交

- `52edb367df5ec39b596a1a9def3c5b5eb7f2c274`：逆向报告纠正40-byte回调拷贝长度。
- `5643d8b5aca506ca1881ade36d18f16a0df37210`：signal-path报告纠正长度。
- `3fb919d86e86c14d3cc1b33a8036b3e9f31a2f23`：交接文档增加第五轮进度。
- 后续 parser偏移细化见本轮提交之后的最新 HEAD。
