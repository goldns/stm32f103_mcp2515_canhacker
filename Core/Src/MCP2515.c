#include "MCP2515.h"
#include "main.h"
/* Pin ??? ?? ????. Modify below items for your SPI configurations */
extern SPI_HandleTypeDef        hspi1;
#define SPI_CAN                 &hspi1
#define SPI_TIMEOUT             10
#define MCP2515_CS_HIGH()   HAL_GPIO_WritePin(CAN_CS_GPIO_Port, CAN_CS_Pin, GPIO_PIN_SET)
#define MCP2515_CS_LOW()    HAL_GPIO_WritePin(CAN_CS_GPIO_Port, CAN_CS_Pin, GPIO_PIN_RESET)

/* Prototypes */
static void SPI_Tx(uint8_t data);
static void SPI_TxBuffer(uint8_t *buffer, uint8_t length);
static uint8_t SPI_Rx(void);
static void SPI_RxBuffer(uint8_t *buffer, uint8_t length);

/* MCP2515 ??? */

bool MCP2515_Initialize(void){
    MCP2515_CS_HIGH();
    uint8_t loop = 10;
    do {
        /* SPI Ready ?? */
        if(HAL_SPI_GetState(SPI_CAN) == HAL_SPI_STATE_READY)
            return true;
        loop--;
    } while(loop > 0);
    return false;
}

/* MCP2515 ? ????? ?? */
bool MCP2515_SetConfigMode(void){
    /* CANCTRL Register Configuration ?? ?? */
    MCP2515_WriteByte(MCP2515_CANCTRL, 0x80);
    uint8_t loop = 10;
    do {
        /* ???? ?? */
        if((MCP2515_ReadByte(MCP2515_CANSTAT) & 0xE0) == 0x80)
            return true;
        loop--;
    } while(loop > 0);
    return false;
}

/* MCP2515 ? Normal??? ?? */
bool MCP2515_SetNormalMode(void)
{
    /* CANCTRL Register Normal ?? ?? */
    MCP2515_WriteByte(MCP2515_CANCTRL, 0x00);

    uint8_t loop = 10;

    do {
        /* ???? ?? */
        if((MCP2515_ReadByte(MCP2515_CANSTAT) & 0xE0) == 0x00)
            return true;

        loop--;
    } while(loop > 0);

    return false;
}

/* MCP2515 ? Sleep ??? ?? */
bool MCP2515_SetSleepMode(void)
{
    /* CANCTRL Register Sleep ?? ?? */
    MCP2515_WriteByte(MCP2515_CANCTRL, 0x20);

    uint8_t loop = 10;

    do {
        /* ???? ?? */
        if((MCP2515_ReadByte(MCP2515_CANSTAT) & 0xE0) == 0x20)
            return true;

        loop--;
    } while(loop > 0);

    return false;
}

/* MCP2515 SPI-Reset */
void MCP2515_Reset(void)
{
    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_RESET);

    MCP2515_CS_HIGH();
}

/* 1??? ?? */

uint8_t MCP2515_ReadByte (uint8_t address)
{
    uint8_t retVal;

    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_READ);
    SPI_Tx(address);
    retVal = SPI_Rx();

    MCP2515_CS_HIGH();

    return retVal;
}

/* Sequential Bytes ?? */
void MCP2515_ReadRxSequence(uint8_t instruction, uint8_t *data, uint8_t length)
{
    MCP2515_CS_LOW();

    SPI_Tx(instruction);
    SPI_RxBuffer(data, length);

    MCP2515_CS_HIGH();
}

/* 1??? ?? */
void MCP2515_WriteByte(uint8_t address, uint8_t data)
{
    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_WRITE);
    SPI_Tx(address);
    SPI_Tx(data);

    MCP2515_CS_HIGH();
}

/* Sequential Bytes ?? */
void MCP2515_WriteByteSequence(uint8_t startAddress, uint8_t endAddress, uint8_t *data)
{
    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_WRITE);
    SPI_Tx(startAddress);
    SPI_TxBuffer(data, (endAddress - startAddress + 1));

    MCP2515_CS_HIGH();
}

/* TxBuffer? Sequential Bytes ?? */
void MCP2515_LoadTxSequence(uint8_t instruction, uint8_t *idReg, uint8_t dlc, uint8_t *data)
{
    MCP2515_CS_LOW();

    SPI_Tx(instruction);
    SPI_TxBuffer(idReg, 4);
    SPI_Tx(dlc);
    SPI_TxBuffer(data, dlc);

    MCP2515_CS_HIGH();
}

/* TxBuffer? 1 Bytes ?? */
void MCP2515_LoadTxBuffer(uint8_t instruction, uint8_t data)
{
    MCP2515_CS_LOW();

    SPI_Tx(instruction);
    SPI_Tx(data);

    MCP2515_CS_HIGH();
}

/* RTS ??? ??? TxBuffer ?? */
void MCP2515_RequestToSend(uint8_t instruction)
{
    MCP2515_CS_LOW();

    SPI_Tx(instruction);

    MCP2515_CS_HIGH();
}

/* MCP2515 Status ?? */
uint8_t MCP2515_ReadStatus(void)
{
    uint8_t retVal;

    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_READ_STATUS);
    retVal = SPI_Rx();

    MCP2515_CS_HIGH();

    return retVal;
}

/* MCP2515 RxStatus ???? ?? */
uint8_t MCP2515_GetRxStatus(void)
{
    uint8_t retVal;

    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_RX_STATUS);
    retVal = SPI_Rx();

    MCP2515_CS_HIGH();

    return retVal;
}

/* ???? ? ?? */
void MCP2515_BitModify(uint8_t address, uint8_t mask, uint8_t data)
{
    MCP2515_CS_LOW();

    SPI_Tx(MCP2515_BIT_MOD);
    SPI_Tx(address);
    SPI_Tx(mask);
    SPI_Tx(data);

    MCP2515_CS_HIGH();
}

/* SPI Tx Wrapper ?? */
static void SPI_Tx(uint8_t data)
{
    HAL_SPI_Transmit(SPI_CAN, &data, 1, SPI_TIMEOUT);
}

/* SPI Tx Wrapper ?? */
static void SPI_TxBuffer(uint8_t *buffer, uint8_t length)
{
    HAL_SPI_Transmit(SPI_CAN, buffer, length, SPI_TIMEOUT);
}

/* SPI Rx Wrapper ?? */
static uint8_t SPI_Rx(void)
{
    uint8_t retVal;
    HAL_SPI_Receive(SPI_CAN, &retVal, 1, SPI_TIMEOUT);
    return retVal;
}

/* SPI Rx Wrapper ?? */
static void SPI_RxBuffer(uint8_t *buffer, uint8_t length)
{
    HAL_SPI_Receive(SPI_CAN, buffer, length, SPI_TIMEOUT);
}
