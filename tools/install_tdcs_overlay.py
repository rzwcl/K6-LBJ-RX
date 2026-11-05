#!/usr/bin/env python3
"""Install the standalone TDCS RX application into a pinned LOSEHU K6 source tree.

The installer changes only a temporary upstream source checkout. It does not
flash the radio, edit the EEPROM layout, or modify either bootloader stage.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

OVERLAY = Path(__file__).resolve().parents[1] / "FirmwareOverlay"


def read(path: Path) -> str:
    if not path.is_file():
        raise RuntimeError(f"missing expected source file: {path}")
    return path.read_text(encoding="utf-8")


def insert_once(source: str, target: str, addition: str,
                marker: str, filename: str) -> str:
    if marker in source:
        return source
    count = source.count(target)
    if count != 1:
        raise RuntimeError(
            f"{filename}: expected one integration anchor, found {count}: {target!r}"
        )
    return source.replace(target, target + addition, 1)


def replace_once(source: str, target: str, replacement: str,
                 marker: str, filename: str) -> str:
    if marker in source:
        return source
    count = source.count(target)
    if count != 1:
        raise RuntimeError(
            f"{filename}: expected one replacement anchor, found {count}: {target!r}"
        )
    return source.replace(target, replacement, 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repo", type=Path, help="root of pinned LOSEHU source tree")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    root = args.repo.resolve()

    makefile = read(root / "Makefile")
    app_c = read(root / "app/app.c")
    main_c = read(root / "main.c")
    radio_c = read(root / "radio.c")
    functions_c = read(root / "functions.c")
    ui_h = read(root / "ui/ui.h")
    ui_c = read(root / "ui/ui.c")
    firmware_ld = read(root / "firmware.ld")

    # Profile switch and RX-only build options.
    if "ENABLE_TDCSRX ?= 0" not in makefile:
        lines = makefile.splitlines(keepends=True)
        indexes = [
            i for i, line in enumerate(lines)
            if re.match(r"^\s*ENABLE_MESSENGER\s*=", line)
        ]
        if len(indexes) != 1:
            raise RuntimeError("Makefile: cannot locate unique ENABLE_MESSENGER setting")
        lines.insert(indexes[0] + 1, "ENABLE_TDCSRX                ?= 0\n")
        makefile = "".join(lines)

    makefile = insert_once(
        makefile,
        "ENABLE_TDCSRX                ?= 0\n",
        "ifeq ($(ENABLE_TDCSRX),1)\n"
        "# Dedicated receive profile: no serial control or software TX helpers.\n"
        "override ENABLE_UART := 0\n"
        "override ENABLE_VOX := 0\n"
        "override ENABLE_CUSTOM_SIDEFUNCTIONS := 0\n"
        "override ENABLE_SIDEFUNCTIONS_SEND := 0\n"
        "override ENABLE_ALARM := 0\n"
        "override ENABLE_TX1750 := 0\n"
        "endif\n",
        "Dedicated receive profile: no serial control or software TX helpers.",
        "Makefile",
    )
    makefile = insert_once(
        makefile,
        "OBJS += app/messenger.o\n",
        "\n# Standalone K6 TDCS RX image\n"
        "ifeq ($(ENABLE_TDCSRX),1)\n"
        "\tOBJS += app/tdcs_protocol.o app/tdcs_radio.o app/tdcs_app.o app/tdcs_k6_adapter.o\n"
        "\tOBJS += ui/tdcs.o\n"
        "\tOBJS += driver/uart.o # UART_Send link dependency only; UART stays disabled\n"
        "endif\n",
        "OBJS += app/tdcs_protocol.o",
        "Makefile",
    )
    makefile = insert_once(
        makefile,
        "LDFLAGS =\n",
        "ifeq ($(ENABLE_TDCSRX),1)\n"
        "ifneq ($(ENABLE_MESSENGER),0)\n"
        "$(error ENABLE_TDCSRX and ENABLE_MESSENGER cannot share the BK4819 FSK FIFO)\n"
        "endif\n"
        "ifneq ($(ENABLE_MDC1200),0)\n"
        "$(error ENABLE_TDCSRX and ENABLE_MDC1200 cannot share the BK4819 FSK FIFO)\n"
        "endif\n"
        "ifneq ($(ENABLE_AIRCOPY),0)\n"
        "$(error ENABLE_TDCSRX and ENABLE_AIRCOPY cannot share the BK4819 FSK FIFO)\n"
        "endif\n"
        "\tLDFLAGS += --specs=nosys.specs\n"
        "\tLDFLAGS += -Wl,-Map=firmware.map -Wl,--print-memory-usage\n"
        "\tCFLAGS += -DENABLE_TDCSRX\n"
        "endif\n\n",
        "CFLAGS += -DENABLE_TDCSRX",
        "Makefile",
    )

    # Export the end-of-data symbol required by newlib's nosys heap support.
    firmware_ld = insert_once(
        firmware_ld,
        "/* Check that there is enough RAM */",
        "\n\tend = .;\n\t_end = .;\n\n",
        "\n\tend = .;",
        "firmware.ld",
    )

    # Block TX at the central function transition, even if an overlooked UI
    # action tries to request FUNCTION_TRANSMIT.
    functions_c = insert_once(
        functions_c,
        "void FUNCTION_Select(FUNCTION_Type_t Function) {",
        "\n#ifdef ENABLE_TDCSRX\n"
        "    if (Function == FUNCTION_TRANSMIT) return;\n"
        "#endif\n",
        "if (Function == FUNCTION_TRANSMIT) return;",
        "functions.c",
    )

    app_c = insert_once(
        app_c,
        '#include "ui/status.h"',
        '\n#ifdef ENABLE_TDCSRX\n#include "app/tdcs_k6_adapter.h"\n#include "ui/tdcs.h"\n#endif',
        '#include "app/tdcs_k6_adapter.h"',
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "        interrupts.__raw = BK4819_ReadRegister(BK4819_REG_02);",
        "\n#ifdef ENABLE_TDCSRX\n"
        "        TDCS_K6_HandleInterrupts(interrupts.__raw);\n"
        "#endif\n",
        "TDCS_K6_HandleInterrupts(interrupts.__raw)",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "        [DISPLAY_SCANNER] = &SCANNER_ProcessKeys,",
        "\n#ifdef ENABLE_TDCSRX\n"
        "        [DISPLAY_TDCS] = &TDCS_ProcessKeys,\n"
        "#endif",
        "[DISPLAY_TDCS] = &TDCS_ProcessKeys",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "void APP_Update(void) {",
        "\n#ifdef ENABLE_TDCSRX\n    TDCS_K6_Process();\n#endif\n",
        "TDCS_K6_Process();",
        "app/app.c",
    )
    app_c = replace_once(
        app_c,
        "    if (gUpdateStatus)\n        UI_DisplayStatus();",
        "    if (gUpdateStatus) {\n"
        "#ifdef ENABLE_TDCSRX\n"
        "        if (gScreenToDisplay == DISPLAY_TDCS) {\n"
        "            gUpdateStatus = false;\n"
        "        } else {\n"
        "            UI_DisplayStatus();\n"
        "        }\n"
        "#else\n"
        "        UI_DisplayStatus();\n"
        "#endif\n"
        "    }",
        "if (gScreenToDisplay == DISPLAY_TDCS)",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "static void ProcessKey(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {",
        "\n#ifdef ENABLE_TDCSRX\n"
        "    /* The TDCS image is receive-only; ignore PTT before key dispatch. */\n"
        "    if (Key == KEY_PTT || gCurrentFunction == FUNCTION_TRANSMIT) return;\n"
        "#endif\n",
        "The TDCS image is receive-only",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "    if (gCurrentFunction == FUNCTION_POWER_SAVE)\n",
        "\n#ifdef ENABLE_TDCSRX\n"
        "    /* EXIT toggles back from the ordinary VFO screen to TDCS RX. */\n"
        "    if (gScreenToDisplay == DISPLAY_MAIN && Key == KEY_EXIT &&\n"
        "        bKeyPressed && !bKeyHeld) {\n"
        "        GUI_SelectNextDisplay(DISPLAY_TDCS);\n"
        "        gUpdateDisplay = true;\n"
        "        return;\n"
        "    }\n"
        "#endif\n",
        "EXIT toggles back from the ordinary VFO screen to TDCS RX",
        "app/app.c",
    )

    ui_h = insert_once(
        ui_h,
        "    DISPLAY_SCANNER,",
        "\n#ifdef ENABLE_TDCSRX\n    DISPLAY_TDCS,\n#endif",
        "    DISPLAY_TDCS,",
        "ui/ui.h",
    )
    ui_c = insert_once(
        ui_c,
        '#include "ui/scanner.h"',
        '\n#ifdef ENABLE_TDCSRX\n#include "ui/tdcs.h"\n#endif',
        '#include "ui/tdcs.h"',
        "ui/ui.c",
    )
    ui_c = insert_once(
        ui_c,
        "        [DISPLAY_SCANNER] = &UI_DisplayScanner,",
        "\n#ifdef ENABLE_TDCSRX\n        [DISPLAY_TDCS] = &UI_DisplayTDCS,\n#endif",
        "[DISPLAY_TDCS] = &UI_DisplayTDCS",
        "ui/ui.c",
    )

    main_c = insert_once(
        main_c,
        '#include "app/app.h"',
        '\n#ifdef ENABLE_TDCSRX\n#include "app/tdcs_k6_adapter.h"\n#endif',
        '#include "app/tdcs_k6_adapter.h"',
        "main.c",
    )
    main_c = insert_once(
        main_c,
        "    RADIO_ConfigureChannel(1, VFO_CONFIGURE_RELOAD);\n",
        "\n#ifdef ENABLE_TDCSRX\n"
        "    /* Keep the EEPROM-selected RX channel; only disable dual-watch in RAM. */\n"
        "    gEeprom.DUAL_WATCH = DUAL_WATCH_OFF;\n"
        "    gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;\n"
        "#endif\n",
        "Keep the EEPROM-selected RX channel",
        "main.c",
    )
    main_c = insert_once(
        main_c,
        "    RADIO_SetupRegisters(true);\n",
        "\n#ifdef ENABLE_TDCSRX\n    TDCS_K6_Start();\n#endif\n",
        "TDCS_K6_Start();",
        "main.c",
    )
    main_c = replace_once(
        main_c,
        "    GUI_SelectNextDisplay(DISPLAY_MAIN);",
        "#ifdef ENABLE_TDCSRX\n"
        "    GUI_SelectNextDisplay(DISPLAY_TDCS);\n"
        "#else\n"
        "    GUI_SelectNextDisplay(DISPLAY_MAIN);\n"
        "#endif",
        "GUI_SelectNextDisplay(DISPLAY_TDCS)",
        "main.c",
    )

    radio_c = insert_once(
        radio_c,
        '#include "radio.h"',
        '\n#ifdef ENABLE_TDCSRX\n#include "app/tdcs_k6_adapter.h"\n#endif',
        '#include "app/tdcs_k6_adapter.h"',
        "radio.c",
    )
    radio_c = insert_once(
        radio_c,
        "    if (switchToForeground)\n        FUNCTION_Select(FUNCTION_FOREGROUND);\n}",
        "    if (switchToForeground)\n        FUNCTION_Select(FUNCTION_FOREGROUND);\n"
        "#ifdef ENABLE_TDCSRX\n    TDCS_K6_Reapply();\n#endif\n}",
        "TDCS_K6_Reapply();",
        "radio.c",
    )
    for signature, marker in (
        ("void RADIO_SetTxParameters(void) {", "TDCS blocks RADIO_SetTxParameters"),
        ("void RADIO_PrepareTX(void) {", "TDCS blocks RADIO_PrepareTX"),
        ("void RADIO_PrepareCssTX(void) {", "TDCS blocks RADIO_PrepareCssTX"),
    ):
        radio_c = insert_once(
            radio_c,
            signature,
            f"\n#ifdef ENABLE_TDCSRX\n    return; /* {marker} */\n#endif\n",
            marker,
            "radio.c",
        )

    if args.dry_run:
        print("TDCS dry-run OK; all integration anchors found, no files written.")
        return 0

    copies = {
        "protocol/tdcs_protocol.c": "app/tdcs_protocol.c",
        "protocol/tdcs_protocol.h": "app/tdcs_protocol.h",
        "app/tdcs_radio.c": "app/tdcs_radio.c",
        "app/tdcs_radio.h": "app/tdcs_radio.h",
        "app/tdcs_app.c": "app/tdcs_app.c",
        "app/tdcs_app.h": "app/tdcs_app.h",
        "app/tdcs_k6_adapter.c": "app/tdcs_k6_adapter.c",
        "app/tdcs_k6_adapter.h": "app/tdcs_k6_adapter.h",
        "ui/tdcs.c": "ui/tdcs.c",
        "ui/tdcs.h": "ui/tdcs.h",
    }
    for source, destination in copies.items():
        target = root / destination
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(OVERLAY / source, target)

    for rel, content in (
        ("Makefile", makefile),
        ("app/app.c", app_c),
        ("main.c", main_c),
        ("radio.c", radio_c),
        ("functions.c", functions_c),
        ("ui/ui.h", ui_h),
        ("ui/ui.c", ui_c),
        ("firmware.ld", firmware_ld),
    ):
        (root / rel).write_text(content, encoding="utf-8", newline="")

    print("TDCS RX overlay installed into the isolated LOSEHU source tree.")
    print("Radio interrupt/FIFO, application processing, display, and RX reapply hooks are connected.")
    print("PTT and transmit entry points are blocked at the application and radio layers.")
    print("The currently selected EEPROM RX channel is used; no channel or EEPROM data is written by the installer.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
