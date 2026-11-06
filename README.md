# K6 overlay APP 试验分支 — 已归档，不是当前 LBJRX/TDCSRX 移植

<badge color="warning">不要从本分支获取可刷入的 LBJRX / TDCSRX 固件</badge>

本分支保留的是早期 K6 overlay APP 容器、EEPROM 槽位管理和主机模拟研究。它不是本次要求的两份独立完整 K6 固件，也不是原版 LBJRX/TDCSRX 的源码。

上一轮错误的 LBJ/TDCS 实验解码器、测试、专属构建/安装脚本、对象文件和错误 BIN 已从相关分支删除；旧提交仍留在 Git 历史中，未做历史重写。不要从旧 SHA 或历史 Actions artifact 下载旧 BIN。

## 保留内容

- 通用 K6 overlay APP ABI/runtime 原型。
- K6 APP EEPROM 槽位、K6A1 容器、主机模拟读写/CRC 测试。
- 与接收器实现无关的 EEPROM 备份和存储区研究。
- 用户确认的 LCD 排版/按键规格历史资料。

这些是通用或历史研究，保留以免误删其他工作；**不会作为当前 LBJRX/TDCSRX 完整固件的实现基础**。

## 当前任务的正确入口

请切换到默认分支 `main`，阅读：
- [当前项目状态与目标](https://github.com/rzwcl/K6-LBJ-RX/blob/main/README.md)
- [完整交接记录](https://github.com/rzwcl/K6-LBJ-RX/blob/main/docs/CONTINUATION.md)
- [LBJRX 0.4 源码参考副本](https://github.com/rzwcl/K6-LBJ-RX/tree/main/research/original_apps/LBJRX-RX04)
- [TDCSRX 0.3 部分逆向恢复](https://github.com/rzwcl/K6-LBJ-RX/tree/main/research/tdcsrx_rx03)

当前 main 没有可刷入的 LBJRX-K6 或 TDCSRX-K6 固件。最终目标仍是两份独立的完整 LOSEHU 固件，使用现有多固件 Bootloader 追加两个固件目录项；不开发新的 Bootloader，也不把原版 FAP1 APP 直接当成 K6 完整固件。
