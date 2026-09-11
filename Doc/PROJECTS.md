# noqeri projects

Project metadata is source-like rather than TOML.

```nqr
project {
    name: "demo"
    version: "0.1.0"
    entry: "src/main.nqr"
    profile: "app"
    target: "native"
}
```

The file is `project.nqr`. `noqeri new demo` creates the manifest, `src/main.nqr`, a tests directory and `noqeri.lock`. `noqeri lock` writes deterministic dependency lines using the bootstrap lock format.
