# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# NOTE: When you change the VID, you must change the version number
#       used in all the header files.
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

VID = 2.7
PRODUCT = 1

SUPPORTED_VERSIONS = 64 67

HEADER_TARGETS = vwpp.h vwpp_types.h vwpp_memory.h
MOD_TARGETS = vwpp.out
LIB_TARGETS = libvwpp.a

include $(PRODUCTS_INCDIR)frontend-2.10.mk

ADDED_C++FLAGS += -D__BUILDING_VWPP

OBJS = sem.o queue.o task.o util.o

${OBJS} : ${HEADER_TARGETS}

vwpp.out : ${OBJS}
	${make-mod-munch}

libvwpp.a : ${OBJS}
	${make-lib}
