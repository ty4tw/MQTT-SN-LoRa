# LoRaMQTT-SN

## LoRaLink sensor network for MQTT-SN and MQTT-SN Client device firmware.
You have to modify LoRaLink/LoaLink.c code, if you use this outside of JAPAN.
![LoRaLinkMQTT-SN](https://user-images.githubusercontent.com/7830788/87372621-fef4ba00-c5c2-11ea-80ce-6a39930d8f18.png)

## Requirements
1. Three B-L0722Z-LRWAN, RxDevice and TxDevice for the gateway and one for the client device.
2. Three USB-UART converters
3. Install GNU Arm Embedded Toolchain 
https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
4. Install STM32CubeProgrammer
https://www.st.com/en/development-tools/stm32cubeprog.html

## How to build
### 1. MQTT-SN Gateway
````
   git clone -b experiment https://github.com/eclipse/paho.mqtt-sn.embedded-c.git
   make SENSORNET=loralink
````
   
### 2. Sensornetwork devices
````
   git clone https://github.com/ty4tw/LoRaMQTT-SN.git
````
   #### 2-1 RxDevice for the Gateway
````
       make TYPE=RXMODEM
       Download Build/Firmware.bin to B-L0722Z-LRWAN with STM32CubeProgrammer.
````
   #### 2-2 TxDevice for the gateway
````
       touch AppSrc/test.c
       make TYPE=TXMODEM
       Download Build/Firmware.bin to B-L0722Z-LRWAN with STM32CubeProgrammer
````      
   #### 2-3 Client Device
 ````
       touch AppSrc/test.c
       make TYPE=CLIENT LOG=DEBUG
       Download Build/Firmware.bin to B-L0722Z-LRWAN with STM32CubeProgrammer
```` 
## Device
### This SDK is developped for the LoRaEz module. But B-L0722Z-LRWAN board is available insted of the module.
![LoRaEz](https://user-images.githubusercontent.com/7830788/87379771-f81e7500-c5cb-11ea-87a3-98fca09ac8fe.png)

## LoRaLink
#### LoRaLink is a simple protocol prepared for MQTT-SN.  It's a different from LoRaWAN.
![LoRaLink](https://user-images.githubusercontent.com/7830788/87379815-12f0e980-c5cc-11ea-8a06-9a2498acdff4.png)
## Sensor Network Application layer protocol
This protocol is handled in loralink/SensorNetwork.cpp
![LoRaLinkApi](https://user-images.githubusercontent.com/7830788/87870157-86d22e00-c9e0-11ea-8c3d-b472a2dff25a.png)
# Required devices
![LoRaMQTT](https://user-images.githubusercontent.com/7830788/87870330-c2b9c300-c9e1-11ea-99a8-c8dc6585aa33.png)
