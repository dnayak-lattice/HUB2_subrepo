# Host-accelerated Unified Bridge (H.U.B.)  

Development repository for the HUB, Applications and GARD FW.

**IMPORTANT NOTE:**
- The latest stable release of HUB and apps related code is in the **"main"** branch.
- The latest stable release of GARD FW code is in the **"fw_core-main"** branch.

## HUB
HUB stands for **Host-accelerated Unified Bridge** - a software framework for connecting to and controlling the GARD and GARD FW.

HUB source code compiles to create a shared (or static) library, that can be included by applications and/or other libraries for creating various applications.

HUB source is intended to be as OS agnostic as possible.

The aim of HUB is to function as an active conduit for control and data transfers between:
- Host Applications
- Edge AI designs instantiated in the GARD

This enables the development of a gamut of applications seeking to accelerate AI/ML operations on Lattice FPGAs.

## KtLib (Katana Library)
KtLib is a customer specific library implementation that uses the HUB library to talk to hardware and the attached GARD / GARD FW.

KtLib is intended to be OS-agnostic; all OS-specific dependencies and functionalities are expected to be fulfilled by the relevant Application.

## Apps
A variety of applications can be developed to demonstrate the usage of the underlying HUB and/or KtLib libraries.

This repository contains Katana App (kt_app) that uses KtLib and HUB in combination, and demonstrates interaction with Firmware running on the GARD. 

kt_app illustrates the collection of AI-related metadata from models such such as Person Detection, Face Detection, Face Identification, etc.

This repository contains an interactive version (console-based) and an IPC-server based. A Python Flask based web application is also included to help visualize the rendering of the AI metadata, as well as to demonstrate the usage of **Register Face ID**.

## GARD
GARD stands for **Golden AI Reference Design**. It is an FPGA RTL design running on a Lattice FPGA, that instantiates various sub-systems such as:
1. RISC-V subsystem IP
2. ML engine IP (could include ISP / mini-ISP)
3. Host Interface IP
4. User Application IP [optional, customer-defined]

The aim being to enable Lattice FPGA buyers get a go-to platform for setting up and evaluating a variety of ML models and use-case offerings from Lattice with minimal setup and developmental effort.

## GARD FW
The RISC-V subsystem on the GARD runs the GARD FW (firmware).

Its functions include, but are not limited to:
1. RISC-V CPU bootup
2. Interaction with the Host over serial interfaces (I2C, UART, etc.)
3. ML engine IP control
4. Sensor (camera, etc.) control and coordination of ML IP (and constituents)
5. Selective pre- and/or post- processing of sensor data
