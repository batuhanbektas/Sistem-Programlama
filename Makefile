# tarsau projesi icin Makefile
# Kullanim:
#   make        -> tarsau calistirilabilirini uretir
#   make clean  -> uretilmis dosyalari temizler

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L
TARGET  = tarsau
SRCS    = tarsau.c
OBJS    = $(SRCS:.c=.o)
HDRS    = tarsau.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Her .c dosyasi icin .o uretimi; header'a bagimli
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
