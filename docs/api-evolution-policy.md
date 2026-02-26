# PrimeStage API Evolution And Compatibility Policy

## Scope

This policy defines how PrimeStage evolves public widget specs and callback contracts while preserving upgrade safety for application code.

Public API surfaces covered by this policy:
- headers under `include/PrimeStage/`
- widget spec fields and callback payload contracts
- behavior contracts documented in `docs/api-ergonomics-guidelines.md` and `docs/prime-stage-design.md`

## Versioning Expectations

PrimeStage follows semantic-versioning intent for public API changes.

- Patch release:
  - bug fixes and internal refactors only
  - no intentional source-breaking API changes
- Minor release:
  - additive API changes (new fields, new optional callbacks, new helpers)
  - deprecations may be introduced
  - existing behavior can only tighten when documented and migration-safe
- Major release:
  - removal of deprecated API
  - source-breaking signature or behavior changes that cannot be backward compatible

Pre-1.0 note:
- PrimeStage is currently pre-1.0.
- Breaking changes still require explicit migration notes and deprecation staging; do not ship silent breakage in minor updates.

## Deprecation Process

When replacing or reshaping a public API:

1. Introduce replacement API first.
2. Keep compatibility path for at least one release window (or one milestone cycle for pre-1.0).
3. Mark old API as deprecated in docs/changelog and point to exact replacement.
4. Add/expand tests to cover replacement semantics.
5. Add migration notes with before/after snippets.
6. Remove deprecated API only in a major version (or explicitly documented pre-1.0 break window).

Deprecation requirements for PRs:
- include migration notes in this document
- include regression checks in `tests/unit/test_api_ergonomics.cpp`
- update `docs/todo.md` if follow-up cleanup/removal work remains

## Migration Notes

### Draft Name Migration: `EditBox` -> `TextField`

Status: completed migration in docs and public API examples.

- old draft naming: `EditBox`, `createEditBox(...)`
- shipped naming: `TextField`, `createTextField(TextFieldSpec const&)`

Migration guidance:
- replace `EditBoxSpec` usage with `TextFieldSpec`
- replace builder calls `createEditBox(...)` with `createTextField(...)`
- migrate callbacks to `TextFieldSpec::callbacks` and state to `TextFieldState`

Before:

```cpp
EditBoxSpec field;
field.text = "name";
root.createEditBox(field);
```

After:

```cpp
PrimeStage::TextFieldSpec field;
field.state = &appState.nameField;
root.createTextField(field);
```

## Compatibility Review Checklist

Before merging public API changes:

1. Classify change as patch/minor/major impact.
2. Confirm whether a compatibility shim or staged deprecation is required.
3. Update migration notes in this document.
4. Add/adjust regression guards in `tests/unit/test_api_ergonomics.cpp`.
5. Update docs that describe affected API contracts.
