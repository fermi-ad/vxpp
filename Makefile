# $Id: Makefile,v 1.11 2015/01/07 15:31:56 neswold Exp $

VID = 2.1
PRODUCT = 1

SUPPORTED_VERSIONS = 61 64 67

HEADER_TARGETS = vwpp.h
MOD_TARGETS = vwpp.out
LIB_TARGETS = libvwpp.a

include $(PRODUCTS_INCDIR)frontend-latest.mk

OBJS = sem.o queue.o task.o util.o

vwpp.out : ${OBJS}
	${make-mod-munch}

libvwpp.a : ${OBJS}
	${make-lib}
