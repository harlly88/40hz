################################################################################
# MRS Version: 1.9.2
# 自动生成的文件。不要编辑！
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
40HZ.hex \

SECONDARY_LIST += \
40HZ.lst \

SECONDARY_SIZE += \
40HZ.siz \


# 所有目标
all: 40HZ.elf secondary-outputs

# 工具调用
40HZ.elf: $(OBJS) $(USER_OBJS)
	@	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized  -g -T "C:\vs-Program\罗门体验\40hz\软件\40HZ\Ld\Link.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,"40HZ.map" --specs=nano.specs --specs=nosys.specs -o "40HZ.elf" $(OBJS) $(USER_OBJS) $(LIBS) -lprintf
	@	@
40HZ.hex: 40HZ.elf
	@	riscv-none-embed-objcopy -O ihex "40HZ.elf"  "40HZ.hex"
	@	@
40HZ.lst: 40HZ.elf
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "40HZ.elf" > "40HZ.lst"
	@	@
40HZ.siz: 40HZ.elf
	@	riscv-none-embed-size --format=berkeley "40HZ.elf"
	@	@
# 其他目标
clean:
	-$(RM) $(ASM_UPPER_DEPS)$(OBJS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(C_DEPS) 40HZ.elf
	-@
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
