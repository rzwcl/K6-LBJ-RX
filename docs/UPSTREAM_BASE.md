# LOSEHU 0.13.2 K6 固件基线与硬件接口

本文只记录完整 K6 固件移植需要依赖的上游基线、Bootloader 约束和 DP32G030/BK4819 接口。它不代表 LBJRX 或 TDCSRX 已移植完成。

## 固定上游版本

- 仓库：https://github.com/losehu/uv-k5-firmware-custom
- 标签：`0.13.2`
- commit：`021656ea13226d349e31acd6eb21079c5bab64c7`
- 不使用滚动 `main` 替代固定标签；基线升级必须先做差异审查。

## 用户提供的 LOSEHU132HS 镜像

仓库此前保存的 `LOSEHU132HS(MDC尾音).bin`：
- 大小 61,458 bytes。
- 作者 0.13.2 的打包格式在 raw firmware 中间插入 16-byte 版本字段、尾部附加 2-byte CRC；打包文件比 raw 多 18 bytes。
- 按该格式解包后 raw 区长度为 61,440 bytes（60 KiB），版本字段为 `*OEFW-LOSEHU`，CRC-16/XMODEM 为 `0xEE94`，与文件保存值一致。
- 这证明该文件符合作者 0.13.2 的打包格式且传输完整；文件名包含“MDC尾音”，不能单凭文件名或版本字段证明它与公开 tag 的机器码完全一致。

该镜像只作为用户当前固件/EEPROM 结构研究的参考，不是本项目的代码来源，也不会被覆盖或改写。

## 现有多固件 Bootloader

用户现有引导器：[losehu/uv-k5-bootloader-custom](https://github.com/losehu/uv-k5-bootloader-custom)，Release `0.02`。

已核对的布局要点：
- 使用 512 KiB EEPROM 保存二级引导、固件目录和固件镜像。
- 固件目录从 EEPROM `0x40000` 一带开始，最多 64 项，每项 32 bytes。
- 二级引导镜像从 `0x41000` 读取；固件镜像数据区由目录条目给出。
- 选择条目后，二级引导把所选固件复制到 MCU Flash 应用区执行。
- 单个固件 raw image 不能超过 60 KiB。

因此最终方案是两份独立完整 K6 镜像、两个 EEPROM 固件目录项。不得修改基础 Bootloader、`L_BL002` 或覆盖用户已有日常/救援固件。

## DP32G030 引脚与 PA4 约束

固定 LOSEHU 0.13.2 中：
- `driver/gpio.h` 定义 `GPIOA_PIN_KEYBOARD_1 = 4`。
- `driver/keyboard.c` 将这个 pin 用作数字键盘矩阵列输入。
- `hardware/dp32g030/portcon.def` 中，PA4 可选 GPIO、CMP0_VP、XTAH_XO；没有 SARADC 复用项。SARADC 通道从 PA5/PA6 等其他引脚开始。

因此原版 FAP1 APP 在 PA4 上进行 MCU ADC 采样的代码不能直接复制到 K6。不要把 PA4 临时改成 ADC 后导致键盘功能冲突，也不要假设把采样变量改名就能解决引脚冲突。

## BK4819 FSK/FFSK 硬件能力（固定上游 0.13.2）

0.13.2 的 `driver/bk4819.c` 已包含两类参考配置：

### 直 FSK 1.2K

`BK4819_SetupAircopy()` 使用 `REG_58 = 0x00C1`，配置 FSK 1.2K RX/带宽和 FSK enable；同时配置 `REG_72` 的 1200 速率控制字。该函数用于 Aircopy，不能不加审查地直接作为 LBJ 配置，因为它还设置了 Aircopy 的 CRC 和包长参数。

### FFSK 1200/1800

`enable_msg_rx(true)` 中的 `REG_58` 位字段为：
- TX mode `001`：FFSK 1200/1800 TX。
- RX mode `111`：FFSK 1200/1800 RX。
- RX gain `11`。
- RX bandwidth `001`。
- FSK enable = 1。

合成值为 `0x3F03`。该函数还设置 `REG_70`、`REG_72`、同步字 `REG_5A/REG_5B`、CRC 开关 `REG_5C`、FIFO 阈值 `REG_5E`、数据长度 `REG_5D`，然后清 FIFO、开启接收。它是 FFSK 的硬件参考，不应直接用于 TDCSRX，因为同步字、长度、CRC 和 polarity 要按 TDCSRX 原始协议重新配置。

### FIFO 与中断

- `REG_59[12]`：FSK RX enable；`REG_59[14]`：清 RX FIFO；`REG_59[10]`：RX 数据反相；`REG_59[3]`：同步字长度选择（2 或 4 字节）。
- `REG_5A/REG_5B`：同步字节 0–3。
- `REG_5D`：FSK data length，11 bits，最大 2,048 bytes；文档示例值 0x0F 表示 16 bytes。
- `REG_5E[2:0]`：RX FIFO almost-full threshold；RX FIFO 总容量 8 words。
- `REG_5F`：读取一个 16-bit FIFO word。
- `REG_3F[1]` / `REG_02[1]`：FSK RX sync 中断使能/状态。
- `REG_3F[12]` / `REG_02[12]`：RX FIFO almost-full 中断使能/状态；`REG_3F[13]` / `REG_02[13]`：RX finished。
- 固定上游 `app/messenger.c` 按 FIFO word 低字节先、高字节后读取；MDC1200 路径会参考 `REG_0B[7]` 的负极性标志对 word 做 XOR。K6 适配必须根据目标协议单独确认 FIFO bit order 和极性处理。

## 两套接收器的候选硬件路径

| 固件 | 目标射频/协议 | 候选 BK4819 接收模式 | 软件层必须保留 |
|---|---|---|---|
| LBJRX | 821.2375 MHz；POCSAG 1200 baud | 直 FSK 1.2K | 原版 POCSAG 同步、BCH/parity、batch、RIC/消息处理 |
| TDCSRX | 457.550 MHz；FFSK 1200/1800 | FFSK 1200/1800 RX | 原版同步容错、26-bit BCH、45-byte frame CRC/字段解析 |

是否让 BK4819 硬件直接匹配协议的完整 4-byte sync，还是先从交替 preamble 开始读 FIFO、再由软件找到协议 sync，必须根据 BK4819 同步行为和真实 FIFO 捕获验证后决定。不能先假设硬件会保留或剥离 sync 的固定方式。

## 当前验证边界

- 已在固定 0.13.2 源码中确认直 FSK/FFSK 配置函数、FIFO 访问、GPIO/键盘 pin 定义。
- 已通过 BK4819v3 寄存器表确认 FSK/FFSK mode 字段、FIFO 大小、阈值和 11-bit 数据长度。
- 目前没有改动 LOSEHU 底层驱动，没有建立 K6 LBJRX/TDCSRX 完整固件，也没有 K6 实机射频捕获验证。
- 后续必须在两个独立固件目标中分别实现只接收适配，避免与 Messenger/MDC1200/Aircopy 争用同一个 BK4819 FIFO。
