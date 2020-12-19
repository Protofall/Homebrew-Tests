
TARGET = userclip.elf

OBJS =  main.o \
	romdisk.o
	
KOS_ROMDISK_DIR = romdisk_boot

all: rm-elf $(TARGET)

#NODEBUGING = -D NDEBUG
NODEBUGING = 
EXTRAOPT=-Wall -Wextra -O2 -mpretend-cmove -mrelax -mfused-madd -fomit-frame-pointer -Wstrict-aliasing=2 -Wno-format
MATHOPT=-ffast-math
ALIGNS=-falign-loops=4 -falign-labels=4 -falign-functions=32 -falign-jumps=4

%.o: %.c
	kos-cc $(CFLAGS) $(NODEBUGING) $(MATHOPT) $(EXTRAOPT) $(ALIGNS) -c $< -o $@

%.o: %.cpp
	kos-c++ $(CFLAGS) $(NODEBUGING) -fno-exceptions -fno-rtti -D __CPLUSPLUS $(MATHOPT)  $(EXTRAOPT) $(CXXFLAGS) $(ALIGNS) -c $< -o $@

%.o: %.s
	kos-as $< -o $@

%.o: %.S
	kos-cc -c $(NODEBUGING) -D _ASSEM $< -o $@

	

subdirs: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean_subdirs: $(patsubst %, _clean_dir_%, $(SUBDIRS))

$(patsubst %, _clean_dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

# Define KOS_ROMDISK_DIR in your Makefile if you want these two handy rules.
ifdef KOS_ROMDISK_DIR
romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d $(KOS_ROMDISK_DIR) -v -x .svn

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o
endif
######################################

clean:
	rm -f $(TARGET) $(OBJS) romdisk.img

rm-elf:
	rm -f $(TARGET)


$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA)  -lstdc++ -lpng -lm -lz $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -n

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
