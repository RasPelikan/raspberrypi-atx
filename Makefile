PROJECT_NAME := raspberrypi-atx

export PROJECT_NAME
#MAKEFILE_NAME := $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) ) 

MK := mkdir
RM := rm -rf

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO := 
else
NO_ECHO := @
endif

GNU_PREFIX := avr

# Toolchain commands
CC              := $(GNU_PREFIX)-gcc
TEST_CC         := gcc
LD              := $(GNU_PREFIX)-ld
OBJDUMP         := $(GNU_PREFIX)-objdump
OBJCOPY         := $(GNU_PREFIX)-objcopy
SIZE            := $(GNU_PREFIX)-size
AVRDUDE			:= avrdude

MCU		:= attiny45
CLOCK_SPEED	:= 8000000UL
AVRDUDE_MCU	:= t45
AVRDUDE_PROGRAMMER	:= usbasp

#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

#source common to all targets
C_SOURCE_FILES += \
$(abspath src/main/c/main.c) \
$(abspath src/main/c/util.c)

#includes common to all targets
INC_PATHS = -I$(abspath ./src/main/c)

OBJECT_DIRECTORY = target
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) )

#flags common to all targets
CFLAGS += -Wall
CFLAGS += -Os
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -std=gnu99
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -mmcu=$(MCU)
CFLAGS += -DF_CPU=$(CLOCK_SPEED)

# keep every function in separate section. This will allow linker to dump unused functions
LDFLAGS += -Wl,-Map,$(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).map
LDFLAGS += -mmcu=$(MCU)

default: $(PROJECT_NAME)

all: $(PROJECT_NAME)

C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

vpath %.c $(C_PATHS)

OBJECTS = $(C_OBJECTS)

$(PROJECT_NAME): $(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).hex

## Create build directories
$(BUILD_DIRECTORIES):
	$(MK) $@

$(OBJECT_DIRECTORY)/%.o: src/main/c/%.c
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(TEST_AVR_INC_PATHS) $(INC_PATHS) -c -o $@ $<

$(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).elf: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $@
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

## Create binary .hex file from the .elf file
$(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).elf
	@echo Preparing: $(PROJECT_NAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).hex
	-@echo ''
	$(NO_ECHO)$(SIZE) --format=avr --mcu=$(MCU) $(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).elf
	-@echo ''

deploy: $(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).hex
	$(AVRDUDE) -p$(AVRDUDE_MCU) -c$(AVRDUDE_PROGRAMMER) -Uflash:w:$(OUTPUT_BINARY_DIRECTORY)/$(PROJECT_NAME).hex
	
readfuses:
	$(AVRDUDE) -p$(AVRDUDE_MCU) -c$(AVRDUDE_PROGRAMMER)
	
writefuses:
	$(AVRDUDE) -p$(AVRDUDE_MCU) -c$(AVRDUDE_PROGRAMMER) -Ulfuse:w:0xE2:m
	$(AVRDUDE) -p$(AVRDUDE_MCU) -c$(AVRDUDE_PROGRAMMER) -Uhfuse:w:0xDF:m
	$(AVRDUDE) -p$(AVRDUDE_MCU) -c$(AVRDUDE_PROGRAMMER) -Uefuse:w:0xFF:m

clean:
	$(RM) $(BUILD_DIRECTORIES)

cleanobj:
	$(RM) $(BUILD_DIRECTORIES)/*.o
