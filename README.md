# Z-Wave Keypad Proof of Concept:

Who: Mark Umina<br>
When: Friday, March 20, 2026<br>
What: Simplicity SDK Suite v2025.12.1, Z-Wave SDK 8.0.0.0<br>
Why: Create a FLiRS keypad proof of concept from DoorLockKeypad application

This repository captures the conversion of the Silicon Labs `ZWave_SoC_DoorLockKeypad_Solution` sample into a Z-Wave keypad proof of concept built around `COMMAND_CLASS_ENTRY_CONTROL`.

## Overview

The project is derived from the Silicon Labs door-lock keypad sample, but it is now modeled as a secure keypad rather than a lock. Lock, and credential-specific behavior was removed because it is not part of the keypad device model used here.

Current implementation:

- Node type changed to Entry Control / Secure Keypad
- Entry Control command handling is implemented
- keypad input is cached locally and reported through Entry Control notifications
- Keypad configuration is stored in NVM
- CLI simulation is available for pre-hardware validation
- KEYSCAN support is integrated for staged hardware bring-up
- The direct WSTK pushbuttons are kept for service actions; no separate pushbutton board is used on the EXP header

## Device Model

- Base sample: `zwave_soc_door_lock_keypad`
- Generic type: `GENERIC_TYPE_ENTRY_CONTROL`
- Specific type: `SPECIFIC_TYPE_SECURE_KEYPAD`
- User icon: `ICON_TYPE_SPECIFIC_ENTRY_CONTROL_KEYPAD_0_9`
- Installer icon: `ICON_TYPE_SPECIFIC_ENTRY_CONTROL_KEYPAD_0_9`
- Requested security: `S2 Access Control`
- Primary command class: `COMMAND_CLASS_ENTRY_CONTROL`
- Supporting command classes kept: `Indicator`, `Battery`, `Association`, `Supervision`, `Version`, `Manufacturer Specific`, `Z-Wave Plus Info`

## Removed Command Classes

| Command Class | Reason |
| --- | --- |
| `Door Lock CC` | Removed because this project is modeled as a keypad, not a lock actuator. |
| `User Code CC` | Removed because local code validation is not part of this keypad implementation. |
| `User Credential CC` | Removed because credential storage and management are not part of this keypad implementation. |
| `Basic CC` | Removed because it does not map cleanly to a secure Entry Control keypad. |

The original sample kept `Basic` because it was still a door-lock product and mapped `Basic` to Door Lock Operation. That behavior was removed along with the lock model.

## External Keypad Reference

The external keypad used for this proof of concept is the 20-key 4x5 membrane keypad shown in the Amazon listing below:

- Amazon reference: <https://www.amazon.com/dp/B07QH6JB23?ref=ppx_yo2ov_dt_b_fed_asin_title>
- Product family: generic `4x5` / `20-key` membrane matrix keypad
- Cable: `9`-conductor tail

Keypad reference image:

![Amazon keypad reference](zwave_soc_keypad/docs/images/keypad-amazon-reference.png)

## WSTK Buttons

This project uses the pushbuttons built directly into the `BRD4002A-A07` Wireless Starter Kit mainboard. It does not use an external button board on the expansion header.

Current WSTK button behavior:

| Button | Press Type | Action |
| --- | --- | --- |
| `BTN0` | Medium press | Send Battery Report |
| `BTN1` | Short press | Toggle Z-Wave learn mode |
| `BTN1` | Very long press | Factory reset |

`BTN0` short and long press are currently unused in this keypad project. The WSTK buttons remain useful for inclusion, exclusion, reset, and bench bring-up while the external keypad provides the actual Entry Control input path.

## Entry Control Behavior

The keypad caches input locally and reports it through Entry Control notifications.

- `Key Cache Size = 1` is valid if per-key reporting is desired.
- Current default: `Key Cache Size = 4`, `Key Cache Timeout = 2 seconds`
- A larger cache such as `32` with a `2` second timeout is a valid configuration for "enter digits, then press Enter" behavior.

Implemented Entry Control support:

- `ENTRY_CONTROL_KEY_SUPPORTED_GET/REPORT`
- `ENTRY_CONTROL_EVENT_SUPPORTED_GET/REPORT`
- `ENTRY_CONTROL_CONFIGURATION_GET/SET/REPORT`
- Lifeline notifications for `CACHING`, `CACHED_KEYS`, `ENTER`, and `CANCEL`

### Selected Key Mapping

This project is based on a 20-key membrane keypad with digits, `*`, `#`, `F1`, `F2`, `Esc`, `Ent`, and arrow keys.

| Key Type | Mapping |
| --- | --- |
| Buffered ASCII keys | `0-9`, `*`, `#` |
| Command keys | `Ent -> ENTER`, `Esc -> CANCEL` |
| Reserved for later | `F1`, `F2`, `Up`, `Down`, `Left`, `Right` |

Phase-1 supported Entry Control events:

- `CACHING`
- `CACHED_KEYS`
- `ENTER`
- `CANCEL`

Bitmask note for `Entry Control Key Supported Report`:

- bit `35` = `#`
- bit `42` = `*`
- bits `48` through `57` = `0` through `9`

## Hardware Plan

A `4x5` keypad requires `9` signals total. On EFR32ZG23, `KEYSCAN.ROW_SENSE_x` only routes on ports `A` and `B`, so the `4`-line dimension should be assigned to `ROW_SENSE`.

- KEYSCAN rows: `4`
- KEYSCAN columns: `5`

### Recommended BRD4210A Expansion-Header Wiring

| Function | GPIO | Expansion Pin |
| --- | --- | --- |
| Row 0 | `PA10` | `3` |
| Row 1 | `PA0` | `5` |
| Row 2 | `PA5` | `7` |
| Row 3 | `PA6` | `11` |
| Col 0 | `PD2` | `9` |
| Col 1 | `PC0` | `10` |
| Col 2 | `PA7` | `13` |
| Col 3 | `PC5` | `15` |
| Col 4 | `PC7` | `16` |

- VCOM TX/RX on EXP pins `12` and `14` stay available.
- VCOM CTS/RTS on EXP pins `3` and `5` are consumed.
- The onboard sensor I2C pins `15` and `16` are repurposed.
- EXP pins `4`, `6`, and `8` are avoided.

### KEYSCAN Configuration

- `SL_KEYSCAN_DRIVER_COLUMN_NUMBER = 5`
- `SL_KEYSCAN_DRIVER_ROW_NUMBER = 4`
- `SL_KEYSCAN_DRIVER_SINGLEPRESS = 1`
- `SL_KEYSCAN_DRIVER_DEBOUNCE_DELAY_MS = 2` or `4`
- `SL_KEYSCAN_DRIVER_STABLE_DELAY_MS = 2`

Pin routing should be done in the Simplicity Configurator / pin tool, not by hand-editing the `.pintool` file. During bring-up, it is valid to keep KEYSCAN inactive until build, flash, and debug behavior are confirmed.

## Validation

The most useful pre-hardware validation is controller `GET` and `REPORT` traffic.

Recommended checks:

- `Entry Control Key Supported Get`
  - expect `#`, `*`, and `0-9`
- `Entry Control Event Supported Get`
  - expect `CACHING`, `CACHED_KEYS`, `ENTER`, and `CANCEL`
- `Entry Control Configuration Get`
  - expect the configured cache size and timeout
- `Entry Control Configuration Set`
  - verify the node accepts valid values and returns them through `Configuration Get`

Physical key notifications require final pin-tool routing, KEYSCAN enable, and matrix-position-to-logical-key mapping.

## Remaining Work

1. Finish the pin-tool assignments.
2. Add the real KEYSCAN callback implementation.
3. Build the matrix-position-to-logical-key table for the actual keypad tail pinout.
4. Verify the physical wiring on the BRD4210A expansion header.
5. Add local indicator or buzzer feedback for key accepted, cancel, and transmit failure.
6. Run inclusion and interview testing and confirm Entry Control reports, configuration behavior, and lifeline notification delivery.
