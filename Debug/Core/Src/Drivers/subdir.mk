################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Src/Drivers/ILI9341.cpp 

C_SRCS += \
../Core/Src/Drivers/TCAL9538RSVR.c 

C_DEPS += \
./Core/Src/Drivers/TCAL9538RSVR.d 

OBJS += \
./Core/Src/Drivers/ILI9341.o \
./Core/Src/Drivers/TCAL9538RSVR.o 

CPP_DEPS += \
./Core/Src/Drivers/ILI9341.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Drivers/%.o Core/Src/Drivers/%.su Core/Src/Drivers/%.cyclo: ../Core/Src/Drivers/%.cpp Core/Src/Drivers/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L496xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"C:/Users/samrb/STM32CubeIDE/workspace_1.16.0/Dashbaord2025/Core/Src/Drivers" -I"C:/Users/samrb/STM32CubeIDE/workspace_1.16.0/Dashbaord2025/Core/Inc/Drivers" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/Drivers/%.o Core/Src/Drivers/%.su Core/Src/Drivers/%.cyclo: ../Core/Src/Drivers/%.c Core/Src/Drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L496xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I"C:/Users/samrb/STM32CubeIDE/workspace_1.16.0/Dashbaord2025/Core/Src/Drivers" -I"C:/Users/samrb/STM32CubeIDE/workspace_1.16.0/Dashbaord2025/Core/Inc/Drivers" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Drivers

clean-Core-2f-Src-2f-Drivers:
	-$(RM) ./Core/Src/Drivers/ILI9341.cyclo ./Core/Src/Drivers/ILI9341.d ./Core/Src/Drivers/ILI9341.o ./Core/Src/Drivers/ILI9341.su ./Core/Src/Drivers/TCAL9538RSVR.cyclo ./Core/Src/Drivers/TCAL9538RSVR.d ./Core/Src/Drivers/TCAL9538RSVR.o ./Core/Src/Drivers/TCAL9538RSVR.su

.PHONY: clean-Core-2f-Src-2f-Drivers

