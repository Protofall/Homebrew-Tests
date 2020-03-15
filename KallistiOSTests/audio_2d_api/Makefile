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

all: clean $(TARGET)

#Compile for linux
linux:
	rm -f $(PROG)
	gcc -c -I/usr/local/include audio_assist.c -lopenal -lpthread
	gcc -c -I/usr/local/include main.c -lopenal -lpthread
	gcc -o $(PROG) main.o audio_assist.o -L/usr/local/lib -lopenal -lpthread

linux-mojoal:
	rm -f $(PROG)
	gcc -c -I/usr/include/SDL2 -ImojoAL mojoal.c -lSDL2
	gcc -c -ImojoAL audio_assist.c -lpthread
	gcc -c -ImojoAL main.c -lpthread
	gcc -o $(PROG) main.o audio_assist.o mojoal.o -L/usr/local/lib -lpthread -lSDL2

include $(KOS_BASE)/Makefile.rules

clean:
	rm -f $(TARGET) romdisk.*
	rm -f *.bin
	rm -f *.iso
	rm -f *.cdi
	rm -f $(PROG)
	rm -f *.o

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
