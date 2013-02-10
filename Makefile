INSTALL=install
PREFIX=/usr

TARGET := paws

CFLAGS += -std=c99 -Wall
CFLAGS += `pkg-config --cflags libpulse`
LDFLAGS += `pkg-config --libs libpulse`

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install:
	$(INSTALL) -Dm 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm $(TARGET)

.PHONY: clean
