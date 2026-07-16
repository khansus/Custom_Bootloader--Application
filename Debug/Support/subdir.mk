################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Support/eth_support.c \
../Support/hash_file.c 

OBJS += \
./Support/eth_support.o \
./Support/hash_file.o 

C_DEPS += \
./Support/eth_support.d \
./Support/hash_file.d 


# Each subdirectory must supply rules for building sources it contributes
Support/%.o Support/%.su Support/%.cyclo: ../Support/%.c Support/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F767xx -c -I../Core/Inc -I"C:/Users/Hp/STM32CubeIDE/workspace_1.19.0/APPLICATION_01/Drivers/BSP/Components/lan8742" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Support

clean-Support:
	-$(RM) ./Support/eth_support.cyclo ./Support/eth_support.d ./Support/eth_support.o ./Support/eth_support.su ./Support/hash_file.cyclo ./Support/hash_file.d ./Support/hash_file.o ./Support/hash_file.su

.PHONY: clean-Support

