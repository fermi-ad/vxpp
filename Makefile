# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# NOTE: When you change the VID, you must change the version number
#       used in vwpp.h. Search for __BUILDING_VWPP.
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

VID = 2.4
PRODUCT = 1

SUPPORTED_VERSIONS = 64 67

HEADER_TARGETS = vwpp.h vwpp_types.h
MOD_TARGETS = vwpp.out
LIB_TARGETS = libvwpp.a

include $(PRODUCTS_INCDIR)frontend-latest.mk

ADDED_C++FLAGS = -D__BUILDING_VWPP

OBJS = sem.o queue.o task.o util.o

${OBJS} : ${HEADER_TARGETS}

vwpp.out : ${OBJS}
	${make-mod-munch}

libvwpp.a : ${OBJS}
	${make-lib}
