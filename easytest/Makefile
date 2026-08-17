# easytest — MPX-Dog WASM Skill
# Convenience wrapper around mpx-cli.

# Override the robot address: make deploy MPX_HOST=10.0.0.5
export MPX_HOST ?= 192.168.2.1
SRC := src/easytest.c
WASM := build/easytest.wasm

.PHONY: deploy build validate upload run clean

# The usual loop: build, upload and run in one step. Paths come from
# manifest.json, so nothing here needs the skill name.
deploy:
	mpx-cli deploy

build:
	mpx-cli build

validate:
	mpx-cli build --validate

upload:
	mpx-cli upload

run:
	mpx-cli run

clean:
	rm -f build/*.wasm
