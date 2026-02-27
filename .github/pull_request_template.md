## Summary

- Describe the change and intended user impact.

## Widget API Checklist (Required For New/Changed Widgets)

Reference: `docs/widget-api-review-checklist.md`

- [ ] Default readability: widget/control is readable with default theme values (no app-side color setup required).
- [ ] Minimal constructor path: canonical 1-3 line usage with defaults is available and documented.
- [ ] Optional callbacks: baseline interaction works without callbacks when binding/state is provided.
- [ ] State/binding story: controlled, binding-backed, and owned-default/state-backed behavior is documented as applicable.
- [ ] Regression/docs gate: tests and API docs were updated in the same PR.

## Validation

- [ ] Ran `./scripts/compile.sh`
- [ ] Added/updated tests for changed behavior
