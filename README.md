# K6 LBJRX / TDCSRX 原版移植工作区

<badge color="warning">当前没有可刷入的 K6 LBJRX / TDCSRX 固件</badge>

本仓库的目标是把公开的 LBJRX、TDCSRX 原版实现分别适配到老款泉盛 K6（DP32G030 + BK4819），最终得到**两份互相独立的完整 K6 固件镜像**，通过用户现有的 LOSEHU 多固件 Bootloader 作为两个独立目录项切换。不会把两个接收器混成一个程序，也不改基础 Bootloader 或 `L_BL002`。

**之前基于 `K6-LBJ-RX` 实验解码器生成的 BIN 不属于原版移植，存在实现错误，不能刷入。** 对应的实验解码器、专属构建流程和二进制已从当前相关分支清理；旧提交仍保留在 Git 历史中，没有改写历史。

## 当前确认的原版资料

### LBJRX

公开归档：[LBJRX-cn.v5.5.6.app](https://github.com/troilus/lbjradioweb/tree/main/archive/apps)。

同名源码候选来自 [troilus/k1lbj](https://github.com/troilus/k1lbj/tree/main/App/apps/lbj)：
- 构建脚本标记应用版本 `0.4`、API 最低版本 `2`，与公开归档包的应用版本/API 匹配。
- 源文件已经原样保存到 [research/original_apps/LBJRX-RX04](research/original_apps/LBJRX-RX04/)。
- 当前还没有用原作者工具链重建并比较 FAP1 代码 CRC，因此版本对应已确认，逐字节构建对应尚未确认。
- 原版 `lbj_app.c` 在 PA4 走 MCU ADC 音频采样；可复用的 POCSAG 解码入口是 `lbj_rx_bit()`，K6 硬件采集部分必须替换。

### TDCSRX

公开归档：[TDCSRX-cn.v5.5.6.app](https://github.com/troilus/lbjradioweb/tree/main/archive/apps)。

- 发布包为 TDCS RX 0.3，FAP1，代码区 4,076 B，API 最低版本 3。
- 目前没有找到匹配的原作者 C/H 源码。
- [research/tdcsrx_rx03](research/tdcsrx_rx03/) 保存了从这份确切版本机器码恢复的 C 实现、主机协议/帧测试、Thumb 反汇编和分析报告。
- 2026-11-07 已用 Unicorn 直接执行原始 FAP1 的 Thumb `0x0950` DSP，并对固定 457.550 MHz WAV 的 16,291 个 9.6 kHz ADC 样本逐样本对照 C 版本：滤波输出、I/Q 累积、平滑基线、相位、判决差值等所有导出状态字段均 0 差异；两条 collector 位流也分别逐位 0 差异（1,988/1,988 与 1,865/1,865）。这确认 C DSP 翻译与 RX 0.3 原始机器码一致。
- 原始默认权重 2:3 / 3:2 在这段声卡 WAV 中仍不能得到有效帧；仅在诊断副本中把权重临时改成 1:1，才可解出 X319 / 20 km/h / CRC B27A。1:1 不是原版配置，不能据此改变生产默认值。
- 仍未完成原版 40-byte 应用记录 ABI 全映射、BK4819 FIFO 实机格式确认、K6 完整固件和无线实测。不能把 DSP 指令级匹配称为可刷入的 K6 接收器。

## K6 接收前端

原版音频采样不能原样移植：

- LBJRX 0.4 采样 PA4，19.2 ksample/s。
- TDCSRX 0.3 采样 PA4，9.6 ksample/s，并用软件做 1200/1800 Hz 双音判决。
- 在 LOSEHU 0.13.2 的 K6/DP32G030 映射中，PA4 是键盘列输入，不是可直接复用的 ADC 输入。

K6 的 BK4819 本身有硬件解调与 FIFO：
- **LBJ**：目标是 821.2375 MHz、POCSAG 1200 baud，研究使用 BK4819 直 FSK 1.2K。
- **TDCS**：目标是 457.550 MHz、FFSK 1200/1800，研究使用 BK4819 FFSK 1200/1800。
- 两者都应保留原版软件层的同步、纠错和业务解析；只替换不兼容的采样/解调入口。
- FIFO 位序、同步字剥离、极性和重新捕获行为仍需用信号源与 K6 实机验证，现阶段没有任何实机验证结果。

详细寄存器与引脚依据见 [docs/UPSTREAM_BASE.md](docs/UPSTREAM_BASE.md) 和 [TDCSRX 信号路径报告](research/tdcsrx_rx03/TDCSRX_RX03_SIGNAL_PATH.md)。

## 保留的显示与操作要求

以下是用户已经确认的 UI 规格，后续移植必须保留；它们不是当前已经实现的固件功能。

| 页面 | 内容 |
|---|---|
| 开机标识 | `LBJ RX 821.2375`，约 1.5 秒后进入主页面；频率不常驻显示 |
| 主页面第一行 | 车次 + 上行/下行/待定 + 速度；方向夹在车次与速度之间，不加“方向”前缀，速度不加“速度”前缀 |
| 主页面第二行 | 公里标独占一行，例如 `K201`；显示整数公里，内部保留小数用于方向判断 |
| 主页面底部 | 车型/机车号和线路尽量靠下排列 |
| 补充信息页 | 新版车次类型、原始经纬度数字；不显示 A/B 端位 |
| 无数据 | 显示等待报文状态 |

主页面字体统一使用常规小字体，车次、速度、公里标、型号和线路保持同一字号风格。按键：UP 查看较新的历史，DOWN 查看较旧历史，MENU 切换主页面/补充页，EXIT 返回普通电台主界面。不新增长按功能，不占用现有多系统引导器按键。历史记录暂定 10 条，仅放 RAM，不写 EEPROM。

## K6 基线与现有 Bootloader

- 固件底座参考 [losehu/uv-k5-firmware-custom](https://github.com/losehu/uv-k5-firmware-custom) 的 `0.13.2` 标签，commit `021656ea13226d349e31acd6eb21079c5bab64c7`。
- 现有多固件引导器：[losehu/uv-k5-bootloader-custom](https://github.com/losehu/uv-k5-bootloader-custom)，Release `0.02`。每个完整固件镜像受 60 KiB 限制；安装方式是在 EEPROM 固件目录中增加镜像和条目。
- 最终应生成两份分别编译、分别验证的完整固件；保留原有日常/救援固件，不另造 Bootloader，不把 FAP1 APP 误当作 K6 完整固件。

用户上传的 `LOSEHU132HS(MDC尾音).bin` 已按作者 0.13.2 的打包格式做过只读解包和 CRC 校验；它是参考基线，不表示用户机器码与公开 tag 已逐字节确认一致。

## 仓库内容与下一步

- `research/original_apps/LBJRX-RX04/`：LBJRX 0.4 源码参考副本，源文件与上游 blob SHA 一致，未修改。
- `research/tdcsrx_rx03/`：TDCSRX 0.3 部分逆向恢复、测试和报告。
- `docs/CONTINUATION.md`：当前任务状态、清理记录和下一步交接。
- `docs/UPSTREAM_BASE.md`：LOSEHU 0.13.2 基线、DP32G030 引脚与 BK4819 FSK/FFSK 能力。

下一阶段：确认 LBJRX 源码与归档二进制的可复现对应；继续补全 TDCSRX 原版 DSP/帧组装；分别实现只接收的 BK4819 FIFO 前端；通过主机测试和 K6 实机信号源测试后，再建立两份独立完整固件构建。每个可验证阶段都会单独提交 GitHub，并记录提交号、构建结果和未验证边界。
