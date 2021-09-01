
// Директивы для работы с битами
#define  SetBit(reg, bit)      reg |= (1<<bit)           // Установка бита в 1
#define  ClearBit(reg, bit)    reg &= (~(1<<bit))        // Установка бита в 0
#define  InvBit(reg, bit)      reg ^= (1<<bit)           // Инверсия  бита
#define  BitIsSet(reg, bit)    ((reg & (1<<bit)) != 0)   // Проверка бита на 1
#define  BitIsClear(reg, bit)  ((reg & (1<<bit)) == 0)   // Проверка бита на 0
