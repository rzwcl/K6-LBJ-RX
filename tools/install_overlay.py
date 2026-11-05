#!/usr/bin/env python3
"""Install the standalone LBJ receiver overlay into a LOSEHU K5/K6 source tree.

This edits source/build integration points and adds a direct-FSK 1200 RX adapter.
It does not flash, modify EEPROM, change the bootloader, or write calibration data.
"""
from __future__ import annotations

import argparse
import shutil
import sys
import re
from pathlib import Path

OVERLAY = Path(__file__).resolve().parents[1] / "FirmwareOverlay"


def read(path: Path) -> str:
    if not path.is_file():
        raise RuntimeError(f"missing expected source file: {path}")
    return path.read_text(encoding="utf-8")


def insert_once(source: str, target: str, addition: str, marker: str, filename: str) -> str:
    if marker in source:
        return source
    count = source.count(target)
    if count != 1:
        raise RuntimeError(f"{filename}: expected exactly one integration anchor, found {count}: {target!r}")
    return source.replace(target, target + addition, 1)


def replace_once(source: str, target: str, replacement: str, marker: str, filename: str) -> str:
    if marker in source:
        return source
    count = source.count(target)
    if count != 1:
        raise RuntimeError(f"{filename}: expected exactly one replacement anchor, found {count}: {target!r}")
    return source.replace(target, replacement, 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repo", type=Path, help="root folder of losehu/uv-k5-firmware-custom")
    parser.add_argument("--dry-run", action="store_true", help="validate anchors without writing files")
    args = parser.parse_args()
    root = args.repo.resolve()

    # Build every edited file in memory first. A failed anchor check leaves the tree untouched.
    makefile = read(root / "Makefile")
    app_c = read(root / "app/app.c")
    action_c = read(root / "app/action.c")
    ui_h = read(root / "ui/ui.h")
    ui_c = read(root / "ui/ui.c")
    main_c = read(root / "main.c")
    radio_c = read(root / "radio.c")

    makefile = insert_once(
        makefile,
        "OBJS += app/messenger.o",
        "\n# LBJ standalone receiver application\nifeq ($(ENABLE_LBJ),1)\n"
        "\tOBJS += app/lbj_app.o app/lbj_pocsag.o app/lbj_core.o app/lbj_loco.o app/lbj_train_type.o app/lbj_radio.o\n"
        "\tOBJS += ui/lbj.o\nendif\n",
        "# LBJ standalone receiver application",
        "Makefile",
    )
    if "ENABLE_LBJ ?= 0" not in makefile:
        anchor = "ENABLE_MESSENGER"
        lines = makefile.splitlines(keepends=True)
        indexes = [i for i, line in enumerate(lines) if re.match(r"^\s*ENABLE_MESSENGER\s*=", line)]
        if len(indexes) != 1:
            raise RuntimeError("Makefile: cannot find unique ENABLE_MESSENGER setting")
        lines.insert(indexes[0] + 1, "ENABLE_LBJ                   ?= 0\n")
        makefile = "".join(lines)
    makefile = insert_once(
        makefile,
        "ENABLE_LBJ                   ?= 0\n",
        "ifeq ($(ENABLE_LBJ),1)\n"
        "# LBJ RX-only build disables optional TX and serial EEPROM-write paths.\n"
        "override ENABLE_UART := 0\n"
        "override ENABLE_VOX := 0\n"
        "override ENABLE_DTMF_CALLING := 0\n"
        "override ENABLE_CUSTOM_SIDEFUNCTIONS := 0\n"
        "override ENABLE_SIDEFUNCTIONS_SEND := 0\n"
        "override ENABLE_ALARM := 0\n"
        "override ENABLE_TX1750 := 0\n"
        "endif\n",
        "LBJ RX-only build disables optional TX and serial EEPROM-write paths.",
        "Makefile",
    )
    makefile = insert_once(
        makefile,
        "LDFLAGS =",
        "\nifeq ($(ENABLE_LBJ),1)\n"
        "ifneq ($(ENABLE_MESSENGER),0)\n$(error ENABLE_LBJ and ENABLE_MESSENGER cannot share the BK4819 FSK FIFO)\nendif\n"
        "ifneq ($(ENABLE_MDC1200),0)\n$(error ENABLE_LBJ and ENABLE_MDC1200 cannot share the BK4819 FSK FIFO)\nendif\n"
        "ifneq ($(ENABLE_AIRCOPY),0)\n$(error ENABLE_LBJ and ENABLE_AIRCOPY cannot share the BK4819 FSK FIFO)\nendif\n"
        "\tCFLAGS += -DENABLE_LBJ\nendif\n\n",
        "CFLAGS += -DENABLE_LBJ",
        "Makefile",
    )

    # ACTION_D_DCD references a VFO field that only exists when
    # ENABLE_DTMF_CALLING is enabled. Keep the action harmless when disabled.
    action_c = replace_once(
        action_c,
        "void ACTION_D_DCD(void) {\n    gRequestSaveChannel = 1;\n\n"
        "    gTxVfo->DTMF_DECODING_ENABLE = !gTxVfo->DTMF_DECODING_ENABLE;\n"
        "    DTMF_clear_RX();\n}",
        "void ACTION_D_DCD(void) {\n#ifdef ENABLE_DTMF_CALLING\n"
        "    gRequestSaveChannel = 1;\n\n"
        "    gTxVfo->DTMF_DECODING_ENABLE = !gTxVfo->DTMF_DECODING_ENABLE;\n"
        "    DTMF_clear_RX();\n#endif\n}",
        "#ifdef ENABLE_DTMF_CALLING\\n    gRequestSaveChannel = 1;",
        "app/action.c",
    )

    app_c = insert_once(
        app_c,
        '#include "ui/status.h"',
        '\n#ifdef ENABLE_LBJ\n#include "app/lbj_app.h"\n#endif',
        "#include \"app/lbj_app.h\"",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        '#include "ui/status.h"',
        '\n#ifdef ENABLE_LBJ\n#include "app/lbj_radio.h"\n#endif',
        '#include "app/lbj_radio.h"',
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "        interrupts.__raw = BK4819_ReadRegister(BK4819_REG_02);",
        "\n#ifdef ENABLE_LBJ\n        LBJ_RFHandleInterrupts(interrupts.__raw);\n#endif",
        "LBJ_RFHandleInterrupts(interrupts.__raw)",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "        [DISPLAY_SCANNER] = &SCANNER_ProcessKeys,",
        "\n#ifdef ENABLE_LBJ\n        [DISPLAY_LBJ] = &LBJ_ProcessKeys,\n#endif",
        "[DISPLAY_LBJ] = &LBJ_ProcessKeys",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "void APP_Update(void) {",
        "\n#ifdef ENABLE_LBJ\n    LBJ_AppProcess();\n#endif\n",
        "LBJ_AppProcess();",
        "app/app.c",
    )
    app_c = replace_once(
        app_c,
        "    if (gUpdateStatus)\n        UI_DisplayStatus();",
        "    if (gUpdateStatus) {\n"
        "#ifdef ENABLE_LBJ\n"
        "        if (gScreenToDisplay == DISPLAY_LBJ) {\n"
        "            /* The LBJ screen owns the top status strip. */\n"
        "            gUpdateStatus = false;\n"
        "        } else {\n"
        "            UI_DisplayStatus();\n"
        "        }\n"
        "#else\n"
        "        UI_DisplayStatus();\n"
        "#endif\n"
        "    }",
        "The LBJ screen owns the top status strip.",
        "app/app.c",
    )
    app_c = insert_once(
        app_c,
        "static void ProcessKey(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {",
        "\n#ifdef ENABLE_LBJ\n    /* Dedicated LBJ image: do not allow PTT/TX from any screen. */\n"
        "    if (Key == KEY_PTT) return;\n#endif\n",
        "Dedicated LBJ image: do not allow PTT/TX",
        "app/app.c",
    )

    ui_h = insert_once(
        ui_h,
        "    DISPLAY_SCANNER,",
        "\n#ifdef ENABLE_LBJ\n    DISPLAY_LBJ,\n#endif",
        "    DISPLAY_LBJ,",
        "ui/ui.h",
    )
    ui_c = insert_once(
        ui_c,
        '#include "ui/scanner.h"',
        '\n#ifdef ENABLE_LBJ\n#include "ui/lbj.h"\n#endif',
        '#include "ui/lbj.h"',
        "ui/ui.c",
    )
    ui_c = insert_once(
        ui_c,
        "        [DISPLAY_SCANNER] = &UI_DisplayScanner,",
        "\n#ifdef ENABLE_LBJ\n        [DISPLAY_LBJ] = &UI_DisplayLBJ,\n#endif",
        "[DISPLAY_LBJ] = &UI_DisplayLBJ",
        "ui/ui.c",
    )

    main_c = insert_once(
        main_c,
        '#include "app/app.h"',
        '\n#ifdef ENABLE_LBJ\n#include "app/lbj_app.h"\n#endif',
        '#include "app/lbj_app.h"',
        "main.c",
    )
    main_c = insert_once(
        main_c,
        '#include "app/app.h"',
        '\n#ifdef ENABLE_LBJ\n#include "app/lbj_radio.h"\n#endif',
        '#include "app/lbj_radio.h"',
        "main.c",
    )
    # LOSEHU 132/133 source trees contain harmless whitespace variations
    # around the VFO setup call. Match the two calls structurally instead of
    # requiring one exact blank-line sequence.
    if "gRxVfo->freq_config_RX.Frequency = 82123750u" not in main_c:
        setup_pattern = re.compile(
            r"(?m)^(?P<indent>[ \t]*)RADIO_SelectVfos\(\);[ \t]*\r?\n"
            r"(?:[ \t]*\r?\n)*(?P<setup_indent>[ \t]*)RADIO_SetupRegisters\(true\);"
        )
        matches = list(setup_pattern.finditer(main_c))
        if len(matches) != 1:
            raise RuntimeError(
                "main.c: expected one RADIO_SelectVfos()/RADIO_SetupRegisters(true) pair, "
                f"found {len(matches)}"
            )
        match = matches[0]
        indent = match.group("indent")
        setup_indent = match.group("setup_indent")
        replacement = (
            f"{indent}RADIO_SelectVfos();\n\n"
            "#ifdef ENABLE_LBJ\n"
            "    /* Fixed RX-only LBJ profile; all changes below remain in RAM. */\n"
            "    gEeprom.DUAL_WATCH = DUAL_WATCH_OFF;\n"
            "    gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;\n"
            "    gRxVfo->FrequencyReverse = 0;\n"
            "    gRxVfo->freq_config_RX.Frequency = 82123750u; /* LOSEHU frequency unit: 10 Hz */\n"
            "    gRxVfo->pRX = &gRxVfo->freq_config_RX;\n"
            "    gRxVfo->Band = BAND7_470MHz;\n"
            "    gRxVfo->Modulation = MODULATION_FM;\n"
            "#endif\n\n"
            f"{setup_indent}RADIO_SetupRegisters(true);\n"
            "#ifdef ENABLE_LBJ\n"
            "    LBJ_AppInit();\n"
            "    LBJ_RFStart();\n"
            "#endif"
        )
        main_c = main_c[:match.start()] + replacement + main_c[match.end():]
    main_c = replace_once(
        main_c,
        "    GUI_SelectNextDisplay(DISPLAY_MAIN);",
        "#ifdef ENABLE_LBJ\n    GUI_SelectNextDisplay(DISPLAY_LBJ);\n#else\n"
        "    GUI_SelectNextDisplay(DISPLAY_MAIN);\n#endif",
        "GUI_SelectNextDisplay(DISPLAY_LBJ)",
        "main.c",
    )

    radio_c = insert_once(
        radio_c,
        "void RADIO_SetTxParameters(void) {",
        "\n#ifdef ENABLE_LBJ\n    return; /* LBJ blocks RADIO_SetTxParameters */\n#endif\n",
        "LBJ blocks RADIO_SetTxParameters",
        "radio.c",
    )
    radio_c = insert_once(
        radio_c,
        "void RADIO_PrepareTX(void) {",
        "\n#ifdef ENABLE_LBJ\n    return; /* LBJ blocks RADIO_PrepareTX */\n#endif\n",
        "LBJ blocks RADIO_PrepareTX",
        "radio.c",
    )
    radio_c = insert_once(
        radio_c,
        "void RADIO_PrepareCssTX(void) {",
        "\n#ifdef ENABLE_LBJ\n    return; /* LBJ blocks RADIO_PrepareCssTX */\n#endif\n",
        "LBJ blocks RADIO_PrepareCssTX",
        "radio.c",
    )

    if args.dry_run:
        print("Dry-run OK. Integration anchors found; no files written.")
        return 0

    for rel in (
        "app/lbj_app.c", "app/lbj_app.h", "app/lbj_core.c", "app/lbj_core.h",
        "app/lbj_loco.c", "app/lbj_loco.h", "app/lbj_pocsag.c", "app/lbj_pocsag.h",
        "app/lbj_train_type.c", "app/lbj_train_type.h", "app/lbj_radio.c", "app/lbj_radio.h",
        "ui/lbj.c", "ui/lbj.h",
    ):
        target = root / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(OVERLAY / rel, target)

    for rel, content in (("Makefile", makefile), ("app/app.c", app_c),
                         ("app/action.c", action_c), ("ui/ui.h", ui_h), ("ui/ui.c", ui_c),
                         ("main.c", main_c), ("radio.c", radio_c)):
        (root / rel).write_text(content, encoding="utf-8", newline="")

    print("LBJ source/UI overlay installed.")
    print("The BK4819 direct-FSK FIFO adapter and receive-only TX guards are installed. RF timing, byte order, and burst-length behavior still require K6 bench verification.")
    print("No bootloader, EEPROM layout, stored firmware slot, or calibration data was changed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
