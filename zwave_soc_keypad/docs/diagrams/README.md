# Diagram Sources

The README uses static SVG diagrams because SVG renders more predictably than raw Mermaid in many Markdown viewers.

The editable source of truth for those diagrams lives here:

- `keypad-physical-to-logical-flow.mmd`
- `keypad-notification-flow.mmd`
- `keypad-em2-wake-flow.mmd`

Generated README assets live here:

- `../images/keypad-physical-to-logical-flow.svg`
- `../images/keypad-notification-flow.svg`
- `../images/keypad-em2-wake-flow.svg`

## Editing Workflow

1. Edit the `.mmd` file first.
2. Regenerate the matching SVG.
3. Verify the SVG in the README preview.
4. Keep node labels short and prefer explicit line breaks like `<br/>` inside Mermaid nodes when a label gets long.

## Example Mermaid CLI Commands

If `mmdc` is available:

```bash
mmdc -i zwave_soc_keypad/docs/diagrams/keypad-physical-to-logical-flow.mmd -o zwave_soc_keypad/docs/images/keypad-physical-to-logical-flow.svg
mmdc -i zwave_soc_keypad/docs/diagrams/keypad-notification-flow.mmd -o zwave_soc_keypad/docs/images/keypad-notification-flow.svg
mmdc -i zwave_soc_keypad/docs/diagrams/keypad-em2-wake-flow.mmd -o zwave_soc_keypad/docs/images/keypad-em2-wake-flow.svg
```

If a future agent is doing the refresh, the intended workflow is the same: update the `.mmd` source, regenerate the SVG, then confirm the README preview still looks clean.
