# Shared S1200 dual-chamber thermal model for mock_oven_s1200*.py
#
# Flow aligned with QtechCalibration:
#   1. START + setTemp(aging target) -> heat up, then HOLD at setpoint (run=1, heat=1).
#   2. Aging ends -> app writes setTemp(cooldownTarget) as "cooldown command".
#   3. While run=1, ramp PV down toward new lower setpoint.
#   4. After PV <= cooldown target, app sends STOP (run=0).

from __future__ import annotations

import logging
import threading

LOG = logging.getLogger("mock_oven")

U_START, U_STOP, U_RUN, U_FAULT, U_SET, U_PV = 0, 1, 2, 3, 4, 6
D_START, D_STOP, D_RUN, D_FAULT, D_SET, D_PV = 30, 31, 32, 33, 34, 36

HR_COUNT = 64
TEMP_SCALE = 10
RAMP_RAW_PER_TICK = 10
COOL_RAW_PER_TICK = 10
AMBIENT_C = 25.0


def raw_to_c(raw: int) -> float:
    return raw / TEMP_SCALE


def c_to_raw(temp_c: float) -> int:
    return int(round(temp_c * TEMP_SCALE))


class _Chamber:
    __slots__ = ("target_raw", "pv_raw", "program_run", "heating", "fault")

    def __init__(self, target_c: float = 85.0, pv_c: float = AMBIENT_C) -> None:
        self.target_raw = c_to_raw(target_c)
        self.pv_raw = c_to_raw(pv_c)
        self.program_run = False
        self.heating = False
        self.fault = 0

    def run_reg(self) -> int:
        if self.fault:
            return 2
        return 1 if self.program_run else 0

    def start(self) -> None:
        if self.fault:
            return
        self.program_run = True
        self.heating = True

    def stop(self) -> None:
        self.program_run = False
        self.heating = False

    def set_fault(self, value: int) -> None:
        self.fault = value & 0xFFFF
        if self.fault:
            self.heating = False

    def apply_set_temp(self, value: int) -> None:
        self.target_raw = value & 0xFFFF
        if self.program_run and not self.fault:
            self.heating = True

    def tick(self) -> None:
        ambient = c_to_raw(AMBIENT_C)
        if self.fault:
            return
        if not self.program_run:
            if self.pv_raw > ambient:
                self.pv_raw = max(self.pv_raw - COOL_RAW_PER_TICK, ambient)
            return
        if not self.heating:
            return
        if self.pv_raw < self.target_raw:
            self.pv_raw = min(self.pv_raw + RAMP_RAW_PER_TICK, self.target_raw)
        elif self.pv_raw > self.target_raw:
            self.pv_raw = max(self.pv_raw - COOL_RAW_PER_TICK, self.target_raw)


class S1200Simulator:
    """Thread-safe S1200 holding-register simulator."""

    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.hr = [0] * HR_COUNT
        self.u = _Chamber()
        self.d = _Chamber()
        self._sync_hr()

    def _sync_hr(self) -> None:
        self.hr[U_RUN] = self.u.run_reg()
        self.hr[U_FAULT] = self.u.fault
        self.hr[U_SET] = self.u.target_raw
        self.hr[U_PV] = self.u.pv_raw
        self.hr[D_RUN] = self.d.run_reg()
        self.hr[D_FAULT] = self.d.fault
        self.hr[D_SET] = self.d.target_raw
        self.hr[D_PV] = self.d.pv_raw

    def on_write(self, address: int, value: int) -> None:
        value &= 0xFFFF
        with self.lock:
            if address == U_SET:
                self.u.apply_set_temp(value)
                LOG.info("U setTemp -> %.1f C (raw=%d)", raw_to_c(value), value)
            elif address == D_SET:
                self.d.apply_set_temp(value)
                LOG.info("D setTemp -> %.1f C (raw=%d)", raw_to_c(value), value)
            elif address == U_START:
                if self.u.fault:
                    LOG.warning("U START ignored (fault active)")
                else:
                    self.u.start()
                    LOG.info("U START (program=1, heating=1)")
            elif address == U_STOP:
                self.u.stop()
                LOG.info("U STOP (program=0, heating=0)")
            elif address == D_START:
                if self.d.fault:
                    LOG.warning("D START ignored (fault active)")
                else:
                    self.d.start()
                    LOG.info("D START (program=1, heating=1)")
            elif address == D_STOP:
                self.d.stop()
                LOG.info("D STOP (program=0, heating=0)")
            elif address == U_FAULT:
                self.u.set_fault(value)
                LOG.info("U fault -> %d", value)
            elif address == D_FAULT:
                self.d.set_fault(value)
                LOG.info("D fault -> %d", value)
            self._sync_hr()

    def read_holding(self, address: int, quantity: int) -> list[int]:
        with self.lock:
            self._sync_hr()
            out: list[int] = []
            for i in range(quantity):
                idx = address + i
                out.append(self.hr[idx] & 0xFFFF if 0 <= idx < HR_COUNT else 0)
            return out

    def write_single(self, address: int, value: int) -> None:
        self.on_write(address, value)

    def tick(self) -> None:
        with self.lock:
            self.u.tick()
            self.d.tick()
            self._sync_hr()
            LOG.info(
                "tick  U: run=%d heat=%d set=%.1f pv=%.1fC  D: run=%d heat=%d set=%.1f pv=%.1fC",
                self.u.run_reg(),
                1 if self.u.heating else 0,
                raw_to_c(self.u.target_raw),
                raw_to_c(self.u.pv_raw),
                self.d.run_reg(),
                1 if self.d.heating else 0,
                raw_to_c(self.d.target_raw),
                raw_to_c(self.d.pv_raw),
            )

    def sync_to_block(self, block) -> None:
        with self.lock:
            self._sync_hr()
            block.setValues(U_RUN, [self.hr[U_RUN]])
            block.setValues(U_FAULT, [self.hr[U_FAULT]])
            block.setValues(U_SET, [self.hr[U_SET]])
            block.setValues(U_PV, [self.hr[U_PV]])
            block.setValues(D_RUN, [self.hr[D_RUN]])
            block.setValues(D_FAULT, [self.hr[D_FAULT]])
            block.setValues(D_SET, [self.hr[D_SET]])
            block.setValues(D_PV, [self.hr[D_PV]])
