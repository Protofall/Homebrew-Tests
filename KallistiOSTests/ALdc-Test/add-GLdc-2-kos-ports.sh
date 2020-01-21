#!/bin/bash

mkdir -p /opt/toolchains/dc/kos-ports/GLdc/files
echo $'\n\
TARGET = libGLdc.a\n\
OBJS = GL/draw.o GL/flush.o GL/framebuffer.o GL/immediate.o GL/lighting.o GL/state.o GL/texture.o GL/glu.o\n\
OBJS += GL/matrix.o GL/fog.o GL/error.o GL/clip.o containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o\n\
\n\
KOS_CFLAGS += -Iinclude -ffast-math -O3\n\
\n\
include ${KOS_PORTS}/scripts/lib.mk\n\
' > /opt/toolchains/dc/kos-ports/GLdc/files/KOSMakefile.mk

echo $'\n\
PORTNAME = GLdc\n\
PORTVERSION = 1.0.0\n\
\n\
MAINTAINER = Luke Benstead <kazade@gmail.com>\n\
LICENSE = BSD 2-Clause "Simplified" License\n\
SHORT_DESC = Partial OpenGL 1.2 implementation for KOS\n\
\n\
DEPENDENCIES = \n\
\n\
GIT_REPOSITORY =    https://github.com/Kazade/GLdc.git\n\
\n\
TARGET =			libGLdc.a\n\
INSTALLED_HDRS =	include/gl.h include/glext.h include/glu.h include/glkos.h\n\
HDR_COMDIR =		GL\n\
\n\
KOS_DISTFILES =		KOSMakefile.mk\n\
\n\
include ${KOS_PORTS}/scripts/kos-ports.mk\n\
' > /opt/toolchains/dc/kos-ports/GLdc/Makefile

source /etc/bash.bashrc; cd /opt/toolchains/dc/kos-ports/GLdc && make install clean
