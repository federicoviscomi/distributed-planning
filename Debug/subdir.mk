################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../dplan.c \
../dserver.c \
../error_check.c \
../lbase.c \
../lcscom.c \
../llist.c \
../planners_table.c \
../pthread_mem.c 

C_DEPS += \
./dplan.d \
./dserver.d \
./error_check.d \
./lbase.d \
./lcscom.d \
./llist.d \
./planners_table.d \
./pthread_mem.d 

OBJS += \
./dplan.o \
./dserver.o \
./error_check.o \
./lbase.o \
./lcscom.o \
./llist.o \
./planners_table.o \
./pthread_mem.o 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


