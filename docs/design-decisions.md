# PrimeStage Design Decisions

This document records concrete architecture decisions that were previously tracked as open questions.

## Decision 1: Table Remains a First-Class Widget
- Decision: Keep `Table` as a first-class widget in PrimeStage instead of treating it as only a specialized list variant.
- Rationale:
  - Table semantics (columns, headers, row/cell styles, and row-selection behavior) are explicit in the public API and tests.
  - A first-class API keeps app code straightforward and avoids overloading list abstractions with table-specific behavior.
  - Shared collection internals can still be reused under the hood without changing user-facing contracts.
- Consequence:
  - `TableSpec` remains part of the core widget set.
  - Future list abstractions should be additive and must not degrade existing table ergonomics.

## Decision 2: Window Chrome Is Composed Explicitly
- Decision: Keep window chrome ownership in composed widget code (app/runtime composition) rather than auto-generating chrome implicitly.
- Rationale:
  - Explicit composition aligns with PrimeStage’s current declarative widget builder model.
  - App/runtime authors retain control over chrome visuals and interaction behavior without hidden framework state.
  - It keeps the core runtime deterministic by avoiding undocumented auto-generated node trees.
- Consequence:
  - Window helpers may be provided as reusable builders, but they emit ordinary widget composition and remain opt-in.
  - No hidden automatic chrome generation is introduced in core PrimeStage runtime paths.

## Decision 3: Patch Operations Use a Strict Safety Whitelist
- Decision: Enforce a strict whitelist for patch-first updates; non-whitelisted changes must request layout or rebuild.
- Whitelisted patch fields:
  - local position/size
  - visibility/opacity
  - style tokens and style overrides
  - scroll offsets
  - focus/hover/pressed visual toggles that do not change graph structure
- Non-whitelisted changes (must rebuild or relayout):
  - node creation/removal/reparenting
  - layout type/padding/gap changes that affect layout topology
  - data-model structural mutations that change collection shape
- Consequence:
  - App/runtime scheduling must use explicit frame lifecycle requests (`requestLayout`, `requestRebuild`) when changes exceed patch safety.
  - This keeps interaction patches deterministic and prevents accidental structural mutation in hot paths.
