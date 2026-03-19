# YD-CH32V307VCT6 ETHERNET-LWIP

![img](https://raw.githubusercontent.com/rtek1000/YD-CH32V307VCT6_ETHERNET-LWIP/refs/heads/main/IMG/Functional.png)

#### Code source:

https://github.com/rtek1000/wch-ch32v307-lwip

Technical Summary: LwIP + FreeRTOS Portability (CH32V307)
1. Startup Adjustments (Startup & Linker)
Interrupt Vector: We corrected the startup_ch32v30x_D8C.S file to include the Software_Handler. Without it, FreeRTOS could not perform task switching (context switch).

Software Bridge: We linked the Software_Handler to the vPortSoftwareInterruptHandler function and the SysTick_Handler to the xPortSysTickHandler to give "life" to the RTOS kernel.

Memory (LD): We adjusted the Linker Script to ensure that the C stack and heap did not collide with the FreeRTOS Heap.

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
- - Code adapted for the YD-CH32V307VCT6 board with the help of Gemini (Google AI) and ChatGPT.
- - The TF card jumpers are off because Ethernet is in use. Unfortunately, the CH32V307VCT6 MCU has shared pins between the SD card and Ethernet. Perhaps the SD card could be adapted to use the SPI port.
- - This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
