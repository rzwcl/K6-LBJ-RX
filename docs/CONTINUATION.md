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

## 现有 LOSEHU 多固件 Bootloader 结构（已核对源码）

项目：`https://github.com/losehu/uv-k5-bootloader-custom`
- Release：`0.02`；当前仓库 `main` 的 `bl.json` 标记最新二级引导文件为 `L_BL002.bin`。
- 仓库 README 说明要求 4Mib（512 KiB）EEPROM；每个被管理的固件镜像不能超过 60 KiB。
- 固件数量字节位于 EEPROM `0x40000`；代码检查最大 64 个条目。
- 固件目录从 `0x40020` 起，每个条目 32 bytes；名称和固件数据起止地址均由 Bootloader 读取。
- 基础引导程序 `LOSEHU_BASE_BOOT` 位于 MCU Flash。它会从 EEPROM `0x41000` 读取 12 KiB 的 `L_BL002.bin` 到 RAM `0x20000800`，然后跳转执行。
- 二级 `L_BL002` 显示 EEPROM 中的固件目录；选择后读取对应固件数据，并将所选镜像写入 MCU 应用 Flash 区，再复位运行。
- 因此日常、救援和 LBJ 都是同一 Bootloader 管理的 EEPROM 固件条目；不是多个固件同时占据 MCU Flash 的独立运行槽。

**对 LBJ 的含义：** 我们刚构建的 53,476-byte raw 镜像小于 60 KiB 限制。最终安装时应通过你正在使用的 Bootloader/K5WEB 既有流程新增一个 LBJ 固件条目和镜像数据，不能把“当前编译未写 EEPROM”误解成“安装 LBJ 不需要写 EEPROM”。现阶段没有修改设备上的 EEPROM、基础 Bootloader 或 `L_BL002`；这些变更只发生在实际添加固件条目时。

## 已确认的基线

- UI 基准：rev3。常规统一字体；车次、方向、速度在第一行；公里标独占一行，显示 K201 形式；型和线放在下方；不显示 A/B 端位。
- 目标设备：老款泉盛 K6，DP32G030 + BK4819。
- LBJ 接收频率：821.2375 MHz；LOSEHU 频率单位按 10 Hz，运行时值 82123750。
- 当前候选 RF 路径：BK4819 直 FSK 1200 + FIFO → MCU POCSAG 软件解码。
- 历史记录暂定 10 条，仅 RAM。
- LBJ 的目标是作为一个新固件条目加入你现有的 LOSEHU 多固件 Bootloader EEPROM 固件目录；保留已有日常固件、救援固件和 Bootloader 条目。这里的“独立”指独立固件镜像/目录项，不是同时存在于 MCU Flash 的多个执行槽。

## 用户补充上传的当前固件镜像

仓库根目录新增：LOSEHU132HS(MDC尾音).bin
- GitHub blob SHA：92862b51dd340880f86dd5b4f6ffe99b5d336c6b
- 文件大小：61,458 bytes。
- 这是打包固件，不是 C 源代码工程。
- 已按作者 0.13.2 的 fw-pack.py 解包/解混淆并验证 CRC-16/XMODEM：0xEE94；raw firmware 长度 61,440 bytes，恰好 60 KiB。
- 版本字段：*OEFW-LOSEHU；初始 MSP：0x20003FF0；复位向量：0x0000010B。
- 这些信息证明打包格式和镜像完整性，但不能单凭它证明 BIN 与作者 tag 的机器码完全一致；“MDC尾音”可能表示定制差异。

作者完整源码不复制进本仓库；通过固定 tag + commit 的 GitHub Actions 工作流自动拉取。这样可复现，也不会把上游滚动 main 混入。

## 作者 LOSEHU 源码已定位

- 上游：`https://github.com/losehu/uv-k5-firmware-custom`
- 固定基准：tag `0.13.2`
- commit：`021656ea13226d349e31acd6eb21079c5bab64c7`
- 选择理由：用户上传镜像文件名为 `LOSEHU132HS(MDC尾音).bin`；作者 0.13.2 Makefile 的打包前缀为 LOSEHU132，README 定义 HS 为 H+SI4732 组合。该版本的 linker script FLASH=60 KiB。
- 这属于强版本线索，不是机器码级精确来源证明；“MDC尾音”可能表示用户定制修改。

用户 BIN 为 61,458 bytes。作者 `fw-pack.py` 的格式会增加 16-byte 版本字段和 2-byte CRC；因此对应的原始 `firmware.bin` 长度为 61,440 bytes（60 KiB），正好达到 0.13.2 linker script 的 FLASH 上限。这个关系与上传文件尺寸相符，但仍需通过解包/字节比对确认实际构建来源。

新增：
- `docs/UPSTREAM_BASE.md`：上游 tag、commit、HS 配置含义、版本对应分析和构建方向。
- `.github/workflows/build-lbj.yml`：从固定 0.13.2 tag 拉取作者源码，在临时副本应用 overlay，使用 ARM GCC 做完整链接，并检查原始 BIN 不超过 60 KiB、打包长度等于 raw+18。产物只作为 Actions artifact，不把二进制混入源码提交。

LBJ 专用镜像已用该配置完整编译链接：保留 `ENABLE_CHINESE_FULL=4`，关闭拼音、SI4732、MDC1200、Messenger、Aircopy、Spectrum、Doppler 和 FM Radio。输出后缀为 `LOSEHU132K`；FLASH 53,908 bytes，RAM 5,128 bytes。它是独立接收系统，不要求保留当前 132HS 的所有附加功能。

## 当前完整构建状态

完整编译和链接：PASS

- 工作流：Build K6 LBJ RX
- 成功 run：41962799054
- 源码提交：5f7f41d7b39ddec33922c1833481896bf4a5e38a
- Artifact：K6-LBJ-RX-LOSEHU132K
- Artifact ID：13386116726
- 下载页：https://github.com/rzwcl/K6-LBJ-RX/actions/runs/41962799054/artifacts/13386116726
- packed BIN：LOSEHU132K.bin，53,494 bytes；raw firmware.bin 53,476 bytes。
- CRC-16/XMODEM 0xAEB0 通过。
- FLASH：53,476 / 61,440 bytes（87.04%）；RAM：5,124 / 16,384 bytes（31.27%）。
- linker map firmware.map 已包含在 artifact。
- 作者 0.13.2 真实源码树 dry-run、overlay 安装、二次 dry-run 幂等性检查均通过。

构建配置保留完整中文字体（ENABLE_CHINESE_FULL=4），关闭拼音、SI4732、Messenger、MDC1200、Aircopy、Spectrum、Doppler、FM Radio、DTMF calling 和 UART 命令处理。输出后缀 K 表示完整中文、无拼音输入、无 SI4732；这是 LBJ 独立系统，不是当前 132HS 的功能复刻。

编译器报告和打包报告见 docs/VERIFICATION.txt。完整 map、ELF、raw BIN、packed BIN 和 JSON 报告均在 Actions artifact 中。

## 已有验证

- 主机端 POCSAG、业务解析、应用逻辑、BK4819 FIFO mock 测试通过。
- overlay C 语法检查通过，但这部分使用本仓库 tests/stubs。
- Clang 17 为 7 个 LBJ overlay translation units 生成 ARM Cortex-M0/Thumb 对象文件。
- 作者 LOSEHU 0.13.2 真实源码树：dry-run、overlay 安装、二次 dry-run 幂等性检查通过。
- ARM GCC 10.3.1 完整编译、链接通过；生成 firmware ELF、raw firmware.bin、packed LOSEHU132K.bin 和 firmware.map。
- 链接器报告 FLASH 53,908/61,440 bytes、RAM 5,128/16,384 bytes。
- 打包输出 CRC-16/XMODEM 0xAEB0 验证通过；初始 MSP/复位向量落在预期地址范围。
- 构建产物保存在 Actions artifact；不是实机测试结果。

完整验证数据见 docs/VERIFICATION.txt。

## TDCSRX RX 0.3 逆向恢复记录（2026-11-06）

已把公开的 `troilus/lbjradioweb/archive/apps/TDCSRX-cn.v5.5.6.app` 的一部分逆向重建资料提交到本仓库：

- 研究目录：`research/tdcsrx_rx03/`
- 包含 `tdcsrx_recovered_protocol.c/.h`、`test_tdcsrx_recovered.c`、`Makefile` 和 `TDCSRX_REVERSE_ENGINEERING.md`。
- 来源 APP：FAP1，4,140 bytes，`TDCS RX 0.3`，F4HWN overlay API 3；这不是 K6 可直接运行的固件，也不是原作者完整 C 源码。
- 从原版帧解析逻辑恢复的应用帧为 45 bytes：固定字节 `0x0C 0x2B`，头部偏移 7..9 有两种允许值，偏移 10 为 32-byte payload 长度，偏移 11..42 为 payload，偏移 43..44 为大端 CRC-16/XMODEM。
- 负载字段映射以及 CRC 校验已写入 C 重建代码。合成帧覆盖两个帧头分支、全部字段、CRC/帧头/长度拒绝。

**测试边界：** 本轮使用 GCC 严格告警编译、普通测试和 AddressSanitizer/UndefinedBehaviorSanitizer 测试均通过；测试向量是合成数据，不是用户录音，也不证明 RF 实收正确。

**已修正的 BCH 结果：** 先前把字节 `B9 05` 误读为 `0xB905`；重新核对 LDR PC-relative 地址后，完整常量池位于代码偏移 `0x200`，四字节 `B9 05 00 00` 的小端值是 `0x05B9`。原版 BCH 的 26 个 syndrome 唯一，重建代码和单比特纠错测试已修正。

**新增采样链结果：** 从入口的 9,600 samples/s 主循环到 `0x950` DSP、`0xA80` 位/字同步、`0x484` BCH、`0x61C` 帧解析器的调用链已追到。DSP 系数表分别对应 8 相位 1,200 Hz 和 16 相位三次谐波 1,800 Hz 的 I/Q 检测；精确的定点状态更新、判决门限和恢复为可编译 C 模块仍需继续逆向。现有代码只是帧解析层，未接入 `FirmwareOverlay`，不能构建成 K6-TDCSRX 固件。

**同步/码字链已恢复：** 原版 `0xA80` 用四组 Hamming 距离检测 `0x0DD4259F` 正常/反相的 31/32 位窗口，容错为 2 bit；锁定后按极性反相处理，MSB-first 每 26 bit 进入 BCH(26,16)。`tdcsrx_recovered_stream.c/.h` 已提交，合成测试覆盖正常/反相同步、2 bit 容错、3 bit 拒绝和码字输出。完整应用帧末尾/回调生命周期还需核对。

**旧分支警告：** `k6-full-firmware` 分支留有早期实验版 TDCS 代码，其中 `TDCS_PAYLOAD_SIZE=40` 和对应帧布局与本次从原版 TDCSRX 0.3 解析器恢复出的 32-byte payload 不符；该分支代码不应直接合并或作为目标协议实现。26-bit 码字数量/音频接收路径同样仍需要原版 DSP 和组帧逻辑验证。

后续顺序：先完成原版 DSP 定点状态、门限和位/字同步 C 重建；然后再把确认后的接收链移植到 K6 DP32G030 + BK4819，最后才接入单独的 TDCSRX 完整固件构建。LBJ 固件现有构建产物和文件不受这次研究目录提交影响。

## 仍缺少的内容

- 本仓库不 vendoring 作者完整固件树；通过 docs/UPSTREAM_BASE.md 和 workflow 固定上游 tag/commit，工作流运行时自动 clone。
- 尚未在实体 K6 上验证 BK4819 真实 FSK 前导码/同步捕获、FIFO 字节顺序、位序、反相极性、长窗口重捕获和中断队列压力。
- 尚未实测 LCD、灵敏度、PTT/侧键/串口等硬件层面的绝对不发射。
- 尚未用你当前的 K5WEB/Bootloader 流程把 LBJ 镜像作为新条目写入 EEPROM 固件目录，也尚未验证选择后从 EEPROM 搬运到 Flash 并启动。

## 下一步工作顺序

1. 固定 0.13.2 源码的 TX 调用链静态审计已完成：中心 FUNCTION_Select、PTT/Transmit 键处理、三个 RADIO TX 入口和相关 feature flags 均有保护；下一步只剩 K6 实机 no-TX 测量，确认 BK4819 PA/发射路径没有实际 RF 输出。
2. 对 lbj_radio.c 做真实芯片 FSK 测试：前导码、sync 消耗、FIFO word/byte 顺序、MSB-first、正常/反相、2048-byte 窗口和重捕获。
3. 测量 FIFO almost-full 到主循环消费的最坏延迟，压测 128-bit queue 溢出和丢包恢复。
4. 验证 UI 实机显示、历史浏览、MENU/EXIT/UP/DOWN 按键。
5. RF、按键和 no-TX 检查通过后，按现有 Bootloader/K5WEB 支持的方式，把 LBJ BIN 作为一个新固件条目追加到 EEPROM 固件目录；保留原日常/救援镜像及现有 Bootloader 条目，不重刷基础 Bootloader 或 L_BL002。

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
