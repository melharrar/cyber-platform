#
# Mikael Elharrar
#	December 26, 2017
#
# Makefile for masterAllocator process. 
#
ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = app_manager

# all source are stored in SRCS-y
SRCS-y := main.c
SRCS-y += app_conf.c
SRCS-y += allocator.c
#SRCS-y += process_manager.c

#CFLAGS += -O3
#CFLAGS += -O0
#CFLAGS += -g
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -I$(RTE_SDK)/../common/
CFLAGS += -I$(RTE_SDK)/../common/llist/
CFLAGS += -I$(RTE_SDK)/../common/json-c/include/
CFLAGS += -I$(RTE_SDK)/../common/log/

LDLIBS += -L$(RTE_SDK)/../common/llist/common/llist/$(RTE_TARGET)/lib/
LDLIBS += -lllist
LDLIBS += -L$(RTE_SDK)/../common/log/common/log/$(RTE_TARGET)/lib/
LDLIBS += -llog
LDLIBS += -L$(RTE_SDK)/../common/json-c/lib/
LDLIBS += -ljson-c

include $(RTE_SDK)/mk/rte.extapp.mk

