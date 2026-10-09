CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
RUNTIME ?= ../fin-proto-runtime-bin-c
PROTO_DSL := proto/risk_v0.2.0.pdsl
OUTPUT_DIR := src/
FIN_PROTOC ?= $(HOME)/workspace/fin-protoc/bin/fin-protoc
BUILD := build
# bench links the generated library without tests.c (which has main())
BENCH_SRC := $(filter-out $(OUTPUT_DIR)tests.c,$(wildcard $(OUTPUT_DIR)*.c))

.PHONY: compile test bench clean all

all: test

compile:
	@$(FIN_PROTOC) -f $(PROTO_DSL) -C $(OUTPUT_DIR) >&2

test: compile
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -I$(RUNTIME)/include -I$(OUTPUT_DIR) $(OUTPUT_DIR)*.c $(RUNTIME)/src/*.c -o $(BUILD)/test
	./$(BUILD)/test

bench: compile
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -I$(RUNTIME)/include -I$(OUTPUT_DIR) $(BENCH_SRC) $(RUNTIME)/src/*.c bench/bench.c -o $(BUILD)/bench
	./$(BUILD)/bench

clean:
	rm -rf $(BUILD)
