# TDCSRX RX 0.3 — 原版机器码的部分逆向恢复

本目录针对公开归档中的 TDCSRX-cn.v5.5.6.app（FAP1，应用版本 0.3）进行静态逆向。这里保存的是从原版机器码恢复出的**部分可读 C 重建**，不是原作者发布的完整 C 源码，也不是可刷入 K6 的固件。

## 文件

- tdcsrx_recovered_protocol.c/.h：45 字节应用帧验证、32 字节 TDCS 业务负载字段提取、CRC-16/XMODEM，以及从原版指令恢复的 BCH(26,16) syndrome / 单比特纠错逻辑。
- tdcsrx_recovered_stream.c/.h：31/32 位同步窗口、正常/反相极性、最多 2 bit 同步误差、26 bit MSB-first 码字收集。
- tdcsrx_recovered_receiver.c/.h：两条独立 collector、BCH 码字解码、45-byte 应用帧组装、CRC 验证和回调。
- tdcsrx_recovered_dsp.c/.h：按 0x0950 指令恢复的第一版 9.6 ksample/s 音频滤波、1200/1800 Hz I/Q 相关器、0x0198 幅度/基线函数、两条加权判决和 DPLL。**这部分目前未通过真实录音端到端校验，不能当作已工作的解调器。**
- test_tdcsrx_recovered.c、test_tdcsrx_recovered_receiver.c：协议、BCH、同步、双 collector、完整应用帧和 CRC 的合成主机测试。
- test_tdcsrx_recovered_dsp.c：DSP 数值、状态布局和样本链诊断。最后的合成音频到整帧断言仍失败；该失败保留为诊断，不会把它删除或标成通过。
- test_tdcsrx_real_wav.c：对固定现场录音执行 9.6 kHz DSP 处理、导出两条判决位流和逐样本 CSV。
- prepare_real_wav.py、compare_real_wav_bits.py：准备真实 WAV 输入，并与独立 48 kHz FFSK 参考判决对照。
- TDCSRX_REVERSE_ENGINEERING.md、TDCSRX_RX03_SIGNAL_PATH.md：原始二进制来源、Thumb 地址、指令级 DSP 记录、帧结构和字段偏移。
- K6_BK4819_FIFO_ADAPTER.md：K6 BK4819 FFSK/FIFO 候选交接方案及实机采集前必须确认的事项。
- Makefile：分开的 protocol/frame 测试、DSP 诊断和真实 WAV 探针。

## 编译和测试

在本目录执行：

    make clean test-protocol

test-protocol 覆盖当前已单独验证的协议/帧层。make test-dsp 会运行仍未通过的 ADC 样本到完整帧诊断；它不能替代真实 RF 或实机测试。

真实录音探针由仓库工作流执行。固定录音为 rzwcl/450-TAx2 中的 2026-10-11_17-30-46_audio_457550000Hz.wav，下载后通过 Git blob SHA-1 校验再处理。录音独立参考解析位置约为 1.146593 s，CRC 为 0xB27A，车次 X319，速度 20 km/h。

## 已恢复并验证的范围

- 帧头 0x0C, 0x2B、长度字段、两种允许的头字段组合。
- 45 字节应用帧和 CRC-16/XMODEM。
- BCH(26,16) 多项式 0x05B9、syndrome 和单比特纠错。
- 31/32 位同步窗口、正常/反相极性、最多 2 bit 同步误差。
- 两条独立 collector 的帧组装和 CRC-gated callback。
- TDCS 前导码交接的主机侧假设测试：在模拟 BK4819 消耗 16 个交替位后，软件仍能在后续 35 个交替位后搜索原始 31-bit 应用同步字。

## DSP 目前的实测失败

原始 .app 的 0x0950 DSP 已有一版指令级 C 翻译。原始字节常量确认了滤波系数 0x140A、0x3246、-6125；时基换算使用 48000，采样循环间隔为 5000 个周期，对应 9.6 ksample/s。两条判决的权重和相减顺序也已按 Thumb 指令核对。

但真实录音验证并未通过：

- 现场 WAV 原始格式为 22,050 Hz、单声道、16-bit PCM，时长 1.696961 s。
- 独立 48 kHz 正交判决找到原始 31-bit sync 的位置为第 1,376 bit / 1.146667 s，与原项目分析记录的约 1.146593 s 对齐。
- 当前 C DSP 在 9.6 kHz 重采样数据上，没有输出完整有效帧。
- 两条 C 判决支路的最近 31-bit 同步字 Hamming distance 约为 7–10 bit；与独立参考位流的对齐 BER 约为 0.32–0.40。
- 简单 8-sample 正交能量对照在该 WAV 上约有 88.3% 的 bit agreement，但这不是原版 DSP，也远不足以替代同步容错、BCH/CRC 和真实 K6 验证。

因此，协议/帧层主机测试与 ADC sample-to-bit DSP 是两个不同成熟度的部分。**不能说 TDCSRX 软解调已完成，不能声称当前代码可接收 RF，也不能将它刷入 K6。** 下一步应继续逐条比对滤波后数据、I/Q 累积、基线、差分符号和 DPLL 事件；不要只调振幅把测试调绿。

## K6 移植状态

- 目标硬件：DP32G030 + BK4819。
- 原版 TDCSRX 使用 MCU ADC 软件音频采样；K6 的 PA4 与键盘矩阵冲突，不能照搬该硬件入口。
- 已记录 BK4819 FFSK 模式和 FIFO 的候选交接策略，但尚未取得用户 K6 的原始 REG_02/REG_0B/REG_5D/REG_5E/REG_5F 捕获。
- FIFO 字节序、bit order、硬件 sync 是否剥离、极性是否规范化、83-byte 候选帧长、连续接收和重捕获都仍待受控信号源/实机确认。
- 没有生产 tdcsrx_bk4819.c，没有完整 LOSEHU K6 固件，也没有可刷入的 TDCSRX-K6 BIN。

本目录仍是逆向研究和主机验证材料；不包括 Bootloader 修改，也不包含任何发射功能。
