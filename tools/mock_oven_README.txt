Mock Siemens S1200 oven (no real hardware)
==========================================

Use when the test PC and a desktop PC are on the same LAN (Ethernet).

Topology
--------
  [Test PC]  --Ethernet-->  [Desktop PC]
  QtechCalibration.exe       mock_oven_s1200.py :8000

Step 1 - On the DESKTOP (fake oven)
-----------------------------------
1. Copy this folder tools\ to the desktop, or map a share from the dev PC.
2. Install Python 3 if missing: https://www.python.org/downloads/
3. Double-click run_mock_oven_s1200.bat
   OR:  py -3 mock_oven_s1200.py --host 0.0.0.0 --port 8000
4. Note the desktop LAN IP:  cmd> ipconfig
   Example: 192.168.1.50
5. Windows Firewall: allow inbound TCP port 8000 on the desktop.

Step 2 - On the TEST PC (QtechCalibration)
------------------------------------------
GateSpec.ini [oven]:
  Enabled=1
  Host=192.168.1.50     <-- desktop IP, NOT 127.0.0.1
  Port=8000
  UnitId=1
  Profile=S1200
  TempScale=10
  DualChamber=1

Quick check from test PC:
  ping 192.168.1.50
  (optional) Test-NetConnection 192.168.1.50 -Port 8000   PowerShell

Step 3 - Verify in Spec dialog (温箱/老化 tab)
----------------------------------------------
1. 连接测试  -> Connect OK, U/D PV ~ 25 C
2. 设温 / 启动 -> setTemp + START; PV ramps ~1 C every 2 s
3. At setpoint during aging: run=1 heat=1 pv holds ~85 C (no early cooldown)
4. After aging: app setTemp(40) -> pv ramps down; final Stop when cooled

Cooldown alignment (matches QtechCalibration)
---------------------------------------------
1. Heat/Start + setTemp(85) -> hold at 85 C during aging (run=1, heat=1, pv stable).
2. When aging ends, app writes setTemp(CooldownTargetC) e.g. 40 C (cooldown command).
3. Mock ramps PV down toward new setpoint; app polls until PV <= target+tol, then Stop.

Full aging flow (optional)
--------------------------
Shorten timeouts in GateSpec.ini for desk testing:
  WaitTimeoutMin=10
  PollIntervalMs=3000
  CooldownTimeoutMin=10
  CooldownTargetC=40
  [aging_test] DurationMin=2

Fault test: use Modbus Poll to write holding reg 3 or 33 = 1,
or write run reg 2/32 = 2; software should treat as fault/timeout path.

Files
-----
  mock_oven_sim_core.py       Shared thermal model (copy with scripts)
  mock_oven_s1200_stdlib.py   **Use this on offline PCs** (Python 3 only, no pip)
  mock_oven_s1200.py          Optional; needs: pip install pymodbus
  run_mock_oven_s1200.bat     Starts stdlib version (no internet)
