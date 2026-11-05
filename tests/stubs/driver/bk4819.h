#ifndef TEST_STUB_DRIVER_BK4819_H
#define TEST_STUB_DRIVER_BK4819_H
#include <stdint.h>
typedef enum {
    BK4819_REG_02 = 0x02,
    BK4819_REG_0B = 0x0B,
    BK4819_REG_3F = 0x3F,
    BK4819_REG_58 = 0x58,
    BK4819_REG_59 = 0x59,
    BK4819_REG_5A = 0x5A,
    BK4819_REG_5B = 0x5B,
    BK4819_REG_5C = 0x5C,
    BK4819_REG_5D = 0x5D,
    BK4819_REG_5E = 0x5E,
    BK4819_REG_5F = 0x5F,
    BK4819_REG_70 = 0x70,
    BK4819_REG_72 = 0x72
} BK4819_REGISTER_t;
#define BK4819_REG_02_FSK_RX_FINISHED       (1u << 13)
#define BK4819_REG_02_FSK_FIFO_ALMOST_FULL  (1u << 12)
#define BK4819_REG_02_FSK_RX_SYNC           (1u << 1)
#define BK4819_REG_3F_FSK_RX_FINISHED       (1u << 13)
#define BK4819_REG_3F_FSK_FIFO_ALMOST_FULL  (1u << 12)
#define BK4819_REG_3F_FSK_RX_SYNC           (1u << 1)
uint16_t BK4819_ReadRegister(BK4819_REGISTER_t reg);
void BK4819_WriteRegister(BK4819_REGISTER_t reg, uint16_t value);
#endif
