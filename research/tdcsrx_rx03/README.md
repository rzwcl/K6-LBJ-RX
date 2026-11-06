# TDCSRX RX 0.3 — 原版机器码的部分逆向恢复

本目录针对公开归档中的 `TDCSRX-cn.v5.5.6.app`（FAP1，应用版本 0.3）进行静态逆向。这里保存的是从原版机器码恢复出的**部分可读 C 重建**，不是原作者发布的完整 C 源码，也不是可刷入 K6 的固件。

## 文件

- `tdcsrx_recovered_protocol.c/.h`：45 字节应用帧验证、32 字节 TDCS 业务负载字段提取、CRC-16/XMODEM，以及从原版指令恢复的 BCH(26,16) 余数和单比特纠错逻辑。
- `tdcsrx_recovered_receiver.c/.h`：两条独立的 26-bit BCH 解码路径、45-byte 应用帧组装和 CRC 校验回调。
- `tdcsrx_recovered_stream.c/.h`：从原版同步/码字收集例程恢复的 31/32 位同步窗口、极性判断、最多 2 bit 同步误差，以及 26 bit MSB-first 码字收集。
- `test_tdcsrx_recovered.c`：仅使用合成数据的主机端测试，不依赖用户录音或 K6 硬件。
- `TDCSRX_REVERSE_ENGINEERING.md`、`TDCSRX_RX03_SIGNAL_PATH.md`：原始二进制来源、FAP1/API 信息、寄存器配置、采样/解调调用链、帧结构和字段偏移。
- `Makefile`：独立测试入口。

## 编译测试

在本目录执行：

```sh
make clean test
```

## 已恢复的范围

已由目标机器码确认：45 字节应用帧的头部和长度条件、两种允许的头字段组合、32 字节业务负载布局、CRC-16/XMODEM，以及 BCH 多项式 `0x05B9`。同步例程已恢复正常/反相的 31/32 位窗口匹配和 26 bit 码字收集；合成测试覆盖同步误码及 26 个单比特 BCH 错误位置。

## 尚未恢复的范围

- `0x950` 音频 DSP 的完整定点数学、滤波/状态更新、判决门限和定时细节。
- 采样与解调状态机的完整 C 实现，以及完整帧结束、回调和异常重同步生命周期。
- 完整 UI、按键和设置逻辑。

因此，这些文件目前只能验证已恢复的协议层和同步层，**不能独立接收射频，不能直接作为 K6 固件，也不能声称是原作者完整源码**。后续 K6 适配必须继续依据原版机器码恢复接收链，并单独验证 DP32G030/BK4819 的硬件采集接口；不再沿用已清理的实验性 `FirmwareOverlay` 解码器。


## Dual stream recovery status

The original DSP contains two independent symbol slicers and two frame-collector
states. Their weights are `0x0302` (1200:2 / 1800:3) and `0x0203`
(1200:3 / 1800:2). The new `tdcsrx_recovered_receiver.c/.h` reconstructs the
post-bit decision layer for both collector paths: rolling sync, 26-bit BCH
words, first-word `0x0C2B` validation, expected word count
`(frame[1]+3)>>1`, 45-byte frame assembly and CRC-gated callback.

The exact I/Q magnitude helper at code offset `0x0198` and the sample-to-bit
fixed-point detector remain unimplemented. This receiver module is a recovered
post-demodulation frame layer, not a complete DSP or K6 receiver.
