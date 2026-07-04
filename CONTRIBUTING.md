# Contributing to Open Observer

Open Observer is an OpenCPN plugin for structured marine observations. The project is developed collaboratively around a stable `main` branch and reviewable pull requests.

## Working model

`main` is the stable integration branch.

Do not push directly to `main`. Create a dedicated branch for each focused task, then open a pull request into `main`.

Suggested branch names:

```text
feature/nmea-cleanup
feature/project-import-xlsx
fix/windows-import
docs/user-guide
```

Keep each branch and pull request limited to one coherent subject. Avoid mixing functional changes, broad refactoring and documentation cleanup in the same pull request unless they genuinely depend on each other.

## Starting work

Update your local `main` branch before creating a new branch:

```bash
git switch main
git pull --ff-only origin main
git switch -c feature/short-description
```

Use a meaningful branch name and a concise commit message which describes the change rather than the activity.

## Before changing code

Read `ARCHITECTURE.md` and inspect the current implementation before editing.

In particular:

- `src/openobserver_pi.*` is the OpenCPN plugin entry point and lifecycle coordinator.
- `src/ooControlDialogDef.*` and `src/ooControlDialogImpl.*` form the main standard interface.
- `src/ooObservations.*` contains the project and observation data model.
- `data/` contains packaged resources copied into installed plugin packages.
- `.github/workflows/` builds the public macOS and Windows import packages.

### Important UI constraint

`ooControlDialogDef.*` originates from wxFormBuilder, but the current generated source is maintained directly in the repository and must not be regenerated casually.

The historical form source must not be treated as authoritative without a coordinated migration plan. Regenerating the dialog may overwrite intentional code-level refinements.

### Experimental AUI panel

The optional AUI mini panel is not part of the stable release path. It remains experimental because Windows crashes were observed after destruction of the AUI panel.

Do not make it the default interface, expand its scope, or change its lifecycle without a separately reviewed cross-platform test plan.

## Build and test expectations

Every pull request into `main` triggers two GitHub Actions workflows:

- macOS Apple Silicon
- Windows 32-bit

Both builds must pass before merging.

Run focused manual tests for the feature you changed. Examples:

- Project-editing changes: create, edit, load and save a project.
- Observation changes: start, save, delete, filter and export observations.
- NMEA changes: test recording, linked recording paths and restart behaviour.
- Data Package changes: create or update a package and inspect the generated files.
- UI changes: open, close, minimize, restore and restart OpenCPN.
- Packaging changes: import the generated `-IMPORT.tar.gz` archive into OpenCPN.

Do not commit generated build output, local OpenCPN data, temporary archives or personal test files.

## Pull requests

A pull request should explain:

1. The user or technical problem being solved.
2. The files and behaviour changed.
3. Known limitations, risks or follow-up work.
4. The tests performed on macOS and/or Windows.

Keep the pull request description understandable to collaborators who did not write the change.

## Review and merge rule

A pull request into `main` requires:

- successful macOS Apple Silicon and Windows 32-bit CI builds;
- at least one approval from another contributor;
- resolution of review comments;
- no unresolved scope or compatibility concern.

Any administrator may merge after these conditions are met. For changes affecting data compatibility, release packaging, licenses, project structure, or the public interface, agree on the intended direction before merging.

## Releases

Releases are created from `main` after cross-platform import-package validation.

A release tag identifies a stable published state. Do not change version metadata, release workflows or public package content incidentally as part of unrelated work.

## Security and project hygiene

Do not commit credentials, API keys, personal paths, private observation data or large raw field datasets.

Raise concerns early when a change could affect user data, package compatibility, cross-platform behaviour or the integrity of observation records.
