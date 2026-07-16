# colon-subdirs

Tools that treat a **colon-path** subpath as an atomic name, then delegate to
the normal Unix command.

## Colon-path

```
BASE:LEAF  →  physical path BASE/LEAF
              atomic name   LEAF
```

The `:` separator is used only when `BASE` contains `/` (or is `.` / `..`), so
`host:path` for `scp`/`rsync` is left alone. See `colon-path(7)`.

## Commands

| Symlink | Binary       | Wraps   |
|---------|--------------|---------|
| `:mv`   | `colon-mv`   | `mv`    |
| `:cp`   | `colon-cp`   | `cp`    |
| `:zip`  | `colon-zip`  | `zip`   |
| `:rm`   | `colon-rm`   | `rm`    |
| `:rmdir`| `colon-rmdir`| `rmdir` |
| `:scp`  | `colon-scp`  | `scp`   |
| `:unzip`| `colon-unzip`| `unzip` |
| `:rsync`| `colon-rsync`| `rsync` |

### Examples

```bash
:mv a/b:c/d ../x
# → mv a/b/c/d ../x/c/d   (mkdir -p ../x/c)

:zip a.zip a/b:c/d/e
# archive members stored as c/d/e/...

:unzip a.zip a/b:/c/d/e
# extract a/b/c/d/e/** from the zip, write to c/d/e/**
```

Each wrapper parses options similarly to the underlying tool, creates needed
parent directories, rewrites file arguments, and `exec`s the real command.
Use `--colon-dry-run` to print the rewritten command. Full option details live
in the original manuals (`mv(1)`, `zip(1)`, …); the colon-* man pages document
colon-path behavior and wrapper flags.

### `COLON_WRAP`

| Value | Behavior |
|-------|----------|
| unset | `exec` the stem of `$0` from `PATH` (`colon-mv` → `mv`) |
| `0` | run the built-in rewrite in `src/rewrite/<cmd>.c` |
| `<name>` | `exec <name>` instead |

A leading colon-path (`:foo/bar`) means BASE `/` (same as `/:foo/bar`).
For `:mv` / `:cp`, destination `:path` is rewritten by substituting the first
`:` with `/`.

## Bash completion

Installs under `bash-completion/completions/` for both `colon-mv` and `:mv`
(and the other commands). Completions keep `:` inside the current word and
complete `BASE:LEAF` by listing entries under `BASE/`:

```bash
:mv a/b:c/<TAB>    # completes names under a/b/c/ as a/b:c/…
```

## Build

```bash
meson setup /build
ninja -C /build
meson test -C /build
```

Install (includes `:mv` → `colon-mv` symlinks):

```bash
ninja -C /build install
```

## License

AGPL-3.0-or-later.
