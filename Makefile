INSTALL=install
PREFIX=/usr

TARGET := paws

PKG_CONFIG ?= pkg-config

CFLAGS += -std=c99 -Wall
CFLAGS += $(shell $(PKG_CONFIG) --cflags libpulse)
LDFLAGS += $(shell $(PKG_CONFIG) --libs libpulse)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install:
	$(INSTALL) -Dm 755 $(TARGET) "$(DESTDIR)$(PREFIX)/bin/$(TARGET)"

clean:
	rm $(TARGET)

.PHONY: clean
