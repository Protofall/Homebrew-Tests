PROG = audio_2d_api
TARGET = $(PROG).elf
OBJS = main.o audio_assist.o

CFLAGS   += $(KOS_CFLAGS)
LDFLAGS  += $(KOS_LDFLAGS)
LIBS     := $(KOS_LIBS)

# The following two lines aren't required it seems
# CFLAGS += -I$(KOS_PORTS)/ALdc/inst/include
# LDFLAGS += -L$(KOS_PORTS)/ALdc/inst/lib/

#Link the lib like you would with KOS's libpng port
	#NOTE: We don't add -lpthreads because KOS has its own implementation
LIBS += -lALdc

#For some reason "-lm" must come after ALdc
LIBS += -lm -lkosutils

all: rm-elf $(TARGET)

#Compile for linux
linux:
	rm -f $(PROG)
	gcc -c -I/usr/local/include audio_assist.c -lopenal -lpthread -D_arch_unix
	gcc -c -I/usr/local/include main.c -lopenal -lpthread -D_arch_unix
	gcc -o $(PROG) main.o audio_assist.o -L/usr/local/lib -lopenal -lpthread

# gcc -o $(PROG) -lopenal -lpthread -D_arch_unix main.o audio_assist.o


include $(KOS_BASE)/Makefile.rules

clean:
	rm -f $(TARGET) $(OBJS) romdisk.*
	rm -f *.bin
	rm -f *.iso
	rm -f *.cdi
	rm -f $(PROG)

rm-elf:
	rm -f $(TARGET) romdisk.*

cdi: $(TARGET)
	../elf-to-cdi.sh $(PROG)

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
