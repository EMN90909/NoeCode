# JetBrains / TextMate setup

JetBrains IDEs with TextMate bundle support can import the Ric TextMate grammar from `../vscode/syntaxes/ric.tmLanguage.json` and associate it with `*.ric`.

The repository intentionally keeps one grammar definition so editor highlighting does not drift. A dedicated JetBrains plugin should be added only when it implements richer behavior such as LSP lifecycle, navigation or diagnostics.
