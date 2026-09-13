# Beginner usability studies

Noqeri's goal of being unusually easy to learn must be measured like a performance claim. Syntax opinions, contributor familiarity and a polished tutorial are not substitutes for observing new users complete the same tasks under a declared protocol.

The repository provides a small study recorder in `Tools/usability/study.mjs` and comparable task definitions in `Tools/usability/tasks.json`. These tools create measurement infrastructure; they do not contain participant results and they do not prove that Noqeri is easier than Python, Go, Lua or JavaScript.

## What to measure

Two primary measures correspond to the product goals:

**Time to first program** uses `first-program`: a participant starts from a clean directory and must create and run a program that prints exactly `Hello, world!`.

**Time to first real app** uses four separate tasks rather than one vague project score: a CLI app, HTTP API, JSON transformation tool and local database CRUD app. Each task has an observable acceptance condition in `tasks.json`.

The comparison languages are Noqeri, Python, Go, Lua and JavaScript. Keep language versions, operating system, editor, hardware class and resource policy in the study notes so results remain interpretable.

## Fair-study protocol

Install and verify toolchains before the timer starts. The timer begins immediately after the participant sees the task card and has a clean working directory; it stops only when the acceptance condition passes. Do not quietly include installation time for one language while excluding it for another. Installation/onboarding can be measured as a separate study.

Use the same editor and comparable machine for a cohort. Official documentation may be allowed consistently across languages. When a participant opens documentation, receives a hint, hits an error or restarts, record an event. If AI assistance is being studied, run it as a separately labelled cohort instead of mixing assisted and unassisted sessions.

Do not coach Noqeri participants more heavily because the language is less familiar. If a task exposes a confusing diagnostic or API, record the friction and fix the product rather than changing the protocol to hide it.

## Record a session

Use an opaque study code such as `P001`. Do not use a participant's name, email address or other identifying information.

```sh
node Tools/usability/study.mjs start \
  --participant=P001 \
  --language=noqeri \
  --task=first-program
```

The command prints a generated session UUID. During the task, record meaningful friction without copying source code or terminal history:

```sh
node Tools/usability/study.mjs event --session=<uuid> --kind=docs
node Tools/usability/study.mjs event --session=<uuid> --kind=compiler-error
node Tools/usability/study.mjs event --session=<uuid> --kind=hint --detail="Asked where the run command is documented"
```

Finish only after the observer checks the acceptance condition, or when the session is deliberately stopped:

```sh
node Tools/usability/study.mjs finish --session=<uuid> --success=true
node Tools/usability/study.mjs finish --session=<uuid> --success=false --reason="participant stopped"
```

By default events are appended to `build/usability/sessions.jsonl`, keeping raw observations out of source files and making the log append-only. A custom path can be supplied with `--log=...`.

## Produce evidence

```sh
node Tools/usability/study.mjs report build/usability/sessions.jsonl
node Tools/usability/study.mjs report build/usability/sessions.jsonl --json
```

The report includes started, completed, incomplete, successful and failed sessions for every task/language group. Successful completion times include median, p90, minimum and maximum values. Help and error event counts remain visible so a fast result achieved through heavy coaching is not presented as equivalent to an independent result.

## Interpreting results

Do not publish “Noqeri is #1 easiest” merely because its median is lower in a few sessions. At minimum, disclose sample counts, participant selection, prior programming experience criteria, language/tool versions, machine/setup, assistance policy, failures and the raw aggregate output. Compare like with like and predeclare how ties, timeouts and incomplete sessions are treated.

The most valuable outcome of early studies is usually diagnostic rather than promotional. Repeated documentation lookups point to discoverability problems; repeated compiler errors point to syntax or diagnostics; repeated hints point to missing affordances. Treat those observations as usability regressions to fix, then rerun the same task card so changes can be compared against the earlier protocol.

## Complexity budget

Usability evidence also feeds the language complexity budget. When a proposed language feature introduces another spelling or concept, test whether beginners actually complete a representative task more reliably or substantially faster with it. If equivalent power can live in a library without making the language surface harder to predict, prefer the library.
