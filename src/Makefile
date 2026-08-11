RIV_HOME ?= $(HOME)/.riv
RIVEMU ?= $(RIV_HOME)/rivemu
RIVEMU_SDK ?= $(RIV_HOME)/rivos-sdk.ext2

.PHONY: all clean run smoke

all: bombflip.sqfs

bombflip: bombflip.c seqt.h
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk -workspace -mem=1024 \
		-exec "gcc bombflip.c -o bombflip -lriv -lm \$$(riv-opt-flags -Ospeed)"

bombflip.sqfs: bombflip songs/gameplay.rivcard info.json cover.png
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -quiet -no-window -sdk -workspace \
		-exec "riv-mksqfs bombflip songs info.json cover.png bombflip.sqfs"

run: bombflip.sqfs
	$(RIVEMU) -cartridge=bombflip.sqfs

smoke: bombflip.sqfs
	$(RIVEMU) -quiet -no-window -no-audio -stop-frame=180 -cartridge=bombflip.sqfs

clean:
	rm -f bombflip bombflip.sqfs
