# SHMServer SWIG Python 绑定

本目录使用 SWIG（已验证 4.3.0）为 SHMServer 构建 Python 扩展，提供与
`SHMServer/pybind11` 相同的模块和主要 Python API：

- `pack_message`：消息、行情、枚举和 `TDataMsg`
- `shm_server`：`SHMServer`
- `shm_connection`：`SHMConnection` 和 `SHMDataConnection`
- `spscqueue`：`SPSCQueue`

源码目录只保存 SWIG 接口和 C++ 适配层。`*_wrap.cxx`、director
`*_wrap.h` 以及代理 `*.py` 均由 CMake 在构建时生成到
`build/generated`，不会写回源码目录，也不需要提交到版本库。

## 依赖

- CMake 3.16 或更高版本
- C++17 编译器
- Python 3.9 或更高版本，包含开发头文件
- NumPy（运行时依赖，pip 安装时会自动解析）
- SWIG 4.0 或更高版本（已验证 4.3.0）
- `scikit-build-core`（构建 wheel 时由 pip 自动安装）

## SWIG 代码生成

```bash
cd SHMServer/python
./generate_bindings.sh
```

CMake 默认将中间代码生成到 `build/generated`。手动重新生成时可执行
`./generate_bindings.sh [输出目录]`，默认输出目录同样是
`build/generated`。

正常构建不需要提前执行该脚本；CMake 会跟踪 `.i`、`SHMSwig.hpp` 和
`SHMServer`/`Utils` 头文件变化并自动重新运行 SWIG。

`pack_message.i` 显式处理了 SWIG 不支持的匿名联合体，并用 typemap
实现定长 `char[N]` 与 Python `str`/`bytes` 的转换。

## 构建与测试

```bash
cd SHMServer/python
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

扩展模块输出到 `SHMServer/python/build`。SWIG 生成的 `*_wrap.cxx`、
`*_wrap.h` 和代理 `*.py` 位于 `SHMServer/python/build/generated`，
不会写入源码目录。

扩展文件名包含当前 Python ABI 后缀，例如：

```text
_pack_message.cpython-313-x86_64-linux-gnu.so
```

## pip 打包

wheel 使用 `scikit-build-core` 构建，会自动安装一个与当前操作系统和
CPython ABI 匹配的二进制 wheel。生成 wheel 后可以通过 pip 安装：

```bash
cd SHMServer/python
python3 -m pip wheel . --no-deps --wheel-dir dist
python3 -m pip install --force-reinstall dist/shmserver-0.1.0-*.whl
```

也可以使用 CMake 目标：

```bash
cmake --build build --target pip_wheel
```

构建源码包：

```bash
python3 -m build --sdist --outdir dist .
```

sdist 会通过 `sdist.force-include` 包含 `SHMServer` 和 `Utils` 中编译
所需的头文件，因此可以脱离完整仓库单独重建 wheel。构建主机仍需安装
SWIG 和 C++17 编译器。

wheel 安装后仍使用顶层模块导入：

```python
import pack_message
import shm_connection
import shm_server
import spscqueue
```

分发包名称为 `shmserver`，当前版本为 `0.1.0`。wheel 文件位于
`SHMServer/python/dist`，包含当前 Python ABI 的二进制扩展。wheel 不
包含 SWIG、CMake 或源码生成工具；切换 Python 小版本、ABI 或操作系统
后需要重新构建。

## 使用

```bash
export PYTHONPATH=/path/to/QuantFabric/SHMServer/python/build:/path/to/QuantFabric/SHMServer/python/build/generated:$PYTHONPATH
python3
```

```python
import pack_message
import shm_connection

client = shm_connection.SHMConnection("MyClient")
client.Start("MarketServer")

msg = pack_message.PackMessage()
msg.MessageType = pack_message.EMessageType.EFutureMarketData
msg.FutureMarketData.Ticker = "ag2501"
client.Push(msg)
```

与 pybind11 版本一致，类和枚举保留现有命名，方法名保持大写，例如
`Start`、`Push`、`Pop` 和 `IsConnected`。

## 目录与生成物

- `pack_message.i`、`shm_server.i`、`shm_connection.i`、`spscqueue.i`：
  SWIG 接口定义
- `SHMSwig.hpp`：模板具体化和 Python 生命周期适配层
- `build/generated/*_wrap.cxx`、`*_wrap.h`：构建时生成的 C++ 中间代码
- `build/generated/*.py`：构建时生成的 Python 代理模块
- `pyproject.toml`：`scikit-build-core` 和 wheel/sdist 配置
- `tests/test_bindings.py`：类型、跨模块、NumPy 和 director 集成测试

原有 `SHMServer/pybind11` 目录保留，便于接口对照和回退。
