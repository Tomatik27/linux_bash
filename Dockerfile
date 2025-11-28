FROM tyvik/kubsh_test:master

# Устанавливаем все зависимости для C++ и FUSE
RUN apt update && \
    apt install -y \
    build-essential \
    g++ \
    make \
    pkg-config \
    fuse3 \
    libfuse3-dev \
    adduser \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Создаем точку монтирования и даем права
RUN mkdir -p /opt/users && chmod 0777 /opt/users

# Создаем необходимые симлинки
RUN ldconfig

WORKDIR /opt