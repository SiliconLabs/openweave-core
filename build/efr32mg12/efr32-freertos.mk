#
#    Copyright (c) 2019 Google LLC.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#   @file
#         Component makefile for incorporating FreeRTOS into an EFR32
#         application.
#

#
#   This makefile is intended to work in conjunction with the efr32-app.mk
#   makefile to build the OpenWeave example applications on Silicon Labs platforms. 
#   EFR32 applications should include this file in their top level Makefile
#   along with the other makefiles in this directory.  E.g.:
#
#       PROJECT_ROOT = $(realpath .)
#
#       BUILD_SUPPORT_DIR = $(PROJECT_ROOT)/third_party/openweave-core/build/efr32
#       
#       include $(BUILD_SUPPORT_DIR)/efr32-app.mk
#       include $(BUILD_SUPPORT_DIR)/efr32-openweave.mk
#       include $(BUILD_SUPPORT_DIR)/efr32-openthread.mk
#       include $(BUILD_SUPPORT_DIR)/efr32-freertos.mk
#
#       PROJECT_ROOT := $(realpath .)
#       
#       APP := openweave-efr32-bringup
#       
#       SRCS = \
#           $(PROJECT_ROOT)/main.cpp \
#           ...
#
#       $(call GenerateBuildRules)
#       

STD_INC_DIRS += \
    $(FREERTOS_ROOT)/Source/include/
    
$(OUTPUT_DIR)/freertos/croutine.c.o : $(FREERTOS_ROOT)/Source/croutine.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/croutine.c -o $(OUTPUT_DIR)/freertos/croutine.c.o

$(OUTPUT_DIR)/freertos/list.c.o     : $(FREERTOS_ROOT)/Source/list.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/list.c -o $(OUTPUT_DIR)/freertos/list.c.o

$(OUTPUT_DIR)/freertos/queue.c.o    : $(FREERTOS_ROOT)/Source/queue.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/queue.c -o $(OUTPUT_DIR)/freertos/queue.c.o
	
$(OUTPUT_DIR)/freertos/event_groups.c.o    : $(FREERTOS_ROOT)/Source/event_groups.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/event_groups.c -o $(OUTPUT_DIR)/freertos/event_groups.c.o

$(OUTPUT_DIR)/freertos/tasks.c.o    : $(FREERTOS_ROOT)/Source/tasks.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/tasks.c -o $(OUTPUT_DIR)/freertos/tasks.c.o

$(OUTPUT_DIR)/freertos/timers.c.o   : $(FREERTOS_ROOT)/Source/timers.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/timers.c -o $(OUTPUT_DIR)/freertos/timers.c.o

$(OUTPUT_DIR)/freertos/port.c.o     : $(FREERTOS_ROOT)/Source/portable/GCC/ARM_CM3/port.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/portable/GCC/ARM_CM3/port.c -o $(OUTPUT_DIR)/freertos/port.c.o

$(OUTPUT_DIR)/freertos/heap_3.c.o     : $(FREERTOS_ROOT)/Source/portable/MemMang/heap_3.c $(FREERTOSCONFIG_DIR)/FreeRTOSConfig.h
	$(CC) -c  $(STD_CFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(DEFINE_FLAGS) $(INC_FLAGS) $(FREERTOS_ROOT)/Source/portable/MemMang/heap_3.c -o $(OUTPUT_DIR)/freertos/heap_3.c.o

FREERTOS_OBJECTS := \
    $(OUTPUT_DIR)/freertos/croutine.c.o \
    $(OUTPUT_DIR)/freertos/list.c.o \
    $(OUTPUT_DIR)/freertos/queue.c.o \
    $(OUTPUT_DIR)/freertos/event_groups.c.o \
    $(OUTPUT_DIR)/freertos/tasks.c.o \
    $(OUTPUT_DIR)/freertos/timers.c.o \
    $(OUTPUT_DIR)/freertos/port.c.o \
    $(OUTPUT_DIR)/freertos/heap_3.c.o


# Add FreeRTOSBuildRules to the list of late-bound build rules that
# will be evaluated when GenerateBuildRules is called. 
LATE_BOUND_RULES += FreeRTOSBuildRules

# Rules for configuring, building and installing FreeRTOS from source.
define FreeRTOSBuildRules

$(OUTPUT_DIR)/freertos/libfreertos.a : $(OUTPUT_DIR)/freertos $(FREERTOS_OBJECTS)
	arm-none-eabi-ar rcs $(OUTPUT_DIR)/freertos/libfreertos.a $(FREERTOS_OBJECTS)

.phony: $(OUTPUT_DIR)/freertos
$(OUTPUT_DIR)/freertos :
	-mkdir -p $(OUTPUT_DIR)/freertos

.phony: build-freertos
build-freertos : $(OUTPUT_DIR)/freertos/libfreertos.a

.phony: install-freertos
install-freertos: $(OUTPUT_DIR)/freertos
	cp -r $(FREERTOS_ROOT)/Source/include $(OUTPUT_DIR)/freertos/ 

.phony: clean-freertos
clean-freertos:
	-rm -rf $(OUTPUT_DIR)/freertos

endef


# ==================================================
# FreeRTOS-specific help definitions
# ==================================================

define TargetHelp +=

  build-freertos        Build the OpenWeave libraries.
  
  install-freertos      Install FreeRTOS libraries and headers in 
                        build output directory for use by application.
  
  clean-freertos        Clean all build outputs produced by the FreeRTOS
                        build process.
endef




