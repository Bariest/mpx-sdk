MPX_HOST ?= 192.168.2.1
MPX_PORT ?= 80
SONG     ?= dance_example
SRC_DIR  := src
INC_DIR  := include
WASM     := $(SRC_DIR)/$(SONG).wasm
WASI_CC  ?= /opt/wasi-sdk/bin/clang
CFLAGS   := --target=wasm32-wasip1 -nostartfiles -Oz -Wl,--no-entry -Wl,--export=on_start -Wl,--import-undefined -I$(INC_DIR)
.PHONY: all build validate upload run clean codegen help
all: build
build: $(WASM)
$(WASM): $(SRC_DIR)/$(SONG).c $(INC_DIR)/moves.h $(INC_DIR)/mpx_host.h $(SRC_DIR)/timetable.h
	@echo "🔨 Building $(SONG)..."
	$(WASI_CC) $(CFLAGS) -o $(WASM) $(SRC_DIR)/$(SONG).c
	@ls -lh $(WASM)
	@echo "✅ Built $(WASM)"
validate: build
	wasm-validate $(WASM) && echo "✅ Valid" || echo "❌ Invalid"
upload: build
	mpx-cli upload $(WASM) --ip $(MPX_HOST) --port $(MPX_PORT)
run: upload
	mpx-cli run $(SONG).wasm --ip $(MPX_HOST) --port $(MPX_PORT)
clean:
	rm -f $(SRC_DIR)/*.wasm
codegen:
	python3 codegen.py generate $(INPUT) --genre $(GENRE) --song "$(SONG)" --output .
help:
	@echo "make SONG=my_dance build|validate|upload|run"
	@echo "make INPUT=slots.json GENRE=pop SONG='Name' codegen"
