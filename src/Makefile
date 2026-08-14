RIV_HOME ?= $(HOME)/.riv
RIVEMU ?= $(RIV_HOME)/rivemu
RIVEMU_SDK ?= $(RIV_HOME)/rivos-sdk.ext2
HOST_CC ?= cc
TEST_BUILD_DIR ?= ../.test-build
STRICT_BUILD_DIR ?= ../.strict-build
RIV_HEADER := $(STRICT_BUILD_DIR)/include/riv.h
GAME_CPPFLAGS ?=

GAME_SOURCES := bombflip.c game.c board.c render.c title.c audio.c
GAME_OBJECTS := $(GAME_SOURCES:.c=.o)
GAME_HEADERS := state.h controls.h game.h board.h render.h title.h audio.h seqt.h
HOST_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -fanalyzer -fcommon \
	-DRIV_IMPORT_CONTEXT -I$(STRICT_BUILD_DIR)/include -I. -I../tests
STRICT_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer \
	-I$(STRICT_BUILD_DIR)/include -I.

.PHONY: all clean run smoke strict test

all: bombflip.sqfs

%.o: %.c $(GAME_HEADERS)
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk -workspace -mem=1024 \
		-exec "gcc $(GAME_CPPFLAGS) $< -c -o $@ \$$(riv-opt-flags -Ospeed)"

bombflip: $(GAME_OBJECTS)
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk -workspace \
		-exec "gcc $(GAME_OBJECTS) -o bombflip -lriv -lm"
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk -workspace \
		-exec "riv-strip bombflip"

bombflip.sqfs: bombflip songs/gameplay.rivcard info.json cover.png
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk -workspace \
		-exec "riv-mksqfs bombflip songs info.json cover.png bombflip.sqfs"

run: bombflip.sqfs
	$(RIVEMU) -cartridge=bombflip.sqfs

smoke: bombflip.sqfs
	$(RIVEMU) -quiet -no-window -no-audio -stop-frame=180 -cartridge=bombflip.sqfs

$(RIV_HEADER):
	mkdir -p $(STRICT_BUILD_DIR)/include
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk \
		-exec "cat /usr/include/riv.h" > $(RIV_HEADER)
	@test -s $(RIV_HEADER)

strict: $(RIV_HEADER)
	mkdir -p $(STRICT_BUILD_DIR)/release $(STRICT_BUILD_DIR)/debug $(STRICT_BUILD_DIR)/cheats
	@set -e; for source in $(GAME_SOURCES); do \
		object=$${source%.c}.o; \
		$(HOST_CC) $(STRICT_CFLAGS) -c $$source \
			-o $(STRICT_BUILD_DIR)/release/$$object; \
		$(HOST_CC) $(STRICT_CFLAGS) -DDEBUG_MODE=1 -c $$source \
			-o $(STRICT_BUILD_DIR)/debug/$$object; \
		$(HOST_CC) $(STRICT_CFLAGS) -DCHEATS_ENABLED=1 -c $$source \
			-o $(STRICT_BUILD_DIR)/cheats/$$object; \
	done

test: $(RIV_HEADER)
	mkdir -p $(TEST_BUILD_DIR)
	$(HOST_CC) $(HOST_CFLAGS) ../tests/test_board.c ../tests/test_support.c board.c -lm \
		-o $(TEST_BUILD_DIR)/test_board
	$(TEST_BUILD_DIR)/test_board
	$(HOST_CC) $(HOST_CFLAGS) -DDEBUG_MODE=1 -DCHEATS_ENABLED=1 \
		../tests/test_game.c ../tests/test_support.c \
		game.c board.c -lm -o $(TEST_BUILD_DIR)/test_game
	$(TEST_BUILD_DIR)/test_game

clean:
	rm -f $(GAME_OBJECTS) bombflip bombflip.sqfs
	rm -f $(TEST_BUILD_DIR)/test_board $(TEST_BUILD_DIR)/test_game
	rm -f $(STRICT_BUILD_DIR)/release/*.o $(STRICT_BUILD_DIR)/debug/*.o \
		$(STRICT_BUILD_DIR)/cheats/*.o $(RIV_HEADER)
	rmdir $(STRICT_BUILD_DIR)/release $(STRICT_BUILD_DIR)/debug \
		$(STRICT_BUILD_DIR)/cheats $(STRICT_BUILD_DIR)/include 2>/dev/null || true
	rmdir $(STRICT_BUILD_DIR) 2>/dev/null || true
	rmdir $(TEST_BUILD_DIR) 2>/dev/null || true
