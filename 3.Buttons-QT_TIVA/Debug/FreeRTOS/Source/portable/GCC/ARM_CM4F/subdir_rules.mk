################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/Source/portable/GCC/ARM_CM4F/%.obj: ../FreeRTOS/Source/portable/GCC/ARM_CM4F/%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"/opt/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/opt/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --include_path="/home/nhr/workspace_v8/base-QT_TIVA" --include_path="/home/nhr/workspace_v8/base-QT_TIVA/FreeRTOS/Source/include" --include_path="/home/nhr/workspace_v8/base-QT_TIVA/FreeRTOS/Source/portable/CCS/ARM_CM4F" --define=ccs="ccs" --define=PART_TM4C123GH6PM --define=TARGET_IS_BLIZZARD_RB1 --define=UART_BUFFERED --define=WANT_CMDLINE_HISTORY --define=WANT_FREERTOS_SUPPORT -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="FreeRTOS/Source/portable/GCC/ARM_CM4F/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS/Source/portable/GCC/ARM_CM4F" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


