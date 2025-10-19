CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = kubsh
SOURCE = main.cpp
PREFIX = /usr
BINDIR = $(PREFIX)/bin
DEB_NAME = kubsh
VERSION = 1.0.0
ARCH = amd64

.PHONY: all build run install clean deb

all: build

build:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

run: build
	./$(TARGET)

install: build
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

deb: build
	mkdir -p $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN
	mkdir -p $(DEB_NAME)_$(VERSION)_$(ARCH)$(BINDIR)
	cp $(TARGET) $(DEB_NAME)_$(VERSION)_$(ARCH)$(BINDIR)/
	
	# Создаем control файл
	@echo "Package: $(DEB_NAME)" > $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Version: $(VERSION)" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Section: utils" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Priority: optional" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Architecture: $(ARCH)" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Depends: libc6 (>= 2.19)" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Maintainer: Your Name <your.email@example.com>" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Description: Custom Kubernetes Shell" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo " A custom shell implementation with Kubernetes features" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	
	# Собираем пакет
	dpkg-deb --build $(DEB_NAME)_$(VERSION)_$(ARCH)

clean:
	rm -f $(TARGET)
	rm -rf $(DEB_NAME)_*

test: build
	# Простые тесты для проверки функциональности
	@echo "Testing basic functionality..."
	@echo "echo 'test'" | timeout 2s ./$(TARGET) || true
	@echo "Testing exit command..."
	@echo "\\q" | timeout 2s ./$(TARGET) || true