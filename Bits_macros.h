
// ��������� ��� ������ � ������
#define  SetBit(reg, bit)      reg |= (1<<bit)           // ��������� ���� � 1
#define  ClearBit(reg, bit)    reg &= (~(1<<bit))        // ��������� ���� � 0
#define  InvBit(reg, bit)      reg ^= (1<<bit)           // ��������  ����
#define  BitIsSet(reg, bit)    ((reg & (1<<bit)) != 0)   // �������� ���� �� 1
#define  BitIsClear(reg, bit)  ((reg & (1<<bit)) == 0)   // �������� ���� �� 0
