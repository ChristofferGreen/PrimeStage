# PrimeStage Default Widget Behavior Matrix

This matrix defines baseline defaults for interactive widgets in canonical PrimeStage usage.
`enabled=true` and default specs are assumed unless noted.

| Widget | Focusable Default | Keyboard Default | Pointer Default | Accessibility Role Default |
| --- | --- | --- | --- | --- |
| `Button` | Yes | `Enter`/`Space` activate | Press + release inside activates | `Button` |
| `TextField` | Yes | Editing keys + `Enter` submit | Click places caret, drag selects | `TextField` |
| `SelectableText` | No | Selection keys when externally focused | Drag selection and clipboard shortcuts | `StaticText` |
| `Toggle` | Yes | `Enter`/`Space` toggle | Click toggles | `Toggle` |
| `Checkbox` | Yes | `Enter`/`Space` toggle | Click toggles | `Checkbox` |
| `Slider` | Yes | Arrow/Home/End adjust value | Click/drag adjusts value | `Slider` |
| `ProgressBar` | Yes | Arrow/Home/End adjust value | Click/drag adjusts value | `ProgressBar` |
| `Tabs` | Yes (per tab) | `Enter`/`Space` activate tab, arrows/Home/End navigate | Click tab selects | `TabList` |
| `Dropdown` | Yes | `Enter`/`Space`/Down advance, Up reverse | Click advances selection | `ComboBox` |
| `Table` | Yes | Up/Down/Home/End move selected row | Row click selects | `Table` |
| `List` | Yes | Up/Down/Home/End move selected row | Row click selects | `Table` (list-backed) |
| `TreeView` | Yes | Up/Down/Home/End/Page keys navigate; Left/Right collapse/expand | Row click selects | `Tree` |
| `Window` | Yes (`focusable=true`) | Focus participation via tab order | Pointer focus/move/resize via chrome callbacks | `Group` |

Implementation note:
- `ProgressBar` keeps a fill node allocated for patch-first interaction updates; at `value=0` with
  `minFillWidth=0` the fill remains hidden (`visible=false`) until value becomes non-zero.

## Regression Gates

- Runtime behavior gates: `tests/unit/test_interaction.cpp`.
- Docs/API/source contract gates: `tests/unit/test_api_ergonomics.cpp`.
- If defaults change intentionally, update this matrix and both test gates in the same change.
