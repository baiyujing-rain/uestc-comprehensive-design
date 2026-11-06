# 部署在线演示（Render 免费层）

把 MyPlacement 的 Flask 前端部署成公网可访问的 Demo，让别人点开就能上传 BookShelf 文件、看到布局结果。

## 一键部署

仓库根目录已有 `render.yaml`（Blueprint），直接点下面的按钮：

[![Deploy to Render](https://render.com/images/deploy-to-render-button.svg)](https://render.com/deploy?repo=https://github.com/baiyujing-rain/uestc-comprehensive-design)

Render 会：

1. 读取 `render.yaml`，识别 `myplacement-demo` 服务；
2. 用 `03-myplacement/web/Dockerfile` 构建镜像（编译 C++ 布局工具 + 安装 Flask / gunicorn）；
3. 绑定 `$PORT` 启动 gunicorn，健康检查走 `/health`。

首次构建约 5–10 分钟（要编译 C++）。建好后把 Render 给的域名填进 README 的「在线演示」一行。

## 本地验证镜像

```bash
cd 03-myplacement
docker build -f web/Dockerfile -t myplacement-web .
docker run --rm -p 5000:5000 myplacement-web
# 打开 http://localhost:5000
```

## 注意事项

| 项 | 说明 |
| --- | --- |
| 免费层限制 | 512 MB 内存、CPU 共享、无持久磁盘；15 分钟无请求会休眠，首次访问要等十几秒 |
| 超时 | 单次分析 `fast` 约 36 秒（adaptec1），`full` 约 56 秒；gunicorn 超时已设 600 秒 |
| 上传文件 | 请求结束即删除，不落盘保留；数据只存在于容器临时目录 |
| 规模 | 免费层跑 adaptec1 这类 21 万单元的数据会比较吃力，建议先用 `data/thin1` 演示 |
| 安全 | 页面是公开的，任何人都能上传文件；不要把它当成生产服务 |

## 不用 Render 的话

任何能跑 Docker 的地方都可以：

```bash
docker build -f web/Dockerfile -t myplacement-web 03-myplacement
docker run -d -p 8080:8080 -e PORT=8080 myplacement-web
```

或者直接在有图形环境的 Linux 上 `cd web && ./start.sh`。
