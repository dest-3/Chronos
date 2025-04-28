## Table of Contents

1. [Introduction](#introduction)
2. [Use Cases](#use-cases)
3. [Repository Structure](#repository-structure)
4. [Target Platform](#target-platform)
5. [Setting up Chronos](#setting-up-chronos)
    1. [Install Code Composer Studio (CCS)](#install-code-composer-studio-ccs)
    2. [Clone the Repository](#clone-the-repository)
    3. [Import Projects into CCS](#import-projects-into-ccs)
    4. [Configure Security Policy](#configure-security-policy)
    5. [Optional: Enforce Return Address Validation](#optional-enforce-return-address-validation)
    6. [Configure Network](#configure-network)
    7. [Run the UDP Server](#run-the-udp-server)
    8. [Build and Flash a Project](#build-and-flash-a-project)
6. [License](#license)
7. [Citation](#citation)

## Introduction

Chronos was developed as a lightweight kernel extension that brings endpoint detection and response (EDR) capabilities to real-time embedded systems. Chronos employs timing-based detection mechanisms to identify abnormal task behavior and enforces memory separation through the Memory Protection Unit (MPU) to isolate EDR and kernel code from untrusted application code. It dynamically adapts to system load, reducing the frequency of security checks during high utilization to maintain responsiveness, and increasing it during low utilization to enhance security coverage. To detect reconnaissance and tampering attempts, Chronos instruments OS kernel APIs, blocking unauthorized modifications to security-critical code and data structures. It also enforces return address integrity for FreeRTOS yield APIs by validating return addresses against a per-task whitelist. When a security event is detected, forensic data is transmitted to a remote server for real-time threat analysis. Chronos is implemented as an extension to FreeRTOS and evaluated on a system that simulates UAV operations. Performance was measured using the CoreMark benchmark. Under the most aggressive security policy configuration, Chronos incurred a runtime overhead of 0.86% and a 45.1% increase in code size.

For the details of Chronos, check the [paper]() - coming soon

## Use Cases

Chronos is intended for deployment in real-time embedded systems such as:

- UAV flight controllers  
- Automotive braking and steering systems  
- Industrial robotics and automation  

## Repository Structure

- `halcogen/` — HAL configuration project for the TI Hercules RM48L952ZWTT microcontroller.
- `workspace/` — Contains FreeRTOS-based example projects for performance evaluation, security testing, and UAV simulation.
  - `coremark_drone/` — Measures performance overhead introduced by Chronos in a UAV workload environment.
  - `coremark_scale/` — Demonstrates linear scaling of performance overhead as more tasks are added to the system.
  - `sec_eval/` — Executes configurable security test cases to demonstrate detection and response guarantees.
  - `main/` — Baseline UAV project serving as a template for deployment and extension.
  - `*/gen_edr_config.py` - Script to configure security policy and generate the relevant EDR code and header files. 
- `get_yield_ret_addr.py` — Script for extracting YIELD API return addresses from compiled firmware to support return address validation.
- `udp_serv/` — Python-based server that receives and logs forensic metadata sent by Chronos during security events.
- `ema_sma_spike_plot.py` — Visualization script that compares the responsiveness of EMA vs. SMA in detecting execution time anomalies.

## Target Platform

Chronos is currently designed for:
- FreeRTOS 10.2.0
- ARM Cortex-R (ARMv7-R) processors
- Systems with an MPU (tested on TI Hercules RM48L952ZWTT)

## Setting up Chronos

To get started with Chronos on the TI Hercules RM48L952ZWTT development board:

1. **Install Code Composer Studio (CCS)**  
   Download and install [Code Composer Studio](https://www.ti.com/tool/CCSTUDIO). Ensure support for the RM48 series and XDS100v2 JTAG is enabled during installation.

2. **Clone the Repository**  
   `git clone https://github.com/your-repo/chronos.git`

3. **Import Projects into CCS**<br>
    Open CCS, go to **File > Import > Code Composer Studio > CCS Projects**, and select the `workspace/` folder. Then select all example projects. 

5. **Configure Security Policy**<br>
    Modify the JSON configuration in `gen_edr_config.py` in the selected project folder to configure the security configuration of Chronos. Then run `python3 gen_edr_config.py` to generate the assosciated code and header files.
    
6. **(Optional) Enforce Return address validation**<br>
    If enforcing return address validation for yield APIs:
    1. Add any relevant task code in `main.c`. 
    2. Compile the firmware. 
    3. Use `get_yield_ret_addr.py` to obtain the return addresses for each yield function in task bodies. For example, for tasks test1, test2, test3 that utilize `vTaskDelay` and `vTaskDelayUntil`, run `python3 get_yield_ret_addr.py firmware.out test1 test2 test3 -- vTaskDelay vTaskDelayUntil`
    4. Add addresses in the JSON config of `gen_edr_config.py` and run `python3 gen_edr_config.py`.
    5. Compile

7. **Configure Network**<br>
    Run a DHCP on your host machine and connect the RM48 via Ethernet. Use the following DHCP server settings:
    ```
       IP pool start address: 192.168.4.100
       Size of Pool: 5
       Lease (minutes): 2000
       Router: 192.168.4.1
       Mask: 255.255.255.0
    ```
    Ensure the server does not enforce pinging an addresses before IP assignment. Also ensure that the Ethernet switch on the RM48 is set to ON. For a quick and easy DHCP setup [TFPD64](https://www.intel.com/content/www/us/en/docs/programmable/683536/current/tftpd64-by-ph-jounin-installation.html) is recommended. 

8. **Run the UDP Server**<br>
    `python3 udp_serv.py`

9. **Build and Flash a Project**<br>
    In CCS, build one of the projects (e.g., coremark_drone) and flash it to the board via USB JTAG by navigating to **Run > Debug** or **Run > Load**

## License

MIT License. See `LICENSE` for details. 

## Citation

If you use Chronos in academic work, please cite as:

```Bibtex
@mastersthesis{chronos2025,
  author       = {Michalis Antoniades},
  title        = {Chronos: Efficient Endpoint Detection and Response for Safety-Critical Real-Time Embedded Systems},
  school       = {Carnegie Mellon University},
  year         = {2025},
  note         = {\url{https://github.com/dest-3/chronos}}
}
```








