# MyPlacement Web 演示镜像
# 构建上下文：仓库根下的 03-myplacement/
# 1) 编译 C++ 布局工具  2) 用 gunicorn 跑 Flask 前端
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        libx11-dev \
        python3 \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

RUN python3 -m venv /opt/venv \
    && /opt/venv/bin/pip install --upgrade pip \
    && /opt/venv/bin/pip install -r web/requirements.txt

ENV PATH="/opt/venv/bin:$PATH" \
    MYPLACE_ROOT=/app \
    MYPLACE_BIN=/app/build/myplace \
    MYPLACE_HOST=0.0.0.0

WORKDIR /app/web
EXPOSE 5000

# Render 通过 $PORT 注入端口；本地直接 docker run 时回落到 5000
CMD ["sh", "-c", "exec gunicorn --bind 0.0.0.0:${PORT:-5000} --workers 1 --threads 4 --timeout 600 --access-logfile - server:app"]
