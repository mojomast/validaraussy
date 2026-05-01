# Validara

Validara is a terminal/TUI-style C application for checking whether a packaged service offer is ready to launch. It adapts analytical method validation concepts to small-business service readiness: intended use, specificity, accuracy, repeatability, range, robustness, system suitability, pilot deviations, and change control.

The current build is local-first and deterministic. It ships with a sample offer so the dashboard, scoring rules, and Markdown method-label export can be exercised without external services.

## Build and Install

Requirements:

- POSIX-like shell
- `make`
- `gcc` or a compatible C11 compiler

Build:

```sh
make clean
make
```

This creates the `validara` binary in the project directory.

## Usage

Show help:

```sh
./validara --help
```

Run the sample data demo with an ASCII readiness dashboard and method-label preview:

```sh
./validara --demo
```

Show only the dashboard:

```sh
./validara --dashboard
```

Export a Markdown method-label report:

```sh
./validara --export /tmp/validara-report.md
```

Running without arguments prints a compact TUI-style menu and dashboard:

```sh
./validara
```

## Testing

The project uses the C template's `greatest.h` test runner. Run:

```sh
make test
```

The test suite covers offer data, validation criteria mapping, readiness-state gating, pilot scoring, list capacity checks, and Markdown report export.

## Readiness Model

Validara computes:

- Protocol score from seven validation criteria: specificity, accuracy, precision/repeatability, range, robustness, system suitability, and change control.
- Pilot score from run-level duration variance, scope variance, missing inputs, deviation severity, and client-fit score.
- Overall score as a weighted protocol/pilot combination.
- State: Draft, Pilot Only, Restricted Launch, or Validated Launch.

Warnings are intentionally conservative. A service cannot be treated as launch-ready without named target clients, intended use, required inputs, exclusions, revision boundaries, acceptance criteria, and pilot evidence.
