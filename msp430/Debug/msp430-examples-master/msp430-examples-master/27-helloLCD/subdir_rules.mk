################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
msp430-examples-master/msp430-examples-master/27-helloLCD/%.obj: ../msp430-examples-master/msp430-examples-master/27-helloLCD/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/learnData/ccs/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv4 --code_state=32 --include_path="C:/Users/30628/workspace_v12/msp430" --include_path="D:/learnData/ccs/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="msp430-examples-master/msp430-examples-master/27-helloLCD/$(basename $(<F)).d_raw" --obj_directory="msp430-examples-master/msp430-examples-master/27-helloLCD" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


