# PPK2 host tools

Host-side helpers for the Nordic Power Profiler Kit II running Dawn
(`boards/arm/nrf52/ppk2`, `nxscope` config): nxscope over USB CDC/ACM.

- `ppk2.py` - all-in-one control: SMU on/off, loads, IO get/set, live
  plot, measure/CSV capture, monitor, IoT demo (`ppk2.py --help`).
- `ppk2_cal.py` - self-calibration sweep on the onboard loads; stores the
  result in the board EEPROM (source of truth) and a JSON file.
- `ppk2_dev.py` - shared `Ppk2` device class used by both.

IO access goes through the `dawn-nxscope` plugin (SET_IO/GET_IO user
frames), the current stream through nxslib `stream_sub`. Object IDs are
resolved by nxscope channel name from `descriptors/examples/
ppk2_nxscope.yaml` (`--descriptor` to override) via dawnpy.

Requires `dawnpy`, `dawn-nxscope` (`pip install -e tools/dawnpy
tools/dawn-nxscope`) and `pyqtgraph` for `plot`/`demo`.

```sh
python tools/examples/ppk2/ppk2.py info
python tools/examples/ppk2/ppk2.py cal            # once per board
python tools/examples/ppk2/ppk2.py on 3.3 && python tools/examples/ppk2/ppk2.py plot
```

`--rate` is the wire sample rate (default: measured from the stream);
`--cal-file` the JSON fallback next to the scripts, used only when the
EEPROM holds no calibration blob.
