# Slang User Skills

This repository contains agent skills for writing Slang code and working with Slang tools.
Its contents are intended for Slang users; contributor workflows for developing the Slang compiler belong elsewhere.

The repository follows the [Agent Skills specification](https://agentskills.io/specification).
Skills live under `skills/` and can be discovered by tools that support that specification.

## Available skills

- `slang-port-hlsl` covers general language and compiler differences encountered while porting HLSL to native Slang.
- `slang-port-hlsl-templates` covers translation of HLSL templates into Slang generics.
- `slang-numeric-generics` covers Slang's experimental capability-oriented numeric interfaces.

The skills are under active development and have not yet reached a stable release.
In particular, the numeric interfaces currently require a Slang build that provides `slang.numerics` and enables experimental features.

## Installation

Installing Slang does not activate these skills or modify an agent's configuration.
Choose an installation method and scope explicitly.

### Follow the latest stable skills

With a version of GitHub CLI that provides the preview `gh skill` commands, install all skills for Codex at user scope with:

```sh
gh skill install shader-slang/slang-user-skills --all --agent codex --scope user
```

Use `--scope project` instead to install into the current project.
Install one skill by replacing `--all` with its name, for example `slang-port-hlsl-templates`.

An unpinned installation selects the latest tagged release of this repository, or `main` when no release exists.
No stable release exists yet, so the command above currently installs development content from `main`.

### Match an installed Slang release

Slang release packages record the tested source commit in `share/slang/agent-skills/PROVENANCE.json`.
Install that exact remote snapshot by passing its `sourceCommit` value:

```sh
gh skill install shader-slang/slang-user-skills --all --pin <sourceCommit> --agent codex --scope user
```

Pinned skills remain on that snapshot until explicitly unpinned.

To install the copy included in an extracted or system-installed Slang package without downloading this repository, point `gh skill` at the bundle root:

```sh
gh skill install /path/to/share/slang/agent-skills --from-local --all --agent codex --scope user
```

Replace `codex` with the selected Agent Skills-compatible client.
Users who do not use `gh skill` can copy individual directories under `skills/` to the location documented by their client.

### Remove installed skills

The preview `gh skill` interface does not currently provide an uninstall command.
Locate an installation with:

```sh
gh skill list --scope user --json skillName,path,sourceURL,pinned,version
```

Then use the selected client's removal mechanism or remove only the reported skill directory.
Repeat the listing with `--scope project` for project-scoped installations.

## Versioning policy

This repository versions and publishes stable skills independently of Slang compiler releases.
Its `main` branch may track current Slang development, while an unpinned installation follows the latest stable skills release after one exists.

Each Slang release separately pins a reviewed commit from this repository and carries that snapshot with provenance.
The commit SHA, rather than a mutable branch or optional tag, is the authoritative compatibility link.
Slang release automation must not create tags or releases in this repository.

## Validation

Validate the repository structure with:

```sh
python scripts/validate-skills.py
gh skill publish --dry-run
```

Validate the Slang examples against a selected compiler with:

```sh
python scripts/validate-examples.py --slangc /path/to/slangc
```

The example validator passes `-lang slang` and `-experimental-feature`.

See [CONTRIBUTING.md](CONTRIBUTING.md) for compatibility, example, and review requirements.
