# TDCSRX RX 0.3 — 真实 457.550 MHz WAV DSP 诊断（2026-11-07）


> **后续状态更正（2026-11-07）：** 本报告早期的“C DSP 尚未匹配原始机器码”结论已被直接 Thumb 执行试验取代。使用 SHA-1 为 `af2142508e75b9ef2d2d2c4963be33ace5ee8523` 的原始 FAP1，Thumb 模拟器执行 `0x0950` 后，C 与原始二进制在 16,291 个采样点的全部导出状态字段均为零差异；两条 collector 位流也逐位完全一致（1,988/1,988 与 1,865/1,865，mismatches=0）。当前真实 WAV 的失败边界是原始固定权重 2:3 / 3:2 不适配这段外部声卡录音，而不是 C 译码器翻译错误。把诊断副本临时改成 1:1 可以解出 X319 / 20 km/h / CRC B27A，但 1:1 不是原版默认值，不应写入生产固件。完整证据：[TDCSRX_THUMB_VALIDATION_2026-11-07.md](TDCSRX_THUMB_VALIDATION_2026-11-07.md)，[Actions run 42354887689](https://github.com/rzwcl/K6-LBJ-RX/actions/runs/42354887689)。

## 输入样本与独立基准

样本来自另一个现有项目仓库：

- 仓库：rzwcl/450-TAx2
- 文件：2026-10-11_17-30-46_audio_457550000Hz.wav
- Git blob SHA-1：8fb409ef803a98c17834fa230410c4579b030a83
- 原始格式：22,050 Hz、mono、signed 16-bit PCM
- 原始采样点：37,418
- 时长：1.696961 s

GitHub Actions 在下载后验证 Git blob SHA-1，防止样本发生变化或取错文件。

独立参考解调器使用 48 kHz、40 samples/bit 的双正交投影。它在：

- bit index：1,376
- 时间：1.146667 s
- 录音项目先前分析时间：约 1.146593 s

找到完整的 31-bit TDCS frame sync。原录音既有 BCH(26,16) 与 CRC-16/XMODEM 通过的记录，CRC 为 0xB27A；对应业务候选为 X319、20 km/h。该参考结果用来定位 C DSP 的位流偏差，不把它当成原版 TDCSRX DSP 已通过的证明。

## 9.6 kHz ADC 输入准备

样本被带限重采样到 9,600 Hz，得到 16,291 个 ADC 样本。重采样后去 DC，再映射到 unsigned 12-bit、中心值 2048 的样本范围。测试对 ADC 交流分量分别乘以 0.50、0.75、1.00、1.25、1.50、2.00，检查不同增益能否恢复。

SoX 直接重采样对照得到近似的 9.6 kHz 波形。独立 8-sample、1200/1800 Hz 正交能量判决不只给出约 88.3% 的 bit agreement；它还能在 9.6 kHz 样本中找到完整帧并通过原样 BCH/CRC 检查（run 42345120801、42348376324、42348715488 均有记录）：

- 采样相位 0：sync bit 259；
- BCH 纠错数 0，23 个码字均可解；
- 帧头检查通过、padding=0；
- CRC-16/XMODEM：收到 B27A / 计算 B27A；
- 解析字段与独立记录一致：X319、20 km/h。

这证明该 WAV 的音频含有足以恢复有效 TDCS 帧的信息，也验证了协议/BCH/CRC 链；但这个简单 detector 是**独立参考算法，不是原版 TDCSRX 0.3 DSP**，不能替代原版 DSP 的恢复。

## 当前 C DSP 的实测结果

对应 Actions：

- 协议/帧测试成功：42344805431
- 恢复源码包成功：42344805428
- 真实 WAV DSP 探针失败（失败是预期保留的诊断结果）：42344820815

真实 WAV DSP 测试结果：

| 项目 | Path A（权重 2:3） | Path B（权重 3:2） |
|---|---:|---:|
| 输出符号数 | 1,988（增益 1.00） | 1,865（增益 1.00） |
| 输出 bit=1 数量 | 273 | 1,352 |
| 参考 sync 最近 Hamming distance | 9 bit | 7 bit |
| 最佳对齐 BER | 约 0.3217 | 约 0.4038 |

对所有测试增益：

- Path A 最近 sync Hamming distance：8–10 bits
- Path B 最近 sync Hamming distance：7 bits
- 对齐 BER：约 0.32–0.40
- 完整 frame callback：0
- X319 / 20 km/h / CRC 0xB27A：没有任何一条 C DSP 路径解出

因此当前 sample-to-bit C 翻译不匹配真实 WAV 的独立参考位流，不能称作已工作的软解调器。简单正交能量判决表现较好，说明 WAV 中两种音调仍可区分；但这并不能证明原始双判决/DPLL 的 C 翻译正确，也不能将简单判决替代原版 DSP 后直接集成。

## 指令复核与已修正错误

在本轮继续核对 0x0950 Thumb 代码时，确认：

- 滤波器乘法常量为 0x140A、0x3246、0xFFFFE813（signed -6125）。
- 两条判决的实际减法次序是：
  mag1200 * smooth1800 * weight1200 - mag1800 * smooth1200 * weight1800。
- baseline helper 在上升分支走 ASRS #5；下降分支由 0x01D2 跳到 0x01C2，**绕过**上升分支的 ASRS #5，实际下降公式为 baseline - (baseline >> 10)。
- 0x0950 的样本输入为 12-bit ADC。循环间隔为 0x1388 = 5000 个时间基准周期；时间换算常量为 48000，因此采样率为 9,600 samples/s。

早期 C 版本曾把判决减法方向写反，后续又曾把下降支路误写成 >>15；这两处都已修回，并在代码/数值测试和逆向文档中更正。它们修正后真实 WAV 探针仍失败，所以不能把这两项修正当作 DSP 已完成。

## 当前诊断工件

GitHub Actions run 42344201505 上传了以下诊断工件（artifact）：

- 原始 sample.wav
- 9.6 kHz ADC 样本
- 48 kHz 参考音频和 reference bitstream
- Path A / Path B 在各增益下导出的判决位流
- 逐样本 CSV：filter、I/Q 累积、平滑基线、判决差分和 DPLL phase
- 本次 DSP 探针使用的源码及比较脚本

最新工件名称：tdcsrx-real-wav-diagnostics-42344820815

## 后续定位顺序

1. 以逐样本 CSV 对齐真实信号 sync 时间附近的样本，逐字段复算 ARM 32-bit wrap / ASR，确认 C trace 和指令级计算是否一致。
2. 单独检查滑动历史区 +0x2E..+0x6D 的 ring indexing、两个 tone table 的地址和每个 STRH/LDRSH 的配对；不能只看最终位流调参。
3. 分开验证原始 slicer 的 sample-level sign 与 DPLL event timing。当前 bit count 显示两个分支输出数量差异明显，须确定是输入 slicer 的 false transitions 还是 phase recovery 更新。
4. 若要将简单 8-sample 正交检测用作替代方案，必须作为明确标注的非原版备用路径单独评估，不能冒充 TDCSRX 0.3 原始 DSP。
5. 只有真实录音端到端通过、并完成 BK4819 FIFO 的位序/极性/同步剥离实测后，才进入完整 K6 固件集成。

**当前结论：协议/帧层主机测试可通过；DSP sample-to-bit 未通过真实 WAV；BK4819 FIFO/实机接收未验证；没有可刷入的 TDCSRX-K6 固件。**


## 2026-11-07 后续 synthetic DSP 检查

为了区分判决门限不足与更深层的时钟/相关器错误，受控合成音频 fixture 已依次测试 1200:1800 输入振幅比 1000:650、1000:400、1200:300、1200:150（最后一组为 8:1）。这些变化没有让原版 C DSP 的样本到帧链通过，因此现在停止继续靠放大某一音调来“调绿”测试。

最新可复现记录：
- commit `8a63b4e89d6370cc5fc28244120724948104bbfd`；
- protocol/frame workflow run `42348715484`：success；
- recovered-source package run `42348715565`：success；
- synthetic/real-WAV DSP probe run `42348715488`：failure。

8:1 fixture 的 DSP 诊断：
- 输入应产生 694 个逻辑位；两条 lane 实际各产生约 831 / 825 个输出符号；
- 在交替前导码早期，bit=0 时归一化差分仍为 A=-9022、B=+2562；bit=1 时为 A=-57474、B=+476429。即此时 Path A 保持 0、Path B 保持 1，没有形成所需的互补数据位；
- 没有完整帧回调，test_full_9600_hz_audio_to_frame 断言失败；
- 同一 run 的独立 9.6 kHz quadrature reference detector 仍解出 CRC B27A 的 X319 帧。

因此当前最明确的故障边界是：

1. 45-byte frame parser、BCH、CRC、两条 collector 与预期 residual-preamble handoff 已分别通过主机测试；
2. 独立的简化 9.6 kHz detector 可以从 WAV 解出正确帧；
3. 逆向恢复的原版双路相关器/基线交叉归一化/DPLL C 链仍不能从同一数据正确输出位流。

下一步不是再扫音量，而是建立逐指令参考模型，对原二进制的 ring history 读写、I/Q 值、helper输入/输出、两个 baseline、两路 diff、previous sign、phase correction 和 frame collector 调用逐采样比较。没有比对证据前不应改生产 DSP 公式。

## 后续验证：C DSP 与原始 Thumb 代码完全匹配

Actions run 42354887689 直接运行原始 FAP1 Thumb 样本 DSP，并与 C 翻译逐项比较：
- 16,291 个 ADC 样本，14 个导出状态字段 mismatch=0，所有最大绝对差值均为0；
- lane A：Thumb 1,988 bits / C 1,988 bits / mismatches=0；
- lane B：Thumb 1,865 bits / C 1,865 bits / mismatches=0。

真实 WAV 的原始权重仍然没有有效帧；诊断 1:1 权重两路均得到 BER=0.0000 的对齐位流并解出 CRC B27A。原始权重不变，外部声卡的幅度响应问题需用原生 K6 ADC 或受控信号源确定。