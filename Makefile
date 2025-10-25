CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -static
TARGET = kubsh
SOURCE = main.cpp
PREFIX = /usr
BINDIR = $(PREFIX)/bin
DEB_NAME = kubsh
VERSION = 1.0.0
ARCH = amd64
DEB_FILE = $(DEB_NAME)_$(VERSION)_$(ARCH).deb

.PHONY: all build run install clean deb test

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
	@echo "Maintainer: Epoque <Tematik27@yandex.ru>" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "Description: KubSH" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	@echo "NeZnayuChtoEto: Smth for KubSH" >> $(DEB_NAME)_$(VERSION)_$(ARCH)/DEBIAN/control
	
	# Собираем пакет
	dpkg-deb --build $(DEB_NAME)_$(VERSION)_$(ARCH)

clean:
	rm -f $(TARGET)
	rm -rf $(DEB_NAME)_*

test: deb
	# Tесты
	@echo "Testing"
	docker run -v $(PWD)/$(DEB_FILE):/tmp/kubsh.deb tyvik/kubsh_test:master
test-src:
	docker run -v $(PWD):/workspace tyvik/kubsh_test:master
test-bin: build
	docker run -v $(PWD)/$(TARGET):/usr/bin/$(TARGET) tyvik/kubsh_test:master
