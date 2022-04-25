# stm32f103_mcp2515_canhacker
Can hacker ;) via STM32F103C6T6 (from china with 32k rom) 
And MCP2515 over SPI
For Keil


Часть  кода и логики спизжена с других проектов:

https://github.com/eziya/STM32_SPI_MCP2515

https://github.com/autowp/arduino-canhacker

Work with: 

	CanHacker V2.00.02
	
	CAN Analyzer ( https://www.elektronik-keller.de/index.php/stm32-can-support )

	CANcool ( https://github.com/MHS-Elektronik/CANcool )

 *  PINouts:
 *  BOARD | MCP2515
 *  +3.3v = VCC      (via 5v = bug ? )
 *  GND   = GND
 *  PB13  = CS   - PIN LABEL = CAN_CS
 *  PA6   = SO
 *  PA7   = SI
 *  PA5   = SCK

   