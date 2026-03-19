# YD-CH32V307VCT6 ETHERNET-LWIP

![img](https://raw.githubusercontent.com/rtek1000/YD-CH32V307VCT6_ETHERNET-LWIP/refs/heads/main/IMG/Functional.png)

#### Code source:

https://github.com/rtek1000/wch-ch32v307-lwip

Technical Summary: LwIP + FreeRTOS Portability (CH32V307)
1. The MCU core files (vendor folder) have been updated, based on the [TCP server example](https://www.wch.cn/downloads/CH32V307EVT_ZIP.html) for [MounRiver Studio IDE](https://www.mounriver.com/download).

2. Ethernet Driver & Internal PHY
Hardware Identification: We confirmed that the board uses the internal 10Mbps PHY and that the RJ45 LEDs are mapped to Port C (PC0 and PC1), and not to Port B as in the WCH examples.

LED Remapping: We modified the eth_driver.c file to point ETH_LedLinkSet and ETH_LedDataSet to the correct pins (GPIOC_Pin_0 and GPIOC_Pin_1).

Activity Logic (ACT): We implemented a Persistence Flag (act_led_timer) within the RX/TX drivers (ChainMode). This ensures that the LED shines brightly for at least 100ms when detecting traffic, avoiding the "dim brightness" effect.

3. Link Monitoring and System Health
Hardware Task (led_link_task): We created a dedicated high-frequency task (running every 10ms-50ms) to:

Read the PHY physical register (ETH_ReadPHYRegister) to instantly turn off the Link LED when the cable is disconnected.

Manage the activity LED countdown (ACT).

Notify the LwIP stack about changes in the cable state (netif_set_link_down/up).

Memory Diagnostics: We implemented FreeRTOS Heap monitoring and stack High Water Mark. We found that the system is operating with ~15KB of free RAM and that the main task uses less than 10% of the allocated stack.

#### Schematic (Partial)

![img](https://raw.githubusercontent.com/rtek1000/YD-CH32V307VCT6_ETHERNET-LWIP/refs/heads/main/IMG/CH32V307VCT6_partial_schematic.png)

#### Notes
- - Compilation requires a toolchain: [MRS_Toolchain_Linux_X64_V240.tar.xz](https://www.mounriver.com/download) Using VsCode (Makefile).
- - The CH32V307VCT6 can be configured to operate in different operating modes with different ROM/RAM sizes. Configuration may be required via the [WCH-LinkUtility program](https://raw.githubusercontent.com/rtek1000/YD-CH32V307VCT6_ETHERNET-LWIP/refs/heads/main/IMG/WCH-LinkUtility_v2_60.png). (See /vendor/Ld/Link.ld file)
- - Code adapted for the YD-CH32V307VCT6 board with the help of Gemini (Google AI) and ChatGPT.
- - The TF card jumpers are off because Ethernet is in use. Unfortunately, the CH32V307VCT6 MCU has shared pins between the SD card and Ethernet. Perhaps the SD card could be adapted to use the SPI port.
- - This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
