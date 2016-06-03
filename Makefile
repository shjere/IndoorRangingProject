
PROJ_NAME=DecaRangeF3_test
OUTPATH=build

# Location of the Libraries folder from the STM32F0xx Standard Peripheral Library
STD_PERIPH_LIB=./common/stm32f3lib/Libraries

# Location of the linker scripts
LDSCRIPT_INC=./common/ldscripts

######################## SOURCE DIRECTORIES ################
SRC_DIR = .
SRC_DIR += ./strlib/src #uncomment for ranging program
SRC_DIR += ./ntb/src 
SRC_DIR += ./common/decaranging/decadriver/src #uncomment for ranging program
SRC_DIR += ./common/decaranging/ranging/src  #uncomment for ranging program
SRC_DIR += ./dw_platform/src  #uncomment for ranging program
SRC_DIR += $(STD_PERIPH_LIB)/STM32F30x_StdPeriph_Driver/src
SRC_DIR += $(STD_PERIPH_LIB)/CMSIS/Device/ST/STM32F30x/Source/Templates

######################## INCLUDE DIRECTORIES ###############
INC_DIR = .
INC_DIR += ./strlib/inc  #uncomment for ranging program
INC_DIR += ./ntb/inc 
INC_DIR += ./common/decaranging/decadriver/inc #uncomment for ranging program
INC_DIR += ./common/decaranging/ranging/inc  #uncomment for ranging program
INC_DIR += ./dw_platform/inc  #uncomment for ranging program 
INC_DIR += $(STD_PERIPH_LIB)/CMSIS/Device/ST/STM32F30x/Include
INC_DIR += $(STD_PERIPH_LIB)/CMSIS/Include
INC_DIR += $(STD_PERIPH_LIB)/STM32F30x_StdPeriph_Driver/inc 

#################### ADDING C FILES ########################
SRCS = $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c' -printf "%f\n")


# that's it, no need to change anything below this line!

###################################################

CC=arm-none-eabi-gcc
GDB=arm-none-eabi-gdb
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size

CFLAGS  = -Wall -g -std=c99 -Os  
CFLAGS += -mlittle-endian -mcpu=cortex-m4  -march=armv7e-m -mthumb
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -lm

LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(PROJ_NAME).map

#############################################################

#vpath %.a $(STD_PERIPH_LIB)

# PATH SETUP
vpath %.c $(SRC_DIR)

ROOT=$(shell pwd)

# Includes
CFLAGS += $(foreach d, $(INC_DIR), -I$d) 
#CFLAGS += -DSTM32F3
#CFLAGS += -I$(DEFS)

CFLAGS += -include $(STD_PERIPH_LIB)/stm32f30x_conf.h

###################### Adding STARTUP file ########################
STARTUP = ./common/startup/startup_stm32f30x.s 


#OBJS = $(addprefix objs/,$(SRCS:.c=.o))
#OBJS = $(addprefix decadriver/,$(SRCS:.c=.o))
#OBJS = $(SRCS:.c=.o)
#DEPS = $(addprefix deps/,$(SRCS:.c=.d))

###################################################################

.PHONY: lib proj dir_tree

all: dir_tree proj 

 -include $(DEPS)

dir_tree: 
		mkdir -p $(OUTPATH) 


proj: 	$(OUTPATH)/$(PROJ_NAME).elf
			$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf


$(OUTPATH)/$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(STARTUP) -L$(STD_PERIPH_LIB) -lstm32f3 -L$(LDSCRIPT_INC) -Tstm32f3.ld
	$(OBJCOPY) -O ihex $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).hex
	$(OBJCOPY) -j .isr_vector -j .text -O binary $(OUTPATH)/$(PROJ_NAME).elf $(OUTPATH)/$(PROJ_NAME).bin
	$(OBJDUMP) -St $(OUTPATH)/$(PROJ_NAME).elf >$(OUTPATH)/$(PROJ_NAME).lst
	$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf

clean:
	find ./ -name '*~' | xargs rm -f	
	rm -f objs/*.o
	rm -f deps/*.d
	find . -name \*.o -type f -delete
	find . -name \*.lst -type f -delete
	rm -f dirs
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).hex
	rm -f $(PROJ_NAME).bin
	rm -f $(PROJ_NAME).map
	rm -f $(PROJ_NAME).lst
	rm -f $(OUTPATH)/$(PROJ_NAME).elf
	rm -f $(OUTPATH)/$(PROJ_NAME).hex
	rm -f $(OUTPATH)/$(PROJ_NAME).bin
	rm -f $(OUTPATH)/$(PROJ_NAME).map
	rm -f $(OUTPATH)/$(PROJ_NAME).lst

reallyclean: clean
	$(MAKE) -C $(STD_PERIPH_LIB) clean

# Flash the STM32F3 via USB DFU
dfu:
	dfu-util -v -d 0483:df11 -a 0 -s 0x08000000 -D $(OUTPATH)/$(PROJ_NAME).bin
