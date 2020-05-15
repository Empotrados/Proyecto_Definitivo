################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
usblib/%.obj: ../usblib/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccs930/ccs/tools/compiler/ti-cgt-arm_18.12.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/ti/ccs930/ccs/tools/compiler/ti-cgt-arm_18.12.4.LTS/include" --include_path="C:/Users/p-l-b/git/Proyecto_Definitivo/3.Buttons-QT_TIVA" --include_path="C:/Users/p-l-b/git/Proyecto_Definitivo/3.Buttons-QT_TIVA/FreeRTOS/Source/include" --include_path="C:/Users/p-l-b/git/Proyecto_Definitivo/3.Buttons-QT_TIVA/FreeRTOS/Source/portable/CCS/ARM_CM4F" --define=ccs="ccs" --define=PART_TM4C123GH6PM --define=TARGET_IS_BLIZZARD_RB1 --define=UART_BUFFERED --define=WANT_CMDLINE_HISTORY --define=WANT_FREERTOS_SUPPORT -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="usblib/$(basename $(<F)).d_raw" --obj_directory="usblib" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


