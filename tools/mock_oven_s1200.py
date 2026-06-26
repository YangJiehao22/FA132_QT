#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Siemens S1200 dual-chamber Modbus TCP mock oven (pymodbus).

Requires: pip install pymodbus

Thermal model: see mock_oven_sim_core.py (cooldown aligned with QtechCalibration).
"""

from __future__ import annotations

import argparse
import logging
import sys
import threading
from typing import List

try:
    from pymodbus.datastore import ModbusDeviceContext, ModbusSequentialDataBlock, ModbusServerContext
    from pymodbus.server import StartTcpServer
except ImportError:
    print("Missing pymodbus. Use mock_oven_s1200_stdlib.py instead (no pip).", file=sys.stderr)
    sys.exit(1)

from mock_oven_sim_core import HR_COUNT, S1200Simulator

LOG = logging.getLogger("mock_oven")


class S1200DataBlock(ModbusSequentialDataBlock):
    def __init__(self, sim: S1200Simulator) -> None:
        super().__init__(0, [0] * HR_COUNT)
        self.sim = sim
        sim.sync_to_block(self)

    def setValues(self, address: int, values: List[int]) -> None:  # type: ignore[override]
        super().setValues(address, values)
        for offset, val in enumerate(values):
            self.sim.on_write(address + offset, int(val) & 0xFFFF)
        self.sim.sync_to_block(self)


def sim_thread(sim: S1200Simulator, block: S1200DataBlock, interval: float, stop: threading.Event) -> None:
    while not stop.wait(interval):
        sim.tick()
        sim.sync_to_block(block)


def main() -> None:
    parser = argparse.ArgumentParser(description="Mock Siemens S1200 dual-chamber oven (Modbus TCP)")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--unit-id", type=int, default=1)
    parser.add_argument("--tick", type=float, default=2.0)
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        datefmt="%H:%M:%S",
    )

    sim = S1200Simulator()
    hr_block = S1200DataBlock(sim)
    store = ModbusDeviceContext(hr=hr_block, zero_mode=True)
    context = ModbusServerContext(devices={args.unit_id: store}, single=False)

    stop = threading.Event()
    threading.Thread(
        target=sim_thread, args=(sim, hr_block, args.tick, stop), name="oven-sim", daemon=True
    ).start()

    LOG.info("Mock S1200 on %s:%d unitId=%d", args.host, args.port, args.unit_id)
    LOG.info("Hold at setTemp during aging; cooldown on lower setTemp write")

    try:
        StartTcpServer(context=context, address=(args.host, args.port))
    except KeyboardInterrupt:
        LOG.info("Stopping...")
    finally:
        stop.set()


if __name__ == "__main__":
    main()
