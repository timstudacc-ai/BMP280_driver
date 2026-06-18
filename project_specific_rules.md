# BMP280 Project Specific Rules

## Configuration Write Constraint (Critical)

**RULE:** Before writing any data to the `CONFIG` register (0xF5) — such as setting the Standby Time (`t_sb`) or IIR Filter (`filter`) — you **MUST** ensure the BMP280 is in `SLEEP` mode (00). 

If the sensor is currently in `NORMAL` mode (11) or `FORCED` mode (01/10), any write operations to the `CONFIG` register may be ignored by the hardware.

### Implementation Steps:
1. Read the `CTRL_MEAS` register (0xF4).
2. Apply the mask `~BMP280_CTRL_MEAS_MODE_MSK` and write `BMP280_MODE_SLEEP` (0x00) to put the sensor to sleep.
3. Perform the necessary configuration writes to the `CONFIG` register (0xF5).
4. If required, restore the previous mode (e.g., `NORMAL` or `FORCED`) by rewriting to `CTRL_MEAS`.
