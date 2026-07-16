# colon-subdirs

将 **colon-path** 中冒号后的子路径视为原子名称，再交给常规 Unix 命令执行。

## Colon-path

```
BASE:LEAF  →  物理路径 BASE/LEAF
              原子名   LEAF
```

仅当 `BASE` 含 `/`（或为 `.` / `..`）时识别 `:`，以免与 `scp`/`rsync` 的
`host:path` 冲突。详见 `colon-path(7)`。

## 命令

安装符号链接 `:mv` → `colon-mv`，以及 `:cp`、`:zip`、`:rm`、`:rmdir`、`:scp`、
`:unzip`、`:rsync`。

### 示例

```bash
:mv a/b:c/d ../x
# → mv a/b/c/d ../x/c/d

:zip a.zip a/b:c/d/e
# 归档内路径为 c/d/e/...

:unzip a.zip a/b:/c/d/e
# 从 zip 取出 a/b/c/d/e/**，写到 c/d/e/**
```

## 构建

```bash
meson setup /build
ninja -C /build
meson test -C /build
ninja -C /build install
```

## 许可证

AGPL-3.0-or-later.
