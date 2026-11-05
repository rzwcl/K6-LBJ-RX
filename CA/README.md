# K6 固件与恢复源码

本目录集中保存 K6-LBJRX / K6-TDCSRX 的编译产物，以及 TDCSRX RX03 的部分恢复源码。

## 编译固件

| 文件 | 用途 | 打包大小 | CRC16-XMODEM | SHA-256 |
|---|---|---:|---|---|
| [K6-LBJ-RX-LOSEHU132K.bin](Firmware/K6-LBJ-RX-LOSEHU132K.bin) | K6 LBJ 接收固件 | 53,494 B | AEB0 | `4e4cb7c5da9665c9e5fc6741fa1eb36d5bfd3e5a37f83e6be8720b7686ea0481` |
| [K6-TDCS-RX-LOSEHU132K.bin](Firmware/K6-TDCS-RX-LOSEHU132K.bin) | K6 TDCS 接收固件（修正版） | 48,918 B | 3824 | `bd6a22f86ee3eabf8a9d24307f6817c786f59c126ae13dbeea8611b5a6345fb2` |

两份固件均基于 LOSEHU 0.13.2（上游固定提交 `021656ea13226d349e31acd6eb21079c5bab64c7`）构建，打包尺寸均低于 60 KiB 限制。

**注意：** 两份固件已通过 CI 编译、链接、尺寸和封装检查，但尚未完成 K6 实机启动及射频接收验证。TDCSRX 目录中的源码是根据公开 FAP1 应用逆向恢复的部分实现，并非原作者完整 C 源码。

## 恢复源码

见 [Source/TDCSRX-RX03](Source/TDCSRX-RX03/)：

- TDCS 45 字节应用帧解析及 32 字节业务载荷字段布局
- CRC16/XMODEM
- BCH(26,16) 单比特纠错
- 同步字匹配、码字收集及流处理
- 逆向分析记录、信号路径说明和主机侧测试

尚未完整恢复原应用的音频 DSP 定点运算、全部帧组装回调生命周期，以及完整 UI/按键逻辑。

## 后续构建

LBJRX 和 TDCSRX 的 GitHub Actions 构建工作流会在成功构建后，自动将各自最新打包 BIN 发布到本目录的 `Firmware/`，同名文件会被新构建覆盖。构建记录仍保留在 GitHub Actions 中。
