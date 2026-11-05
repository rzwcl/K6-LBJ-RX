CC ?= gcc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -Iinclude

HOST_TEST = tests/test_host
CORE_TEST = tests/test_core
APP_TEST = tests/test_app
RADIO_TEST = tests/test_radio
HOST_SRCS = src/lbj_pocsag.c src/lbj_train_type.c tests/test_host.c
CORE_SRCS = src/lbj_core.c src/lbj_loco.c src/lbj_train_type.c tests/test_core.c

.PHONY: all test clean
all: test

$(HOST_TEST): $(HOST_SRCS) include/lbj_pocsag.h include/lbj_train_type.h
	$(CC) $(CFLAGS) $(HOST_SRCS) -o $(HOST_TEST)

$(CORE_TEST): $(CORE_SRCS) include/lbj_core.h include/lbj_loco.h include/lbj_train_type.h
	$(CC) $(CFLAGS) $(CORE_SRCS) -o $(CORE_TEST)

APP_SRCS = src/lbj_app.c src/lbj_pocsag.c src/lbj_core.c src/lbj_loco.c src/lbj_train_type.c tests/stubs/extra_globals.c tests/test_app.c
$(APP_TEST): $(APP_SRCS) include/lbj_app.h include/lbj_core.h include/lbj_pocsag.h
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/stubs -Iinclude $(APP_SRCS) -o $(APP_TEST)

$(RADIO_TEST): FirmwareOverlay/app/lbj_radio.c tests/test_radio.c
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -Itests/stubs -IFirmwareOverlay FirmwareOverlay/app/lbj_radio.c tests/test_radio.c -o $(RADIO_TEST)

test: $(HOST_TEST) $(CORE_TEST) $(APP_TEST) $(RADIO_TEST)
	./$(HOST_TEST)
	./$(CORE_TEST)
	./$(APP_TEST)
	./$(RADIO_TEST)

clean:
	rm -f $(HOST_TEST) $(CORE_TEST) $(APP_TEST) $(RADIO_TEST)

.PHONY: syntax-firmware-overlay
syntax-firmware-overlay:
	$(CC) -std=c99 -Wall -Wextra -Werror -Itests/stubs -IFirmwareOverlay -Iinclude -fsyntax-only FirmwareOverlay/app/lbj_app.c FirmwareOverlay/app/lbj_pocsag.c FirmwareOverlay/app/lbj_core.c FirmwareOverlay/app/lbj_loco.c FirmwareOverlay/app/lbj_train_type.c FirmwareOverlay/app/lbj_radio.c FirmwareOverlay/ui/lbj.c src/lbj_app.c src/lbj_ui.c
