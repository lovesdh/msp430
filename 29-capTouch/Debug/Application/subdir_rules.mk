################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Application/%.obj: ../Application/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"D:/learnData/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmsp --abi=eabi --use_hw_mpy=none --include_path="D:/learnData/ccs/ccs/ccs_base/msp430/include" --include_path="D:/learnData/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --include_path="C:/Users/30628/workspace_v12/29-capTouch/Library" --include_path="C:/Users/30628/workspace_v12/29-capTouch/Application" --advice:power=all -g --gcc --define=__MSP430G2553__ --display_error_number --diag_warning=225 --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="Application/$(basename $(<F)).d_raw" --obj_directory="Application" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


