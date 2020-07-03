
#CFLAGS += -DTEST

DESTLIB = ../../target/lib
DESTBIN = ../../target/usr/sbin
INC_KERNEL_PATH=$(TARGET_HOME)kernel/linux/include

CFLAGS  = -Os -s -Wall  -I$(INC_KERNEL_PATH) $(LIBNewPATH)

all: libnvram.so nvram

libnvram.so : nvram.o
	$(CC) -Os -s -shared -Wl,-soname,libnvram.so -o libnvram.so nvram.o 

nvram.o : nvram.c
	$(CC) $(CFLAGS) -fPIC -g -c $^ -o $@

install:
	install -D libnvram.so $(DESTLIB)/libnvram.so
	install -D nvram $(DESTBIN)/nvram
	$(STRIP) $(DESTLIB)/libnvram.so
	$(STRIP) $(DESTBIN)/nvram
	

nvram: nvram_bin.c nvram.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -rf *~ *.o *.so nvram 

