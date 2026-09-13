# Beginner usability benchmark

Noqeri's ease-of-use claim must be measured like performance. This protocol is for observed sessions, not maintainer intuition.

## Participants

Recruit participants in clearly recorded experience bands:

- first-time programmer;
- beginner with less than one year of programming;
- experienced developer new to Noqeri.

Do not mix the bands when reporting medians. Record prior exposure to Python, JavaScript, Go and Lua without treating it as a failure condition.

## Environment

Use a clean machine/account or clean disposable workspace. The participant gets the public installation/getting-started material and may use normal search/documentation. Record the exact compiler version, OS, editor and documentation commit.

A facilitator may clarify the task but must not dictate source code. Every intervention is recorded.

## Measures

### Time to first program

Clock starts when the participant begins installation/setup. Success requires all of:

1. Noqeri is callable from the terminal;
2. participant creates a source file without a provided finished program;
3. compiler accepts it;
4. program produces intended output;
5. participant can explain at a basic level what they changed.

Record setup time, coding time, compiler-error count, documentation lookups, facilitator interventions and total elapsed time.

For comparison runs, use equivalent minimal tasks in Python, Go, Lua and JavaScript and the same participant band/environment rules.

### Time to first real app

Run four independent tasks:

1. CLI app: accept or represent an input and produce useful output;
2. HTTP API: one endpoint/request-response path;
3. JSON tool: load/validate/transform structured data;
4. database CRUD app: create/read/update/delete a small domain record.

Success requires a runnable app plus one meaningful error case. Record elapsed time, diagnostics encountered, external packages required and concepts the participant had to learn.

## Qualitative prompts

After each task ask, without leading:

- What did you expect to happen?
- Which message or concept slowed you down most?
- What did you have to learn that felt unrelated to the task?
- Where did you look for help?
- What would you try next without assistance?

Tag friction as installation, syntax, diagnostics, library discovery, package management, tooling, documentation, runtime/provider or conceptual complexity.

## Reporting

Publish participant count and experience bands; median and range; task completion rate; intervention count; error/lookup count; and top friction categories. Preserve anonymized raw event data when consent permits.

Do not publish “Noqeri is easier than X” from one maintainer walkthrough. Comparative claims require comparable participants, tasks and environments.

## Release feedback loop

Usability results may block an “easiest” marketing claim just as benchmark regressions may block a performance claim. Repeated beginner friction should first trigger a library/tooling/documentation fix; new syntax is the last resort and must pass `Doc/DESIGN_PRINCIPLES.md`.
