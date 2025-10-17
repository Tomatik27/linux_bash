CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11
TARGET = kubsh
SOURCES = src/main.cpp
PREFIX = usr/local
DESTDIR =

$(TARGET): $(SOURCES)
 $(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

 install: $(TARGET)
  install -d $(DESTDIR)$(PREFIX)/bin
  install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin

  clean:
   rm -f $(TARGET)

.PHONY: install clean
