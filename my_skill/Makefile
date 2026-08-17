# my_skill — MPX-Dog WASM Skill
# Convenience wrapper around mpx-cli.

MPX_HOST ?= 192.168.2.1
SRC := src/my_skill.c
WASM := src/my_skill.wasm

.PHONY: build validate upload run clean

build:
	mpx-cli build $(SRC)

validate:
	mpx-cli build $(SRC) --validate

upload:
	mpx-cli upload $(WASM) --host $(MPX_HOST)

run:
	mpx-cli run my_skill.wasm --host $(MPX_HOST)

clean:
	rm -f src/*.wasm
