# K6 原生 Overlay APP 移植记录

状态：方案与镜像工具第一阶段；**尚未实现固件侧 APP 加载器，也未进行 K6 实机测试**。

## 目标

为 DP32G030 + BK4819 的 K6 主固件增加独立应用机制：

- 一套主固件中可以同时安装 LBJRX-K6、TDCSRX-K6 等多个 APP。
- APP 分别占用独立槽位；不会把一个 APP 当成另一份完整固件。
- 通过 F+7 打开 APP 列表，选择一个启动；同一时刻运行一个 APP。
- APP 退出后回到原主界面。
- APP 有自己的 K6 API/ABI，不直接依赖主固件内部符号。
- 原有 LOSEHU 多固件 Bootloader、固件目录和其他固件镜像不作为本项目的改造对象。

## 已核实的原版机制

原版 LBJRADIO/F4HWN APP 属于 RAM Overlay：

- .app 文件使用 FAP1 容器，头部为 64 字节。
- 每个槽位 8 KiB；代码段在槽位偏移 0x1000，资源放在头部所在的 4 KiB 区域。
- 固件从外部 PY25Q16 SPI Flash 读取 APP，将代码加载到固定 SRAM 地址运行。
- 主固件提供函数指针 API；APP 入口为 app_main(const app_api_t *api)。
- APP 通过 F+7 菜单启动，安装/擦除/验证通过 0x073x 串口命令完成。

归档文件实测：

| 项目 | LBJRX-cn.v5.5.6.app | TDCSRX-cn.v5.5.6.app |
|---|---:|---:|
| 文件大小 | 5,954 B | 4,140 B |
| 代码大小 | 3,928 B | 4,076 B |
| 资源大小 | 1,962 B | 0 B |
| APP API 最低版本 | 2 | 3 |
| 链接地址 | 0x20000280 | 0x20000280 |

原版二进制依赖 F4HWN 的 API 表、PY32F071 固件布局以及原版固定链接地址，**不能直接当作 K6 APP 执行**。本项目会做 K6 专用 ABI，而不是伪装成原版 FAP1 兼容格式。

## K6 侧硬件约束与规划

### EEPROM

K6 目标机使用 512 KiB I²C EEPROM。LOSEHU 多固件 Bootloader 的布局为：

- 0x00000–0x3FFFF：固件数据/保留区域。
- 0x40000–0x40FFF：固件目录与启动信息。
- 0x41000–0x43FFF：第二级 Bootloader。
- 0x44000–0x7FFFF：多固件镜像数据区。

LOSEHU 标准 H/HS EEPROM 布局中，拼音索引/汉字表到 0x2A330 结束，SI4732 配置/patch 从 0x3C210 开始。因此当前规划的候选 APP 区为：

- APP 区候选范围：0x2A400–0x3A400
- 槽位大小：0x2000（8 KiB）
- 槽位数量：8
- 槽位地址：0x2A400 + slot * 0x2000

此候选范围不与标准拼音表、SI4732 patch、Bootloader 目录或 Bootloader 固件镜像区重叠。**它仍需与目标机 EEPROM 备份和实际启用的自定义数据区核对后才能启用写入；在此之前，固件侧代码不得向该区域写数据。**

### SRAM

现有 LOSEHU132K LBJ 构建的链接映射显示：

- SRAM：0x20000000–0x20003FFF（16 KiB）
- .data/.bss 结束：0x20001384
- 栈顶：0x20004000

当前规划的 APP overlay SRAM 区：

- 起始：0x20001800
- 大小：0x1800（6 KiB）
- 结束：0x20003000
- 留给主栈的上部空间：0x1000（4 KiB）

这样从当前 .bss 尾部到 APP 区之间保留约 1.1 KiB。APP 的镜像、初始化数据和 BSS 总量必须不超过 6 KiB；这个上限需要通过单独的 ARM 链接结果验证。若 LBJ 代码不能满足，需要拆分/压缩代码或重新评估 SRAM 分配，不能直接挤占主栈。

## K6A1 APP 镜像格式（第一版）

新增的 tools/k6_app_image.py 定义了 K6 专用容器：

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0x00 | 4 | Magic：K6A1 |
| 0x04 | 2 | Header version |
| 0x06 | 1 | ABI major |
| 0x07 | 1 | API minimum |
| 0x08 | 4 | Image size |
| 0x0C | 4 | Image CRC32 |
| 0x10 | 2 | Entry offset |
| 0x12 | 2 | Flags（bit 0 = committed） |
| 0x14 | 16 | APP name |
| 0x24 | 16 | APP version |
| 0x34 | 4 | Link VMA |
| 0x38 | 4 | BSS size |
| 0x3C | 4 | Required capabilities |

头部后紧跟连续 ARM 镜像（text、rodata、初始化 data）；BSS 不放入文件，由驻留加载器清零。容器的头部结构借鉴了 FAP1，但使用独立的 K6A1 magic 和 K6 ABI，避免把原版 PY32 APP 误认为 K6 APP。

当前打包器校验：
- 64 B 头部、magic、header/ABI 版本。
- image CRC32。
- 镜像文件不能超过 8 KiB 槽位。
- image + BSS 不能超过 6 KiB overlay SRAM。
- entry offset 必须落在镜像范围内并半字对齐。

打包器目前只处理容器，不代表 APP 已能在电台上运行。

## 当前代码状态

现有 FirmwareOverlay/app/lbj_app.c、lbj_core.c、lbj_pocsag.c、lbj_radio.c 和 FirmwareOverlay/ui/lbj.c 是静态链接进整套 LBJ 固件的模块。

下一阶段需要：
1. 增加 K6 驻留 APP 管理器：槽位扫描、头部/CRC/ABI 校验、加载、BSS 清零、入口调用和退出恢复。
2. 定义精简的 k6_app_api_t：按键、LCD、背光、时间、BK4819 接收控制/FIFO、系统恢复。
3. 把 F+7 接入 APP 列表，不占用现有菜单中无关按键。
4. 把 LBJ 解析/显示代码拆成真正独立的 K6 APP，完成独立 ARM 链接并确认总 RAM 不超过 overlay 配额。
5. 增加 APP 安装器；安装必须先读取目标机 EEPROM 备份和确认候选区空闲，再写镜像，最后写 committed 头部。
6. 研究 TDCSRX 协议/二进制并实现第二个 K6 APP。当前只确认拿到了上游 TDCSRX APP 二进制，尚未确认其 C 源码。

## 当前不声称的事项

- 没有声称 K6A1 与 FAP1 二进制兼容。
- 没有声称已完成固件侧加载器或 F+7 菜单。
- 没有向电台 EEPROM 写入 APP 区。
- 没有声称 LBJRX/TDCSRX 已可在 K6 实机运行。
