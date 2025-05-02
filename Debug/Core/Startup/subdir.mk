################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/Startup/startup_stm32l496rgtx.s 

S_DEPS += \
./Core/Startup/startup_stm32l496rgtx.d 

OBJS += \
./Core/Startup/startup_stm32l496rgtx.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Startup/%.o: ../Core/Startup/%.s Core/Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -I"C:/Users/samrb/Downloads/Dashboard2025-f9fc1c9614cd094ea30e5a81c4da64a29998663a/Dashboard2025-f9fc1c9614cd094ea30e5a81c4da64a29998663a/Core/Src/Drivers" -I"C:/Users/samrb/Downloads/Dashboard2025-f9fc1c9614cd094ea30e5a81c4da64a29998663a/Dashboard2025-f9fc1c9614cd094ea30e5a81c4da64a29998663a/Core/Inc/Drivers" -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Core-2f-Startup

clean-Core-2f-Startup:
	-$(RM) ./Core/Startup/startup_stm32l496rgtx.d ./Core/Startup/startup_stm32l496rgtx.o

.PHONY: clean-Core-2f-Startup

