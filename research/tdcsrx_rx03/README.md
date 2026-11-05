# TDCSRX RX 0.3 — 部分逆向恢复源码

本目录收录从公开的 `TDCSRX-cn.v5.5.6.app`（FAP1，应用版本 0.3）机器码中恢复出的**应用帧解析层**，不是原作者的完整 C 源码。

## 文件

- `tdcsrx_recovered_protocol.c/.h`：CRC-16/XMODEM、45 字节帧验证、32 字节 TDCS payload 字段提取，以及原版 BCH(26,16) 余数运算与单比特纠错的可读重建。
- `test_tdcsrx_recovered.c`：只使用合成帧，不依赖用户录音；覆盖两种帧头、全部已恢复字段、CRC/帧头/长度拒绝及 BCH syndrome 对照。
- `TDCSRX_REVERSE_ENGINEERING.md`：二进制来源、FAP1/API 信息、接收执行模型、寄存器配置、帧结构、字段偏移、BCH 实际常量和采样解调链分析。
- `Makefile`：本目录独立测试入口。

## 编译测试

在本目录执行：

```sh
make clean test
```

也可以直接执行：

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 \
  tdcsrx_recovered_protocol.c test_tdcsrx_recovered.c \
  -o test_tdcsrx_recovered
./test_tdcsrx_recovered
```

## 已恢复与未恢复边界

已恢复：45 字节应用帧的直接校验条件、两种允许的头部组合、32 字节负载字段偏移、CRC-16/XMODEM，以及二进制中的 BCH 多项式运算字面值与 syndrome 查表行为。

尚未恢复为可编译 C 模块：`0x950` 音频 DSP、完整的 9600 Hz 采样与 26-bit 字码流同步/组帧代码、完整 UI/按键/设置逻辑。当前代码不能单独接收射频，也不能直接编进 K6 固件。

## BCH 已确认

目标机器码在代码偏移 `0x1DC` 的 LDR 读取代码偏移 `0x200` 的常量池。
四字节 `B9 05 00 00` 的完整 little-endian 值是 `0x000005B9`。
原版纠错使用 `0x05B9`，26 个单比特 syndrome 全部唯一。测试覆盖全部
26 个单比特错误位置并验证可纠正回原 16-bit 数据。

## K6 项目接续规则

此目录属于研究资料，不是 `FirmwareOverlay`。不要把早期 `k6-full-firmware` 分支中 `TDCS_PAYLOAD_SIZE=40` 的实验代码直接并入 `main`：该负载长度与这里从 TDCSRX 0.3 帧解析器恢复出的 32 字节负载不一致。先完成原始接收链 DSP/组帧逆向，再针对 K6 的 DP32G030 + BK4819 改写驱动与 UI。

## 同步与码字收集

同步器和码字收集已经从原版 `0xA80` 重建为 `tdcsrx_recovered_stream.c/.h`。
它计算正常/反相的 31/32 位四种 Hamming 距离，容许最多 2 位错误；锁定后按极性处理，26 bit MSB-first 形成一个 BCH(26,16) 字。
合成测试覆盖正常/反相同步、两位同步误码容错、三位误码拒绝，以及单比特纠错后的 16-bit 字恢复。
完整帧计数/结束/回调时序和音频 DSP 的 C 重建仍未完成。
