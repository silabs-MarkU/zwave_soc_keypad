# Z-Wave Keypad POC Migration Guide

This guide converts the original door-lock keypad sample into a pure Z-Wave reporting keypad proof of concept based on the Entry Control Command Class.

The intended phase-1 behavior is:

- Pure reporting keypad.
- No local PIN validation.
- No local lock control.
- Key input is cached locally and reported to the controller through Entry Control Notifications.

## Recommended product model

Treat this device as an Entry Control keypad, not as a lock.

- Device type: `GENERIC_TYPE_ENTRY_CONTROL` + `SPECIFIC_TYPE_SECURE_KEYPAD`
- Security request: keep `S2 Access Control`
- Primary application behavior: `COMMAND_CLASS_ENTRY_CONTROL`
- Keep `Indicator CC` for local feedback
- Do not support `Basic CC`
- Do not reintroduce `Door Lock`, `User Code`, or `User Credential` unless the product is later intended to grant local access

Why the lock-oriented command classes were removed:

- `Door Lock CC`
  - The phase-1 product is not a lock actuator.
  - There is no local bolt, latch, or door-handle state that should be exposed as the primary product behavior.
- `User Code CC`
  - Phase 1 is pure reporting only.
  - The keypad is not validating PINs locally and is not making access decisions in the node.
- `User Credential CC`
  - Phase 1 does not implement credential storage, credential learn flows, or credential database management in the node.
  - Reintroduce this only if the keypad later becomes a credential-managing access product.
- `Basic CC`
  - The Z-Wave device-type requirements for an Entry Control Keypad say `Basic CC` must not be supported.
  - Even if `Basic CC` is omitted from the advertised NIF, keeping the handler linked still means the node can answer `Basic` traffic, which is not standards-aligned for this product type.
  - Semantically, `Basic Set/Get` is a poor fit for a pure reporting keypad because there is no single actuator state to map.

Why `Basic CC` existed in the original door-lock sample:

- The original `zwave_soc_door_lock_keypad` example was a lock product.
- In that sample, `Basic` was intentionally kept for compatibility and mapped to Door Lock Operation.
- That behavior made sense for a lock, but it does not make sense for a pure reporting keypad.

## Current project status

The current software direction now matches the keypad POC more closely:

- The generated device type and icon configuration make sense:
  - `GENERIC_TYPE_ENTRY_CONTROL`
  - `SPECIFIC_TYPE_SECURE_KEYPAD`
  - `ICON_TYPE_SPECIFIC_ENTRY_CONTROL_KEYPAD_0_9`
- `Door Lock`, `User Credential`, and `User Code` have been removed from the Studio configuration.
- `Basic CC` should be removed as part of the keypad conversion and must not be reintroduced by later Studio regeneration.
- The firmware now has a dedicated `app_keypad` module that implements phase-1 Entry Control behavior and CLI simulation.
- The project can be built before final pin routing is assigned:
  - `keyscan_driver` may be added first
  - KEYSCAN initialization and scan start can remain disabled until the pin tool work is complete

Items that still require user action in Studio or hardware:

- Add and configure the `keyscan_driver` component.
- Remove `zw_cc_basic` if it is still present or if Studio regeneration reintroduces it.
- Assign the final keypad pins in the pin tool.
- Regenerate the Studio project after the `.slcp` metadata cleanup.
- Replace the temporary keypad simulation path with the real KEYSCAN callback hookup.

## Step-by-step migration summary

Use this section as the customer-facing overview of what was done to convert the original sample.

1. Start from the Silicon Labs `zwave_soc_door_lock_keypad` sample.
2. Reclassify the node as a keypad product instead of a lock:
   - `GENERIC_TYPE_ENTRY_CONTROL`
   - `SPECIFIC_TYPE_SECURE_KEYPAD`
   - `ICON_TYPE_SPECIFIC_ENTRY_CONTROL_KEYPAD_0_9`
3. Keep `S2 Access Control` because the product is still an access-related device.
4. Remove lock-oriented command classes and related configuration:
   - `Door Lock CC`
   - `User Code CC`
   - `User Credential CC`
   - any door-lock-specific NVM or credential settings
5. Remove `Basic CC`:
   - the original sample used it because it was still a lock
   - the keypad POC is not a lock and should not expose `Basic`
6. Keep the WSTK buttons only for service actions:
   - learn mode
   - factory reset
   - battery report or simple lab action
7. Replace keypad/lock sample behavior with a dedicated `app_keypad` path:
   - local key caching
   - Entry Control reports and notifications
   - no local credential validation
8. Add CLI simulation so Entry Control behavior can be tested before the physical keypad is wired.
9. Add `keyscan_driver`, but do not rush the pin-tool work:
   - first confirm the project still builds, flashes, and debugs
   - then assign the final matrix pins
10. After pin routing is ready, replace the temporary simulated path with the real KEYSCAN callback and matrix decode table.
11. Validate the node with a controller:
   - command-class interview
   - `Key Supported`
   - `Event Supported`
   - `Configuration Get/Set`
   - Entry Control notifications once real input is enabled

## What the Z-Wave keypad should do on key presses

For a standards-aligned keypad POC, user input should be cached locally and then reported with Entry Control Notifications.

From the Z-Wave Application Specification:

- Input must be cached before the full entry is sent.
- Entry termination may happen because:
  - cache size is exceeded
  - cache timeout is exceeded
  - a command button is pressed
  - other data such as RFID is received
- `Key Cache Size = 1` is valid and results in per-key reporting.
- The default `Key Cache Size` should be `4`.
- The default `Key Cache Timeout` should be `2` seconds.
- In deployments where a user enters characters first and then presses a command key such as Enter, it is recommended to set cache size to `32` and timeout to `2` seconds.

### Phase-1 behavior selected for this keypad

The attached membrane keypad has dedicated `Ent` and `Esc` keys, so the logical behavior should follow the physical legend instead of overloading `#` or `*`.

Phase-1 mapping:

- Buffered ASCII keys:
  - `0-9`
  - `*`
  - `#`
- Command keys:
  - `Ent` -> `ENTER`
  - `Esc` -> `CANCEL`
- Reserved for later:
  - `F1`
  - `F2`
  - `Up`
  - `Down`
  - `Left`
  - `Right`

Phase-1 supported Entry Control events:

- `CACHING`
- `CACHED_KEYS`
- `ENTER`
- `CANCEL`

Implementation note:

- For timeout-terminated or cache-size-terminated entries, send `CACHED_KEYS`.
- For `Ent`, send `ENTER`.
- For `Esc`, send `CANCEL`.
- This guide intentionally keeps `ENTER` and `CANCEL` as distinct command-button events because the keypad has dedicated command keys.

## Keypad image alignment

The guide is based on the attached 20-key membrane keypad with:

- Digits `0-9`
- `*`
- `#`
- `F1`
- `F2`
- `Esc`
- `Ent`
- arrow keys

Use a single logical-key mapping table in firmware so the low-level matrix decode is isolated from the Z-Wave reporting behavior.

Recommended logical interpretation for phase 1:

```text
Reporting keys: 0 1 2 3 4 5 6 7 8 9 * #
Command keys:   Ent, Esc
Reserved keys:  F1, F2, Up, Down, Left, Right
```

## Hardware direction

### Matrix size

A `4x5` keypad needs `9` signals total.

- 4 lines for one matrix dimension
- 5 lines for the other matrix dimension

### Why the keypad should be wired as 4 rows x 5 columns in KEYSCAN

On EFR32ZG23:

- `KEYSCAN.ROW_SENSE_x` only routes on ports `A` and `B`
- `KEYSCAN.COL_OUT_x` routes on ports `A`, `B`, `C`, and `D`

Because of that routing restriction, the smaller matrix dimension should be assigned to `ROW_SENSE`.

For this keypad project:

- KEYSCAN rows: `4`
- KEYSCAN columns: `5`

That means the electrical KEYSCAN orientation may be transposed relative to the printed keypad legend. That is okay. The firmware should translate raw matrix positions into logical keys.

### Recommended first-pass BRD4210A expansion-header wiring

Recommended signals for the first hardware spin:

- KEYSCAN rows:
  - `PA10` on EXP pin `3`
  - `PA0` on EXP pin `5`
  - `PA5` on EXP pin `7`
  - `PA6` on EXP pin `11`
- KEYSCAN columns:
  - `PD2` on EXP pin `9`
  - `PC0` on EXP pin `10`
  - `PA7` on EXP pin `13`
  - `PC5` on EXP pin `15`
  - `PC7` on EXP pin `16`

Why this layout is a good first choice:

- It keeps VCOM TX/RX on EXP pins `12` and `14`.
- It consumes VCOM CTS/RTS on EXP pins `3` and `5`, which is acceptable because this project currently has flow control disabled.
- It uses the onboard sensor I2C pins `15` and `16`, so the Si7021 should be treated as sacrificed for keypad use.
- It avoids EXP pins `4`, `6`, and `8`, which are tied into other board functions.

### KEYSCAN configurator settings

Configure the Silicon Labs `keyscan_driver` component with:

- `SL_KEYSCAN_DRIVER_COLUMN_NUMBER = 5`
- `SL_KEYSCAN_DRIVER_ROW_NUMBER = 4`
- `SL_KEYSCAN_DRIVER_SINGLEPRESS = 1`
- `SL_KEYSCAN_DRIVER_DEBOUNCE_DELAY_MS = 2` or `4`
- `SL_KEYSCAN_DRIVER_STABLE_DELAY_MS = 2`

Important:

- Do the pin assignment in the Simplicity Configurator / pin tool.
- Do not hand-edit the `.pintool` XML.
- It is reasonable to add the driver and verify build/flash/debug behavior before assigning the final pins.
- During that bring-up stage, keep KEYSCAN inactive so the new driver does not start consuming real keypad input before routing is finalized.

## Product identifiers

The current screenshot shows:

- Manufacturer ID = `0x0000`
- Product Type ID = `0x0004`
- Product ID = `0x0001`

That is acceptable as temporary lab configuration, but it is not what should ship to a customer.

Recommended customer-facing scheme:

- Manufacturer ID:
  - Use the assigned Z-Wave manufacturer ID of the actual company.
- Product Type ID:
  - Use this as the product family identifier.
  - Suggested value for this keypad family: `0x0001`
- Product ID:
  - Use this as the specific SKU or hardware revision identifier.
  - Suggested value for first keypad POC / rev A: `0x0001`

Practical recommendation:

- Keep `0x0004` only as a temporary placeholder if you are still iterating locally.
- For the customer package, switch to a documented family/revision scheme and replace Manufacturer ID `0x0000`.

## Firmware architecture

### Board buttons

Keep the WSTK on-board buttons only for service functions:

- `BTN1` short press: learn mode toggle
- `BTN1` very long press: factory reset
- `BTN0` medium press: battery report or simple lab action

Do not use the board buttons as a substitute for the real keypad in the delivered POC.

Reason:

- The board buttons are still useful for inclusion, exclusion, reset, and bench bring-up.
- They should remain simple service controls while the external keypad becomes the real Entry Control input device.

### Keypad input path

The KEYSCAN callback runs in interrupt context, so it must not perform Z-Wave transmissions directly.

Recommended event flow:

1. KEYSCAN callback converts raw matrix position to a logical key.
2. The callback queues a lightweight app event using `zaf_event_distributor_enqueue_app_event_from_isr()`.
3. The application task updates the keypad state machine:
   - append ASCII keys
   - start or restart the timeout timer
   - send `ENTER`
   - send `CANCEL`
4. The application task sends Entry Control Notifications through Lifeline.
5. Local indicator or buzzer feedback is applied for accepted input and transmit failures.

## Implemented phase-1 software shape

The recommended next coding step has now been started in the project:

- Added an `app_keypad` module.
- Implemented an Entry Control supporting command-class handler for:
  - `ENTRY_CONTROL_KEY_SUPPORTED_GET`
  - `ENTRY_CONTROL_EVENT_SUPPORTED_GET`
  - `ENTRY_CONTROL_CONFIGURATION_GET`
  - `ENTRY_CONTROL_CONFIGURATION_SET`
- Implemented Entry Control Lifeline notifications for:
  - `CACHING`
  - `CACHED_KEYS`
  - `ENTER`
  - `CANCEL`
- Added CLI simulation commands so keypad behavior can be exercised before KEYSCAN is wired.
- Added a safe KEYSCAN bring-up path so the project can build before final pin-tool routing is completed.
- Removed project metadata references that still positioned the app as a door-lock product.

Repository-level cleanup performed or required as part of this migration:

- Remove `zw_cc_basic`.
- Remove `app_cc_door_lock_callbacks.c` from the build.
- Remove `app_credentials.h` from the active app include set.
- Remove `DoorLockKeyPad.cc_config` from the active customer build configuration.
- Keep `Indicator CC`, `Battery CC`, Association, Supervision, Version, Manufacturer Specific, and Z-Wave Plus Info as supporting command classes for a normal keypad node.

### Current phase-1 defaults

The implemented defaults are:

- `Key Cache Size = 4`
- `Key Cache Timeout = 2 seconds`

This matches the spec-recommended defaults.

If the desired controller workflow is “enter digits, then press `Ent`”, use:

- `Key Cache Size = 32`
- `Key Cache Timeout = 2 seconds`

That configuration avoids the entry being flushed before `Ent` is pressed.

## Studio cleanup checklist

Keep these steps in the customer package even if some were already completed in the working tree:

1. Set device type to:
   - `GENERIC_TYPE_ENTRY_CONTROL`
   - `SPECIFIC_TYPE_SECURE_KEYPAD`
2. Set user and installer icon type to:
   - `ICON_TYPE_SPECIFIC_ENTRY_CONTROL_KEYPAD_0_9`
3. Keep `S2 Access Control` requested.
4. Remove:
   - `Door Lock CC`
   - `User Code CC`
   - `User Credential CC`
   - any door-lock-specific NVM selections
5. Remove `zw_cc_basic`.
6. Keep `Indicator CC`.
7. Add `keyscan_driver`.
8. Confirm the project still builds, flashes, and debugs before final keypad routing is assigned.
9. Assign the KEYSCAN pins in the pin tool.
10. Regenerate the project.
11. Verify that Studio regeneration did not reintroduce:
   - `zw_cc_basic`
   - door-lock-specific sources
   - door-lock-specific config files

## Controller validation notes

Before real keypad wiring is enabled, the most useful validation is with controller `GET`/`REPORT` traffic.

Recommended checks:

- `Entry Control Key Supported Get`
  - expect a `Key Supported Report`
  - phase-1 expected supported keys are `#`, `*`, and `0-9`
- `Entry Control Event Supported Get`
  - expect `CACHING`, `CACHED_KEYS`, `ENTER`, and `CANCEL`
- `Entry Control Configuration Get`
  - expect the default cache size and timeout
- `Entry Control Configuration Set`
  - verify the node accepts valid values
  - read them back with `Configuration Get`

How to interpret the `Key Supported` bitmask:

- The bit number is the ASCII code of the supported key.
- For this keypad POC, the expected set bits are:
  - bit `35` for `#`
  - bit `42` for `*`
  - bits `48` through `57` for `0` through `9`
- Example expected result:
  - `bit 35, bit 42, bit 48 ... bit 57`

What not to expect yet:

- Do not expect real `Entry Control Notification` traffic from physical key presses until:
  - the pin-tool routing is assigned
  - KEYSCAN is enabled
  - the matrix-position-to-logical-key mapping is connected

## Remaining integration work

The main remaining work items after this software pass are:

1. Add the real KEYSCAN callback implementation.
2. Create the physical matrix-position-to-logical-key table for the actual keypad tail pinout.
3. Verify the solder/harness wiring on the BRD4210A headers.
4. Add local indicator or buzzer feedback for:
   - key accepted
   - cancel
   - transmit failure
5. Run inclusion/interview testing with a controller and confirm:
   - Entry Control interview
   - Key Supported report
   - Event Supported report
   - Configuration Set/Get behavior
   - Lifeline delivery of notifications

## Customer handoff note

The keypad photo from this discussion is useful for customer context, but it is not embedded directly in this repository document. For the delivered package, either:

- include the keypad photo alongside this guide and reference it externally, or
- replace the text-only mapping section with an inserted project image in the customer document set.
