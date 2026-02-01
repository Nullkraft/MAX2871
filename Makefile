SHELL := /bin/sh
.DEFAULT_GOAL := ci

TOOLS_DIR := $(CURDIR)/.tools
BIN_DIR   := $(TOOLS_DIR)/bin
ARDUINO_CLI_BIN := $(BIN_DIR)/arduino-cli

# Prefer project-local tools if present
export PATH := $(BIN_DIR):$(PATH)

.PHONY: banner tools check-arduino-cli doctor test-native arduino-uno ci

banner:
	@echo
	@echo "     ┌───────────────────────────────────────────────────────────────────────┐"
	@echo "     │                               NEW BUILD                               │"
	@echo "     └───────────────────────────────────────────────────────────────────────┘"

# Explicit install target (only runs when you call `make tools`)
tools: banner
	@mkdir -p $(BIN_DIR)
	@echo "Installing Arduino CLI into $(BIN_DIR)..."
	@{ \
	  if command -v curl >/dev/null 2>&1; then \
	    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh; \
	  elif command -v wget >/dev/null 2>&1; then \
	    wget -qO- https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh; \
	  else \
	    echo "Error: need curl or wget to install Arduino CLI"; exit 1; \
	  fi; \
	} | BINDIR="$(BIN_DIR)" sh
	@echo "Installed: $(ARDUINO_CLI_BIN)"

check-arduino-cli: banner
	@command -v arduino-cli >/dev/null 2>&1 || { \
	  echo "ERROR: arduino-cli not found on PATH."; \
	  echo "Run: make tools"; \
	  echo "Or install Arduino CLI system-wide, then retry."; \
	  exit 1; \
	}

check-pio: banner
	@command -v pio >/dev/null 2>&1 || { \
	  echo "ERROR: pio (PlatformIO) not found on PATH."; \
	  echo "Install PlatformIO, then retry."; \
	  exit 1; \
	}

doctor: banner check-arduino-cli check-pio
	arduino-cli version
	pio --version

test-native: banner check-pio
	pio test -e native -v

arduino-uno: banner check-arduino-cli
	@echo
	@echo "arduino-uno built using Arduino CLI (arduino-cli)"
	arduino-cli core install arduino:avr
	arduino-cli compile --fqbn arduino:avr:uno --library . examples/ci_smoke

ci: arduino-uno test-native
