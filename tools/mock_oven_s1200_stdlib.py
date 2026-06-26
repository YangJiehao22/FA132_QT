#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Siemens S1200 dual-chamber Modbus TCP mock oven — NO third-party packages.

  py -3 mock_oven_s1200_stdlib.py --host 0.0.0.0 --port 8000

Thermal model (see mock_oven_sim_core.py):
  START -> heat to setTemp and HOLD at setpoint during aging.
  App setTemp(cooldownTarget) after aging -> ramp PV down; STOP when done.
"""

from __future__ import annotations

import argparse
import logging
import socket
import struct
import threading

from mock_oven_sim_core import S1200Simulator

LOG = logging.getLogger("mock_oven")

FC_READ_HOLDING = 0x03
FC_WRITE_SINGLE = 0x06


def build_mbap_response(tid: int, unit_id: int, pdu: bytes) -> bytes:
    length = 1 + len(pdu)
    return struct.pack(">HHH", tid, 0, length) + bytes([unit_id & 0xFF]) + pdu


def build_exception(tid: int, unit_id: int, fc: int, code: int) -> bytes:
    return build_mbap_response(tid, unit_id, bytes([fc | 0x80, code & 0xFF]))


def handle_request(data: bytes, sim: S1200Simulator) -> bytes | None:
    if len(data) < 8:
        return None
    tid, proto, length = struct.unpack(">HHH", data[0:6])
    if proto != 0 or len(data) < 6 + length:
        return None
    unit_id = data[6]
    pdu = data[7 : 6 + length]
    if not pdu:
        return build_exception(tid, unit_id, 0, 0x01)

    fc = pdu[0]
    if fc == FC_READ_HOLDING:
        if len(pdu) < 5:
            return build_exception(tid, unit_id, fc, 0x03)
        addr, qty = struct.unpack(">HH", pdu[1:5])
        if qty < 1 or qty > 125:
            return build_exception(tid, unit_id, fc, 0x03)
        regs = sim.read_holding(addr, qty)
        body = struct.pack(">BB", fc, qty * 2)
        for r in regs:
            body += struct.pack(">H", r)
        return build_mbap_response(tid, unit_id, body)

    if fc == FC_WRITE_SINGLE:
        if len(pdu) < 5:
            return build_exception(tid, unit_id, fc, 0x03)
        addr, val = struct.unpack(">HH", pdu[1:5])
        sim.write_single(addr, val)
        return build_mbap_response(tid, unit_id, pdu)

    return build_exception(tid, unit_id, fc, 0x01)


def handle_client(conn: socket.socket, addr, sim: S1200Simulator) -> None:
    conn.settimeout(30.0)
    try:
        while True:
            buf = b""
            while len(buf) < 7:
                part = conn.recv(4096)
                if not part:
                    return
                buf += part
            need = 6 + struct.unpack(">H", buf[4:6])[0]
            while len(buf) < need:
                part = conn.recv(4096)
                if not part:
                    return
                buf += part
            rsp = handle_request(buf, sim)
            if rsp:
                conn.sendall(rsp)
    except (ConnectionResetError, BrokenPipeError, TimeoutError, OSError):
        pass
    finally:
        conn.close()
        LOG.debug("client closed %s", addr)


def sim_loop(sim: S1200Simulator, interval: float, stop: threading.Event) -> None:
    while not stop.wait(interval):
        sim.tick()


def serve(host: str, port: int, tick: float) -> None:
    sim = S1200Simulator()
    stop = threading.Event()
    threading.Thread(target=sim_loop, args=(sim, tick, stop), daemon=True, name="oven-sim").start()

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((host, port))
    srv.listen(8)
    LOG.info("Mock S1200 (stdlib) on %s:%d", host, port)
    LOG.info("Hold at setTemp during aging; cooldown when app writes lower setTemp")

    try:
        while True:
            conn, addr = srv.accept()
            LOG.debug("connect %s", addr)
            threading.Thread(target=handle_client, args=(conn, addr, sim), daemon=True).start()
    except KeyboardInterrupt:
        LOG.info("Stopping...")
    finally:
        stop.set()
        srv.close()


def main() -> None:
    parser = argparse.ArgumentParser(description="Mock S1200 oven — Python stdlib only")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--tick", type=float, default=2.0, help="sim tick interval seconds")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        datefmt="%H:%M:%S",
    )
    serve(args.host, args.port, args.tick)


if __name__ == "__main__":
    main()
