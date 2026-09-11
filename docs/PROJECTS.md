# Noe Projects and Packages

Noe projects use Noe-native project metadata.

## Layout

```text
myapp/
├── project.noe
├── src/
│   └── main.noe
└── noe.lock
```

## `project.noe`

```noe
project {
    name: "myapp"
    version: "0.1.0"
    entry: "src/main.noe"
}
```

The manifest is parsed in a restricted deterministic mode. It is project metadata, not arbitrary code execution.

## Lockfile

Generate deterministic metadata:

```sh
./build/noe lock
```

Remote package downloads and a public package registry are not implemented yet.

## Why no TOML?

Noe projects intentionally use `project.noe` so the ecosystem remains consistent without mixing a second configuration language into normal Noe work.
