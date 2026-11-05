# K6 原生 Overlay APP 移植记录

状态：K6A1 容器、主机模拟 EEPROM 加载器、API 绑定和独立 ARM 探针已完成第一轮测试；**尚未接入主固件、未实现 APP 菜单/实际 EEPROM 写入，也未进行 K6 实机测试**。

## 目标

为 DP32G030 + BK4819 的 K6 主固件增加独立应用机制：

- 一套主固件中可以同时安装 LBJRX-K6、TDCSRX-K6 等多个 APP。
- APP 分别占用独立槽位；不会把一个 APP 当成另一份完整固件。
- 通过新增的独立 Apps 菜单项打开列表，选择一个启动；同一时刻运行一个 APP。快捷键只能作为可配置入口，不能覆盖原有 F 组合。
- APP 退出后回到原主界面。
- APP 有自己的 K6 API/ABI，不直接依赖主固件内部符号。
- 原有 LOSEHU 多固件 Bootloader、固件目录和其他固件镜像不作为本项目的改造对象。

## 已核实的原版机制

原版 LBJRADIO/F4HWN APP 属于 RAM Overlay：

- .app 文件使用 FAP1 容器，头部为 64 字节。
- 每个槽位 8 KiB；代码段在槽位偏移 0x1000，资源放在头部所在的 4 KiB 区域。
- 固件从外部 PY25Q16 SPI Flash 读取 APP，将代码加载到固定 SRAM 地址运行。
- 主固件提供函数指针 API；APP 入口为 app_main(const app_api_t *api)。
- 原版 F4HWN 固件通过 F+7 菜单启动，安装/擦除/验证通过 0x073x 串口命令完成。K6 不能直接照搬这个快捷键：LOSEHU 0.13.2 的 F+7 在启用 VOX 时切换 VOX，未启用 VOX 时切换信道扫描列表。K6 版默认新增独立 Apps 菜单项，不关闭 VOX、不抢占原快捷键。

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

- APP 区候选范围：0x2A400–0x3A400（结束地址不包含）
- 槽位大小：0x2000（8 KiB）
- 槽位数量：8
- 槽位地址：0x2A400 + slot * 0x2000

此候选范围已使用用户提供的完整 EEPROM 备份进行逐字节只读检查（2026-11-05）：

| 检查项 | 实测结果 |
|---|---|
| 备份文件 | `Thu Nov 05 2026 12_10_17 GMT+0800 (中国标准时间)_backup.bin` |
| 容量 | 524,288 B（0x80000，512 KiB） |
| SHA-256 | `d51cf65bc3a4d326617ee53bbed01177c047c9f00e6971663e6ad33a74699153` |
| CRC32 | `0xD51194EE` |
| 拼音/汉字区最后一个非擦除字节 | `0x2A31B` |
| `0x2A400–0x3A400` | 65,536 B 全部为 `0xFF` |
| `0x3A400–0x3C210` | 额外间隔 7,696 B 全部为 `0xFF` |
| SI4732 区域首个观测到的非 `0xFF` 字节 | `0x3C228` |
| 多固件目录起点 | `0x40000`，目录记录可见且未触及候选范围 |

8 个槽位分别检查，全部 8,192 B 为 `0xFF`，非擦除字节均为 0。由此可以把 `0x2A400–0x3A400` 确认为这份备份里的连续擦除区，并在 K6 APP 设计中正式预留为 8 个独立 APP 槽位；候选区与拼音表、SI4732 数据及多固件目录没有字节范围重叠。

只读报告由 Actions run 42026914782 生成，artifact `K6-EEPROM-backup-read-only-report`。**当前仍未向 EEPROM 写入任何内容**。正式接入硬件安装器前，仍要完成 I²C 读写回调的实机验证和安装中断恢复测试；同时保留这份原始备份作为恢复基准。

### K5Web BL 槽位界面参考

用户指定参考页面：`https://k5.vicicode.cn/#/tool/bl`。已检查其开源实现 `silenty4ng/k5web/src/views/list/bl/index.vue`，界面与存储模型如下：

- 使用 64 B 为一个可视化 block，把 EEPROM `0x40000–0x80000` 显示为 4,096 个 block。
- 前 256 个 block（`0x40000–0x44000`，16 KiB）显示为 Bootloader 保留区；后续区域由用户把固件卡片拖放到起始 block，按固件长度连续占用 block。
- 固件文件按 64 B 对齐填充 `0xFF`；界面用不同颜色显示每个固件占用的 block，并显示固件名称。固件数量、版本和每个固件的起止地址写入 `0x40000` 附近的目录元数据。
- 写入流程逐 64 B 写入并立即读回比较；写入失败会重试该 block。

**一个容易忽略的区别：** 当前 K5Web 这份源码里的 `readConfig()` 是空函数，页面启动时仅把前 256 个 block 标成 Bootloader 保留，其余 block 全部初始化为空。因此它主要展示用户在网页上编排的“待写入布局”，不能据此认为页面已从电台自动读取并还原当前真实固件占用情况。Losehu Bootloader 文档也明确说明了其固件目录和 EEPROM 区域分工。实际依据见 [K5Web BL 页面源码](https://github.com/silenty4ng/k5web/blob/master/src/views/list/bl/index.vue) 与 [多固件 Bootloader 的 EEPROM 说明](https://github.com/losehu/uv-k5-bootloader-custom/wiki/10-%E2%80%90-Basic-idea-and-operation)。

**K6 APP 管理器借鉴交互，不照搬固件目录和地址：**

- 仍然保留 8 个固定 APP 槽位，每槽 8 KiB；不把 APP 放入 `0x40000–0x80000` 的多固件目录/镜像区。
- 页面采用 8 张可拖动 APP 卡片和 8 个固定槽位格；可拖动 LBJRX/TDCSRX 卡片到指定槽位，也可以调整槽位顺序。
- 每槽显示：槽号、APP 名称/版本、代码长度、资源长度、占用比例、CRC 校验状态；空槽明确显示“空闲”，损坏或未提交的槽显示“无效”，不能伪装成空槽。
- 顶部汇总空闲槽数、已用容量和总容量；提供从电台读取真实槽位、选择 `.k6app` 文件、安装、验证、卸载和读回备份操作。
- 与 K5Web 现有页面不同，K6 APP 管理器必须先通过只读 EEPROM 协议扫描 8 个 K6A1 头部，逐槽读取并验证代码/资源 CRC，再显示“当前设备状态”。拖动只改变待执行的槽位布局；安装时先写目标槽、读回验证、最后提交头部，验证完成后才清理旧槽。不得只显示浏览器本地的预设布局。

### SRAM

已单独从干净的 upstream 0.13.2（commit 021656ea13226d349e31acd6eb21079c5bab64c7）构建 132HS 特征基线，配置为 ENABLE_CHINESE_FULL=4、ENABLE_PINYIN=1、ENABLE_4732=1；没有运行 LBJ 修改脚本，没有禁用 VOX、DTMF 等功能。干净 ELF 的链接结果：

- SRAM：0x20000000–0x20003FFF（16 KiB）
- .data：148 B，VMA 0x20000000–0x20000094
- .bss：2,644 B，VMA 0x20000094–0x20000AE8
- 链接脚本保留的最小栈区：128 B，0x20000AE8–0x20000B68
- 初始栈顶：0x20004000

基线报告：Actions run 42020554230 的 artifact 只包含内存报告，没有上传任何固件 BIN。该构建来自干净 upstream tag，不包含你原始 BIN 文件名中标注的 MDC 尾音定制改动，因此是同特征布局参考，不声称与原 BIN 逐字节/逐功能相同。

当前规划的 APP overlay SRAM 区：

- 起始：0x20001800
- 大小：0x1800（6 KiB）
- 结束：0x20003000
- 留给主栈的上部空间：0x1000（4 KiB）

按干净基线计算，0x20000AE8 到 APP 起始地址 0x20001800 之间有 3,352 B 空隙；APP 区结束地址 0x20003000 到初始栈顶 0x20004000 之间保留 4 KiB 给主固件/APP 调用栈。APP 的镜像、初始化数据和 BSS 总量必须不超过 6 KiB。4 KiB 栈预算尚未通过运行时最大栈深度测量，必须在 APP 运行测试阶段继续验证。

## K6A1 APP 镜像格式（第一版）

新增的 tools/k6_app_image.py 定义了 K6 专用容器，文件总长度为 64 B 头部 + APP 代码镜像 + 可选资源数据：

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
| 0x14 | 12 | APP name |
| 0x20 | 12 | APP version |
| 0x2C | 4 | Link VMA |
| 0x30 | 4 | BSS size |
| 0x34 | 4 | Required capabilities |
| 0x38 | 4 | Asset size |
| 0x3C | 4 | Asset CRC32 |

头部后紧跟连续 ARM 镜像（text、rodata、初始化 data），再后面是只读资源区。驻留加载器只把代码镜像复制到 SRAM，BSS 清零；资源仍留在 EEPROM，APP 通过 asset_read() 分块读取，避免把车型表和字符串全部占用 RAM。容器有独立的 K6A1 magic 和 K6 ABI，不能当成原版 PY32 FAP1 APP。

当前打包器校验：
- 64 B 头部、magic、header/ABI 版本。
- 代码 CRC32 与资源 CRC32 分开校验。
- 代码镜像+BSS 不能超过 6 KiB SRAM。
- 头部+代码+资源不能超过 8 KiB EEPROM 槽位。
- image + BSS 不能超过 6 KiB overlay SRAM。
- entry offset 必须落在镜像范围内并半字对齐。

固定 VMA 的 K6 APP 探针已通过 ARM 编译、链接和封装：代码 236 B、BSS 4 B、总 SRAM 240/6,144 B，带头文件后 300/8,192 B。Actions run 42020338961 只上传 probe report 和 linker map。探针只验证构建/封装路径，不代表 APP 已能在电台上运行。

## 当前代码状态

- `FirmwareOverlay/apps/k6_app_runtime.c` 已实现槽位头读取、镜像/资源 CRC 校验、代码复制、BSS 清零、资源读取、槽位信息复核、Thumb 入口地址计算，以及单 APP API 服务绑定。
- `tests/test_k6_app_runtime.c` 用模拟 EEPROM 测试上述逻辑；它不是 DP32G030 上的硬件测试。
- `K6_APP_EEPROM_BASE=0x2A400` 已由用户提供的完整 512 KiB EEPROM 备份逐字节确认：8 个槽位全部为 `0xFF`，可以作为 K6 APP 的规划区；但硬件写入仍须经过页边界、写入等待、读回校验和断电恢复测试。
- 尚未将 `K6APP_Load()`、`K6APP_BuildAPI()` 接入 LOSEHU 主循环、显示/按键调度或 BK4819 驱动。当前探针由 CI 编译和打包，不是可直接安装的 APP。

现有 FirmwareOverlay/app/lbj_app.c、lbj_core.c、lbj_pocsag.c、lbj_radio.c 和 FirmwareOverlay/ui/lbj.c 是静态链接进整套 LBJ 固件的模块。

下一阶段需要：
1. 增加 K6 驻留 APP 管理器：槽位扫描、头部/CRC/ABI 校验、加载、BSS 清零、入口调用和退出恢复。
2. 定义精简的 K6 APP API。第一版头文件和探针已经新增：FirmwareOverlay/apps/k6_app_api.h、app_probe.c、k6_app.ld；当前 API 提供显示、按键、时间、只读接收配置/FIFO/RSSI、射频恢复和 EEPROM 资源读取接口；不提供任意 BK4819 寄存器写入，也不提供发射 API。
3. 在主菜单增加独立 Apps 入口；后续可以增加用户自选快捷键，但默认不占用 F+7、F+0 等现有组合。
4. 把 LBJ 解析/显示代码拆成真正独立的 K6 APP，完成独立 ARM 链接并确认总 RAM 不超过 overlay 配额。
5. APP 镜像安装核心已实现：`K6APP_Install()` 拒绝覆盖非空槽，按 128 B EEPROM 页边界拆分写入，先写未提交头，再写代码/资源，逐块读回并验证 CRC，最后单独提交有效头；`K6APP_VerifySlot()` 可复核槽位。主机模拟测试已覆盖安装、断电中断保持未提交、已占用槽拒绝覆盖。尚未把该核心连接到实际 K6 串口安装器；协议参考 K5Web/LOSEHU：扩展协议 `0x052B/0x051C` 读、`0x0538/0x051E` 写，`0x073x` 是 F4HWN 外置 SPI Flash APP 命令，不用于本 K6 EEPROM 方案。
6. 研究 TDCSRX 协议/二进制并实现第二个 K6 APP。当前只确认拿到了上游 TDCSRX APP 二进制，尚未确认其 C 源码。

## 上游 ENABLE_OVERLAY 的含义

LOSEHU 0.13.2 的 ENABLE_OVERLAY / sram-overlay.c 是 MCU 内部 Flash 擦写代码的 SRAM 运行副本，与 F4HWN 外置 Flash APP 系统不是一回事；它不提供 `.app` 槽位、APP API 或运行菜单。本项目不能因为该宏存在就认为 K6 已有 APP 支持。

## 当前不声称的事项

- 上次 standalone LBJ 编译所生成的 LOSEHU 修改版不作为基线、不作为日常固件，也不用于刷机。本分支新建的 pristine baseline workflow 从未应用旧的 install_overlay.py；它只编译干净上游源码并仅上传 SRAM 报告。

- 没有声称 K6A1 与 FAP1 二进制兼容。
- 没有声称已完成固件侧加载器或 F+7 菜单。
- 没有向电台 EEPROM 写入 APP 区。
- 没有声称 LBJRX/TDCSRX 已可在 K6 实机运行。
