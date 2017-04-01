OPENSRC_DIR=/home/project/openSrc
TARGET_ARCH=x86

ifeq ($(TARGET_ARCH), mips)
TOOLS_PATH=/opt/buildroot-gcc463/usr/bin/
CROSS_COMPILE	=$(TOOLS_PATH)mipsel-linux-
MY_LIB=-lbase463 $(SDK_PATH)/lib/libnvram/libnvram-0.9.28.so
  
SDK_PATH=/home/7620sdk/7688/sdk4300_20140916/RT288x_SDK/source
KERNEL_PATH =$(SDK_PATH)/linux-2.6.36.x
KERNEL_INC=-I $(SDK_PATH)/lib/libnvram/ -I $(KERNEL_PATH)/include/
OPEN_LIB=  -lz -lssl -lcrypto 

OPEN_INC=-I $(OPENSRC_DIR)/openssl-1.1.0e/platfrom/mips/include/ -DMIPS
CFLAGS = -Wall -I ./include $(KERNEL_INC) $(OPEN_INC) 

else ifeq ($(TARGET_ARCH), x86)
OPEN_INC=-I $(OPENSRC_DIR)/openssl-1.1.0e/platfrom/x86/include/ -DX86
CFLAGS = -Wall -I ./include $(OPEN_INC) 
OPEN_LIB=-lz -L $(OPENSRC_DIR)/openssl-1.1.0e/platfrom/x86/lib/ -lssl -lcrypto
endif

CC =$(CROSS_COMPILE)gcc
LDFLAGS= $(MY_LIB) $(OPEN_LIB) -lpthread     
TAR = testmd5

all +=md5example.o

export CC
$(TAR): $(all)
	$(CC) $(CFLAGS) -o $(TAR) $(all) $(LDFLAGS)
#	cp $(TAR) /nfs/yue/
#	cp $(TAR) /home/7620sdk/7688/sdk4300_20140916/RT288x_SDK/source/romfs/bin/
	$(RM) -f *.gch *.o *.bak $(all) 
	
%.o:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $< 

.PHONY: clean
clean:
	rm -f $(TAR) $(all) 
