TARGET = basic.elf
OBJS = main.o

CFLAGS   += $(KOS_CFLAGS)
LDFLAGS  += $(KOS_LDFLAGS)
LIBS     := $(KOS_LIBS)

# The following two lines aren't required it seems
# CFLAGS += -I$(KOS_PORTS)/ALdc/inst/include
# LDFLAGS += -L$(KOS_PORTS)/ALdc/inst/lib/

#Link the lib like you would with KOS's libpng port
LIBS += -lALdc

#For some reason "-lm" must come after ALdc
LIBS += -lm -lkosutils

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.*

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	$(KOS_CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) romdisk.o $(OBJEXTRA) $(LIBS)

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)
