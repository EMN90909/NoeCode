# Ric projects

Ric project metadata is source-like rather than TOML.

```ric
project {
    name: "demo"
    version: "0.1.0"
    entry: "src/main.ric"
    profile: "app"
    target: "native"
}
```

The file is named `project.ric`. `ric new demo` creates the manifest, `src/main.ric`, a tests directory and `ric.lock`.

`ric lock` writes deterministic dependency lines using the bootstrap lock format. The current dependency parser records name/version strings; registry resolution is not yet implemented.
