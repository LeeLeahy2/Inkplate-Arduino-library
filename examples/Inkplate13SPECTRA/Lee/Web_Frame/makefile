######################################################################
# makefile
#
# Robots-For-All (R4A)
# Build the example sketch
######################################################################

.ONESHELL:
SHELL=/bin/bash

##########
# Source files
##########

SKETCH=Web_Frame
VENDOR=soldered-inkplate-boards
ARCHITECTURE=esp32
BOARD_NAME=Inkplate13SPECTRA
BOARD_FQBN=$(VENDOR):$(ARCHITECTURE):$(BOARD_NAME)
BOARD_PATH=$(VENDOR).$(ARCHITECTURE).$(BOARD_NAME)
PARTITION_FILE_NAME=huge_app

ESP_CORE_VERSION=3.2.0
ESPTOOL_VERSION=4.9.dev3
IDF_VERSION=idf-release_v5.4-2f7dcd86-v1

##########
# OS specific paths
##########

ifeq ($(OS),Windows_NT)
#---------
# Windows NT generic paths
#---------
OS_NAME=Windows NT

USER_DIRECTORY_PATH=C:\Users\$(USERNAME)\
EXAMPLE_PATH=..\

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Documents\Arduino\
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries\
BUILD_PATH=build\
BIN_PATH=$BUILD_PATH)$(BOARD_PATH)\
BOOT_LOADER_PATH=$(ARDUINO_LIBRARY_PATH)R4A_ESP32\binaries\
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH)AppData\Local\Arduino15\packages\esp32\
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools\esp32-arduino-libs\$(IDF_VERSION)\
ESPTOOL_PATH=$(HOME_BOARD_PATH)\.arduino15\packages\esp32\tools\esptool_py\$(ESPTOOL_VERSION)\

# Windows NT utilities
ARDUINO_CLI=~/Arduino/arduino-cli
CLEAR=cls
COPY=copy
DELETE=rmdir /s
DIR_LISTING=dir
TERMINAL_APP=

TERMINAL_PORT=COM3
TERMINAL_PARAMS=

else
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
#---------
# Mac generic paths
#---------
OS_NAME=Mac OS

USER_DIRECTORY_PATH=($HOME)/
EXAMPLE_PATH=../

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Documents/Arduino/
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries/
BUILD_PATH=build/
BIN_PATH=$(BUILD_PATH)$(BOARD_PATH)/
BOOT_LOADER_PATH=$(ARDUINO_LIBRARY_PATH)R4A_ESP32/binaries/
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH)AppData/Local/Arduino15/packages/esp32/
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools/esp32-arduino-libs/$(IDF_VERSION)/
ESPTOOL_PATH=$(HOME_BOARD_PATH)tools/esptool_py/$(ESPTOOL_VERSION)/

# Mac utilities
ARDUINO_CLI=arduino-cli
CLEAR=clear
COPY=cp
DELETE=rmdir /s
DIR_LISTING=ls
TERMINAL_APP=minicom

TERMINAL_PORT=COM3
TERMINAL_PARAMS=-b 115200 -8 -D $(TERMINAL_PORT) < /dev/tty

else
#---------
# Linux generic paths
#---------
OS_NAME=Linux

USER_DIRECTORY_PATH=~/
EXAMPLE_PATH=../

ARDUINO_PATH=$(USER_DIRECTORY_PATH)Arduino/
ARDUINO_LIBRARY_PATH=$(ARDUINO_PATH)libraries/
BUILD_PATH=build/
BIN_PATH=$(BUILD_PATH)$(BOARD_PATH)/
BOOT_LOADER_PATH=$(BUILD_PATH)$(BOARD_PATH)/
HOME_BOARD_PATH=$(USER_DIRECTORY_PATH).arduino15/packages/esp32/
ESP_IDF_PATH=$(HOME_BOARD_PATH)tools/esp32-arduino-libs/$(IDF_VERSION)/
ESPTOOL_PATH=$(USER_DIRECTORY_PATH).arduino15/packages/esp32/tools/esptool_py/$(ESPTOOL_VERSION)/

# Linux utilities
ARDUINO_CLI=$(USER_DIRECTORY_PATH)Arduino/arduino-cli
CLEAR=clear
COPY=cp
DELETE=rm -Rf
DIR_LISTING=ls
TERMINAL_APP=minicom

TERMINAL_PORT=/dev/ttyUSB0
TERMINAL_PARAMS=-b 115200 -8 < /dev/tty

endif
endif

#---------
# OS Independent
#---------

# Files
BIN_FILE=$(BIN_PATH)$(SKETCH).ino.merged.bin

##########
# Buid all the sources - must be first
##########

EXECUTABLES += $(BIN_FILE)

.PHONY: all

all: $(EXECUTABLES)

.PHONY: Car

Car:	$(BIN_FILE)

##########
# Configure the Arduino environment
##########

.PHONY: arduino-config

arduino-config:
	$(ARDUINO_CLI) config init --overwrite --additional-urls "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json,https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json"

##########
# Get the ESP32 core
##########

.PHONY: core-update

core-update:
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install esp32:esp32@$(ESP_CORE_VERSION)

########
# Robot Firmware
##########

#DEBUG_LEVEL=verbose
#DEBUG_LEVEL=debug
#DEBUG_LEVEL=warn
#DEBUG_LEVEL=error
DEBUG_LEVEL=none

$(BIN_FILE):	$(SKETCH).ino   *.ino   *.h   makefile
	@$(CLEAR)
	@echo "------------------------- $(OS_NAME) ---------------------------------------------"
	@echo Building $(SKETCH) for $(BOARD_PATH)
	$(ARDUINO_CLI)  \
		compile  \
		--fqbn   "$(BOARD_FQBN)":DebugLevel=$(DEBUG_LEVEL),PartitionScheme=$(PARTITION_FILE_NAME),FlashMode=qio,UploadSpeed=921600,EraseFlash=none  \
		$<  \
		--build-property build.partitions=$(PARTITION_FILE_NAME)  \
		--build-property upload.maximum_size=3145728  \
		--export-binaries

##########
# Upload the firmware
##########

.PHONY: upload

upload:	$(BIN_FILE)
	$(ESPTOOL_PATH)esptool \
		--chip   esp32s3 \
		--port   $(TERMINAL_PORT) \
		--baud   921600 \
		--before   default_reset \
		--after   hard_reset \
		write_flash \
		--force \
		--flash_freq 80m \
		--flash_size detect \
		--compress \
		      0   $<
	$(TERMINAL_APP) -D $(TERMINAL_PORT) $(TERMINAL_PARAMS)

##########
# Analyze a system crash using the MAP file
##########

.PHONY: crash

READ_MAP_FILE_PATH=$(USER_DIRECTORY_PATH)SparkFun/rc/RTK/Firmware/Tools/

crash: $(BIN_FILE)
	$(READ_MAP_FILE_PATH)Read_Map_File  $(BIN_PATH)$(SKETCH).ino.map < /dev/tty

##########
# Open the terminal (tty)
##########

.PHONY: terminal

terminal:
	$(TERMINAL_APP) -D $(TERMINAL_PORT) $(TERMINAL_PARAMS)

########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -Rf $(BUILD_PATH)
