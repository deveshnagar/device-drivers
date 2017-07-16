
EMPTY_VAR=
# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE), $(EMPTY_VAR))

    KERNEL_SOURCE := /home/devesh/workspace/mywork/radxa/kernel/linux-3.18.11
    PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else
	src-m := $(notdir $(wildcard ${SUBDIRS}/*.c))
	src-mod := $(notdir $(wildcard ${SUBDIRS}/*.mod.c))
	src-m := $(filter-out ${src-mod}, ${src-m})
	obj-m := $(src-m:.c=.o)
endif

#
#This is third way to compile, just create a make file with this line :
#obj-m := simple.o
#And compile using this command :
#make -C ../../../kernel/linux-3.18.11 M=`pwd` modules
#make -C ../../../kernel/linux-3.18.11 M=`pwd` modules clean
#
