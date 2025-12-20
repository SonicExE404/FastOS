KERNEL_SRC = kernel.c
BOOT_SRC = boot.s
KERNEL_OBJ = kernel.o
BOOT_OBJ = boot.o

KERNEL_BIN = kernel
ISO_DIR = Fast
ISO_NAME = Fast.iso

CC = gcc
NASM = nasm
LD = ld
GRUB = grub-mkrescue

all: $(ISO_NAME)

$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) -m32 -fno-stack-protector -fno-builtin -c $< -o $@

$(BOOT_OBJ): $(BOOT_SRC)
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJ) $(BOOT_OBJ)
	$(LD) -m elf_i386 -T linker.ld -o $@ $(BOOT_OBJ) $(KERNEL_OBJ)

$(ISO_NAME): $(KERNEL_BIN)
	mv $(KERNEL_BIN) $(ISO_DIR)/boot/kernel
	$(GRUB) -o $(ISO_NAME) $(ISO_DIR)
