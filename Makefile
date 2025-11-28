# Компилятор и флаги
CXX := g++
CXXFLAGS := -O2 -std=c++17 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=35
LDFLAGS := -lfuse3 -pthread

# Имя программы
TARGET := kubsh

# Исходные файлы
SOURCES := main.cpp vfs.cpp

# Настройки пакета
PACKAGE_NAME := $(TARGET)
VERSION := 1.0
ARCH := amd64
DEB_FILENAME := kubsh.deb

# Временные директории
BUILD_DIR := deb_build
INSTALL_DIR := $(BUILD_DIR)/usr/local/bin

# Docker
DOCKER_IMAGE := kubsh-local
TEST_CONTAINER := kubsh-test-$(shell date +%s)

.PHONY: all clean deb run docker-build docker-test docker-shell quick-test local-test

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

deb: $(TARGET) | $(BUILD_DIR) $(INSTALL_DIR)
	# Копируем бинарник
	cp $(TARGET) $(INSTALL_DIR)/
	
	# Создаем базовую структуру пакета
	mkdir -p $(BUILD_DIR)/DEBIAN
	
	# Генерируем контрольный файл
	@echo "Package: $(PACKAGE_NAME)" > $(BUILD_DIR)/DEBIAN/control
	@echo "Version: $(VERSION)" >> $(BUILD_DIR)/DEBIAN/control
	@echo "Architecture: $(ARCH)" >> $(BUILD_DIR)/DEBIAN/control
	@echo "Maintainer: $(USER)" >> $(BUILD_DIR)/DEBIAN/control
	@echo "Description: Simple shell with VFS using FUSE3" >> $(BUILD_DIR)/DEBIAN/control
	@echo "Depends: fuse3" >> $(BUILD_DIR)/DEBIAN/control
	
	# Собираем пакет с фиксированным именем
	dpkg-deb --build $(BUILD_DIR) $(DEB_FILENAME)

$(BUILD_DIR) $(INSTALL_DIR):
	mkdir -p $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR) $(DEB_FILENAME)

run: $(TARGET)
	./$(TARGET)

# Docker цели
docker-build:
	docker build -t $(DOCKER_IMAGE) .

# Быстрый тест (собирает и сразу тестирует)
quick-test: docker-build
	docker run --rm \
		-v $(PWD):/mnt \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		$(DOCKER_IMAGE) \
		./check.sh

# Локальный тест (предполагает, что образ уже собран)
local-test:
	docker run --rm \
		-v $(PWD):/mnt \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		$(DOCKER_IMAGE) \
		./check.sh

# Тест с отладкой (останавливается на ошибках)
debug-test: docker-build
	docker run --rm -it \
		-v $(PWD):/mnt \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		$(DOCKER_IMAGE) \
		bash -c "cd /mnt && make && cd /opt && bash"

# Запуск контейнера для интерактивной отладки
docker-shell: docker-build
	docker run -it --rm \
		-v $(PWD):/mnt \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		$(DOCKER_IMAGE) \
		bash

# Тестирование без пересборки образа (самый быстрый)
test-fast:
	docker run --rm \
		-v $(PWD):/mnt \
		--device /dev/fuse \
		--cap-add SYS_ADMIN \
		$(DOCKER_IMAGE) \
		./check.sh

# Проверка зависимостей в контейнере
check-deps:
	docker run --rm \
		-v $(PWD):/mnt \
		$(DOCKER_IMAGE) \
		bash -c "echo '=== Checking dependencies ===' && \
		         g++ --version && echo 'G++: OK' && \
		         pkg-config --cflags fuse3 && echo 'FUSE3: OK' && \
		         make --version && echo 'Make: OK' && \
		         ls -la /dev/fuse && echo 'FUSE device: OK'"

# Очистка Docker образов
docker-clean:
	docker rmi $(DOCKER_IMAGE) 2>/dev/null || true

# Полная очистка
distclean: clean docker-clean

# Помощь
help:
	@echo "Доступные команды:"
	@echo "  make all          - собрать программу"
	@echo "  make run          - запустить шелл"
	@echo "  make deb          - создать deb-пакет"
	@echo "  make clean        - очистить сборку"
	@echo ""
	@echo "Docker команды:"
	@echo "  make docker-build - собрать Docker образ"
	@echo "  make quick-test   - собрать образ и запустить тесты"
	@echo "  make local-test   - запустить тесты (образ должен быть собран)"
	@echo "  make test-fast    - быстрый тест без пересборки"
	@echo "  make docker-shell - войти в контейнер для отладки"
	@echo "  make debug-test   - тест с интерактивной отладкой"
	@echo "  make check-deps   - проверить зависимости в контейнере"
	@echo "  make docker-clean - удалить Docker образ"
	@echo "  make distclean    - полная очистка"