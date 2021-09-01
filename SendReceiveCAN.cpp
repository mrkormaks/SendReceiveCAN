#define _CRT_SECURE_NO_WARNINGS
#define hiBite(a) ((a >> 8) & 0xFF) 
#define loBite(a) ((a) & 0xFF)

#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <math.h>
#include <algorithm>

#include "AdvCan.h"
#include "AdvCANIO.h"
#include "Bits_macros.h"
#include <so2010struct.h>
#include <STRUCT_AVN_410_RTSH_4.h>
#include <Struct_SUT_410.h>

char* ini_pathT(char* T);

/* Global parameters */

HANDLE      hMapFile;
LPCSTR      shMemName = "STRUCT_AVN_410";

HANDLE		hMapFileSUT;
LPCSTR		shSUTMemName = "Struct_SUT";

HANDLE      hSendThread;
HANDLE      hReceiveThread;
DWORD       dwSendThreadID;
DWORD       dwReceiveThreadID;

HANDLE      hCANDevice_1;
HANDLE      hCANDevice_2;

TCHAR       portName_1[8];
TCHAR       portName_2[8];

canmsg_t    msgRead_1;
canmsg_t    msgRead_2;
canmsg_t    msgWrite[20];

OVERLAPPED  ovReceive_1;
OVERLAPPED  ovReceive_2;

ULONG       ulReadIndex         = 0;
ULONG       ulWriteIndex        = 0;
ULONG       ulReadCount         = 1;
ULONG       ulWriteCount        = 0;
ULONG       ulNumberOfRead      = 0;
ULONG       ulNumberOfWritten   = 0;
ULONG       ulReadTimeOut       = 5;
ULONG       ulWriteTimeOut      = 5;

ULONG       ulReadIndexPort2	= 0;
ULONG       ulWriteIndexPort2	= 0;
ULONG       ulReadCountPort2	= 1;
ULONG       ulWriteCountPort2	= 0;
ULONG       ulNumberOfReadPort2 = 0;
ULONG       ulNumberOfWrittenPort2 = 0;

//ULONG       ulReadTimeOutPort2	= 50;
ULONG       ulReadTimeOutPort2 = 5;
//ULONG       ulReadTimeOutPort2 = 150;

//ULONG       ulWriteTimeOutPort2 = 50;
ULONG       ulWriteTimeOutPort2 = 5; 
//ULONG       ulWriteTimeOutPort2 = 150;

int         data_1[8]           = {0};
int         data_2[8]           = {0};

static double   old_kompasKurs      = 0.0;
static double   old_sharik          = 0.0;
static double   old_arkFrequency    = 0.0;
static int      old_soMessage       = 0;
static int      old_soDisplayedCode = 0;

static bool     startSign           = 1;

SHAVN_410*  AVN;
SHSUT* sut;
//SHMAIN_STRUCT_CABINE* cab;

bool	close_all = false;

bool	Buttons_FK[13][3];						// кнопки ФК   МФИ ([0] = 1 ПИЛОТ, [1] = ЦЕНТР, [2] = 2 ПИЛОТ) - заложено на 1 больше, чтобы не путаться с ТЗ
bool	Buttons_MFK[29][3];					// кнопки МФК  МФИ ([0] = 1 ПИЛОТ, [1] = ЦЕНТР, [2] = 2 ПИЛОТ) - заложено на 1 больше, чтобы не путаться с ТЗ
bool	Buttons[74][2];						// кнопки  МФПУ ([0] = 1 ПИЛОТ, [1] = 2 ПИЛОТ)

//int brightness[3]			= {0,0,0};		// яркость(+1 - поворот на 1 деление по часовой, -1 поворот на одно деление против часовой). [0] - MFI1  [1] - MFI2  [3] - MFI3
bool	brightness_button[3] = { 0,0,0 };		// непонятная кнопка на регуляторе яркости (значение принимаем, но пока не отрабатываем)

//int indik_internal[3]		= {0,0,0};		// вращение внутренней кремальеры на ИНДИК (-1 против часовой/+1 по часовой) - (значение принимаем, но пока не отрабатываем)
//int indik_external[3]		= {0,0,0};		// вращение внешней кремальеры на ИНДИК (-1 против часовой/+1 по часовой) - (значение принимаем, но пока не отрабатываем)
bool	indik_button[3] = { 0,0,0 };		// непонятная кнопка на кремальере ИНДИК - (значение принимаем, но пока не отрабатываем)

double	Kontur1;

bool	Brightness_minus = 0;			// нажатие кнопки уменьшения яркости (true - нажата, false - не нажата)
bool	Brightness_plus = 0;			// нажатие кнопки увеличения яркости (true - нажата, false - не нажата)
bool	Fast_Korrection = 0;			// Нажатие кнопки ускоренной коррекции
bool	HDG_press = 0;			// Признак кратковременного нажатия кремальеры HDG;
bool	BARO_press = 0;			// Признак кратковременного нажатия кремальеры BARO;

static int	BARO_roll = 0;			/* вращение ручки BARO, стартовое значение равно 0. при вращении против часовой на 1 щелчок значение BARO должно уменьшаться на единицу,
										при вращении по часовой увеличиваться на единицу. число может быть "плавающим", его обработку осуществляю сам*/
static int	HDG_roll = 0;			/* вращение ручки HDG, стартовое значение равно 0. при вращении против часовой на 1 щелчок значение HDG должно уменьшаться на единицу,
										при вращении по часовой увеличиваться на единицу. число может быть "плавающим", его обработку осуществляю сам*/

soStruct_t	soStruct;

/****************************************/

int nRet = 0;

UCHAR lowMask				= 0x07;
UCHAR nabor_value_left		= 1;
UCHAR gromk_value_left		= 1;
UCHAR nabor_value_right		= 1;
UCHAR gromk_value_right		= 1;
UCHAR nabor_value_left_prev	= 1;
UCHAR gromk_value_left_prev = 1;
UCHAR nabor_value_right_prev = 1;
UCHAR gromk_value_right_prev = 1;
int nabor_direction			= 0;
int gromk_direction			= 0;
char kontur[10];

DWORD WINAPI ReceiveThreadMethod(LPVOID)
{
	if (startSign)
	{
		old_kompasKurs = AVN->CAN.toCan.KompasKurs;
		old_sharik = AVN->CAN.toCan.Sharik;
		old_arkFrequency = AVN->CAN.toCan.ark_Freq;
		old_soMessage = AVN->CAN.toCan.message;
		old_soDisplayedCode = AVN->CAN.toCan.displayedCode;
		//Kontur1 = GetPrivateProfileIntA("PRIB", "Kontur", 75, "CAN_Settings.ini");
		Kontur1 = 75;

		// Опрос состояния тумблеров потолочной панели (один раз при запуске программы, далее в цикле по факту нажатия)
		ZeroMemory(&msgWrite[13], sizeof(canmsg_t));
		msgWrite[13].id = 121;
		msgWrite[13].length = 1;
		msgWrite[13].data[0] = 111;

		ZeroMemory(&msgWrite[14], sizeof(canmsg_t));
		msgWrite[14].id = 122;
		msgWrite[14].length = 1;
		msgWrite[14].data[0] = 111;

		ZeroMemory(&msgWrite[15], sizeof(canmsg_t));
		msgWrite[15].id = 123;
		msgWrite[15].length = 1;
		msgWrite[15].data[0] = 111;

		ZeroMemory(&msgWrite[16], sizeof(canmsg_t));
		msgWrite[16].id = 124;
		msgWrite[16].length = 1;
		msgWrite[16].data[0] = 111;

		ZeroMemory(&msgWrite[17], sizeof(canmsg_t));
		msgWrite[17].id = 125;
		msgWrite[17].length = 1;
		msgWrite[17].data[0] = 111;

		ZeroMemory(&msgWrite[18], sizeof(canmsg_t));
		msgWrite[18].id = 126;
		msgWrite[18].length = 1;
		msgWrite[18].data[0] = 111;

		ZeroMemory(&msgWrite[19], sizeof(canmsg_t));
		msgWrite[19].id = 127;
		msgWrite[19].length = 1;
		msgWrite[19].data[0] = 111;

		for (int i = 13; i < 20; i++)
		{
			nRet = acCanWrite(hCANDevice_2, &msgWrite[i], 1, &ulNumberOfWritten, &ovReceive_2);
		}
		/////////////////////////////
		startSign = false;
	}

	HANDLE hTimer			= NULL;							// идентификатор объекта-таймера 
	LARGE_INTEGER li;
	CRITICAL_SECTION psc;									//критическая секция для перезаписи

	DWORD dwTimeout			= 500;							//ms
	li.QuadPart				= -1000000;						//1000 ms do begin    //1 сек = -10 000 000 тиков по 100 ns;

	hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	SetWaitableTimer(hTimer, &li, 20, NULL, NULL, FALSE);	//Period=20ms
//	SetWaitableTimer(hTimer, &li, 50, NULL, NULL, FALSE);	//Period=50ms
//	SetWaitableTimer(hTimer, &li, 70, NULL, NULL, FALSE);	//Period=75ms
//	SetWaitableTimer(hTimer, &li, 100, NULL, NULL, FALSE);	//Period=100ms
//  SetWaitableTimer(hTimer, &li, 150, NULL, NULL, FALSE);	//Period=150ms

	InitializeCriticalSection(&psc);

	while (!close_all)
	{
		DWORD temp = WaitForSingleObject(hTimer, dwTimeout);

		int brightness[3] = { 0,0,0 };		// яркость(+1 - поворот на 1 деление по часовой, -1 поворот на одно деление против часовой). [0] - MFI1  [1] - MFI2  [3] - MFI3
		int indik_internal[3] = { 0,0,0 };		// вращение внутренней кремальеры на ИНДИК (-1 против часовой/+1 по часовой) - (значение принимаем, но пока не отрабатываем)
		int indik_external[3] = { 0,0,0 };		// вращение внешней кремальеры на ИНДИК (-1 против часовой/+1 по часовой) - (значение принимаем, но пока не отрабатываем)

		nRet = acCanRead(hCANDevice_1, &msgRead_1, ulReadCount, &ulNumberOfRead, &ovReceive_1);
		if (nRet == SUCCESS)
		{
			//cout << "Start receiving..." << ; //("Start receiving...");

			if (msgRead_1.id == ERRORID)
			{
				//						printf("Incorrect package! ERRORID! \n");
										//return 0;
			}
			else
			{
				//if ((msgRead_1.id & MSG_RTR) > 0)
				//{
				//	printf("RTR package! \n");
				//	//return 0;
				//}
				//else
				//{
				int len = msgRead_1.length;
				if (len > 8 || len < 0)
				{
					len = 8;
				}
				for (int i = 0; i < len; i++)
				{
					data_1[i] = msgRead_1.data[i];
					//std::cout << "package -> " << data_1[0] << data_1[1] << data_1[2] << data_1[3] << data_1[4] << data_1[5] << data_1[6] << data_1[07] << std::endl;
				}

				////////////////////// HDG /////////////////////////////////////////////////////////////////////////////
				switch (msgRead_1.data[0])
				{
					//------------------- Кремальера HDG ------------------------------------------------------------
				case 30:
					//if (msgRead_1.length <= 3) {			// если приходит вращение, то length = 3, если нажатие кнопки, то length = 4
					//	switch (msgRead_1.data[1]) {
					//	case 0:
					//		HDG_roll = HDG_roll + 1; //(msgRead_1.data[2]);
					//		break;
					//	case 1:
					//		HDG_roll = HDG_roll - 1; //msgRead_1.data[2];
					//		break;
					//	}
					//}
					//else if (msgRead_1.length > 3) {
					//	//------------------- Кнопка HDG ----------------------------------------------------------------
					//	switch (msgRead_1.data[3]) {
					//	case 0:
					//		HDG_press = msgRead_1.data[3];
					//		break;
					//	case 1:
					//		HDG_press = msgRead_1.data[3];
					//		break;
					//	}
					//}

					if (msgRead_1.length <= 3) {						// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки
						switch (msgRead_1.data[1]) {
						case 0:
							indik_external[0] = 1; // msgRead_1.data[2];
							break;
						case 1:
							indik_external[0] = -1; // (msgRead_1.data[2]);
							break;
						}
					}

					break;
				}

				////////////////////// BARO ////////////////////////////////////////////////////////////////////////////
				switch (msgRead_1.data[0])
				{
					//------------------- BARO кремальера -----------------------------------------------------------
				case 29:
					if (msgRead_1.length <= 3) {
						switch (msgRead_1.data[1]) {
						case 0:
							BARO_roll = BARO_roll + 1; //msgRead_1.data[1];
							break;
						case 1:
							BARO_roll = BARO_roll - 1; //msgRead_1.data[1];
							break;
						}
					}
					else if (msgRead_1.length > 3) {
						//------------------- BARO кнопка ---------------------------------------------------------------
						switch (msgRead_1.data[3]) {
						case 0:
							BARO_press = msgRead_1.data[3];
							break;
						case 1:
							BARO_press = msgRead_1.data[3];
							break;
						}
					}
				}

				//////////////////////////////////////////////////////////////////////////////////////////////////////
				switch (msgRead_1.data[0])
				{
					//------------------- Яркость, индикация левого МФИ --------------------------------------------------
										// верхняя кремальера ИНДИК
				case 20:
					if (msgRead_1.length <= 3) {						// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки
						switch (msgRead_1.data[1]) {
						case 0:
							indik_external[0] = 1;//msgRead_1.data[2];
							break;
						case 1:
							indik_external[0] = -1;//(msgRead_1.data[2]);
							break;
						}
					}
					// кнопка ИНДИК
					else if (msgRead_1.length > 3) {
						switch (msgRead_1.data[3]) {
						case 0:
							indik_button[0] = false;
							break;
						case 1:
							indik_button[0] = true;
							break;
						}
					}
					break;

				case 21:
					switch (msgRead_1.data[1]) {
					case 0:
						indik_internal[0] = 1;//msgRead_1.data[2];
						break;
					case 1:
						indik_internal[0] = -1;//(msgRead_1.data[2]);
						break;
					}
					break;

				case 22:
					switch (msgRead_1.data[1]) {
					case 0:
						brightness[0] = 1;//msgRead_1.data[2];
						break;
					case 1:
						brightness[0] = -1;//msgRead_1.data[2];
						break;
					}
					break;
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////
				switch (msgRead_1.data[0])
				{
					//------------------- Яркость, индикация среднего МФИ --------------------------------------------------
										// верхняя кремальера ИНДИК
				case 23:
					if (msgRead_1.length <= 3) {						// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки
						switch (msgRead_1.data[1]) {
						case 0:
							indik_external[1] = 1;
							break;
						case 1:
							indik_external[1] = -1;
							break;
						}
					}
					// кнопка ИНДИК
					else if (msgRead_1.length > 3) {
						switch (msgRead_1.data[3]) {
						case 0:
							indik_button[1] = false;
							break;
						case 1:
							indik_button[1] = true;
							break;
						}
					}
					break;

					// кремальера ИНДИК нижн средн и кнопка ИСРП Яркость +
				case 24:
					if (msgRead_1.length <= 3) {							// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки ИСРП Яркость +
						switch (msgRead_1.data[1]) {
						case 0:
							indik_internal[1] = 1;
							break;
						case 1:
							indik_internal[1] = -1;
							break;
						}
					}
					else if (msgRead_1.length > 3) {
						switch (msgRead_1.data[3]) {
						case 0:
							Brightness_plus = false;
							break;
						case 1:
							Brightness_plus = true;
							break;
						}
					}
					break;

					// кремальера ЯРК средн и кнопка ИСРП FE
				case 25:
					if (msgRead_1.length <= 3) {							// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки ИСРП FE
						switch (msgRead_1.data[1]) {
						case 0:
							Kontur1 += 10;
							if (Kontur1 > 100)
							{
								Kontur1 = 100;
							}
							brightness[1] = -1;
							break;
						case 1:
							Kontur1 -= 10;
							if (Kontur1 < 0)
							{
								Kontur1 = 0;
							}
							brightness[1] = 1;
							break;
						}
					}
					else if (msgRead_1.length > 3) {
						switch (msgRead_1.data[3]) {
						case 0:
							Fast_Korrection = false;
							break;
						case 1:
							Fast_Korrection = true;
							break;
						}
					}
					break;
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////
				switch (msgRead_1.data[0])
				{
					//------------------- Яркость, индикация правого МФИ --------------------------------------------------
										// верхняя кремальера ИНДИК
				case 26:
					if (msgRead_1.length <= 3) {						// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки
						switch (msgRead_1.data[1]) {
						case 0:
							indik_external[2] = 1;
							break;
						case 1:
							indik_external[2] = -1;
							break;
						}
					}
					// кнопка ИНДИК
					else if (msgRead_1.length > 3) {
						switch (msgRead_1.data[3]) {
						case 0:
							indik_button[2] = false;
							break;
						case 1:
							indik_button[2] = true;
							break;
						}
					}
					break;

				case 27:
					switch (msgRead_1.data[1]) {
					case 0:
						indik_internal[2] = 1;
						break;
					case 1:
						indik_internal[2] = -1;
						break;
					}
					break;

					// кремальера ЯРК прав и кнопка ИСРП Яркость -
				case 28:
					if (msgRead_1.length <= 3) {							// если length = 3, то это вращение, если length = 4, то пришло нажатие кнопки ИСРП FE
						switch (msgRead_1.data[1]) {
						case 0:
							brightness[2] = 1;
							break;
						case 1:
							brightness[2] = -1;
							break;
						}
					}
					else if (msgRead_1.length > 3) {
						switch (msgRead_1.data[3]) {
						case 0:
							Brightness_minus = false;
							break;
						case 1:
							Brightness_minus = true;
							break;
						}
					}
					break;
				}

				/*************************************************************************************************************/
				switch (msgRead_1.data[0])
				{
					//---------- Приборы Б8-50КР левый и правый --------------------------------------------------------------------
				case 42:														// 42 = Б8-50КР лев. (кнопки)
					switch (msgRead_1.data[1])
					{
					case 1:
						AVN->IBKO.B8_LEFT_IN.line1treug = msgRead_1.data[1];			// нажатие кнопки Треугольник верх
						break;
					case 2:
						AVN->IBKO.B8_LEFT_IN.line2treug = msgRead_1.data[1];			// нажатие кнопки Треугольник средн
						break;
					case 3:
						AVN->IBKO.B8_LEFT_IN.line3treug = msgRead_1.data[1];			// нажатие кнопки Треугольник нижн
						break;
					case 20:
						AVN->IBKO.B8_LEFT_IN.button_psh_1 = msgRead_1.data[1];		// нажатие кнопки ПШ 1
						break;
					case 19:
						AVN->IBKO.B8_LEFT_IN.button_psh_2 = msgRead_1.data[1];		// нажатие кнопки ПШ 2
						break;
					case 18:
						AVN->IBKO.B8_LEFT_IN.button_psh_3 = msgRead_1.data[1];		// нажатие кнопки ПШ 3
						break;
					case 4:
						AVN->IBKO.B8_LEFT_IN.button_APr = msgRead_1.data[1];			// нажатие кнопки АПр
						break;
					case 6:
						AVN->IBKO.B8_LEFT_IN.button_1 = msgRead_1.data[1];			// нажатие кнопки 1
						break;
					case 9:
						AVN->IBKO.B8_LEFT_IN.button_2 = msgRead_1.data[1];			// нажатие кнопки 2
						break;
					case 12:
						AVN->IBKO.B8_LEFT_IN.button_3 = msgRead_1.data[1];			// нажатие кнопки 3
						break;
					case 15:
						AVN->IBKO.B8_LEFT_IN.button_vvod = msgRead_1.data[1];		// нажатие кнопки Ввод
						break;
					case 7:
						AVN->IBKO.B8_LEFT_IN.button_4 = msgRead_1.data[1];			// нажатие кнопки 4
						break;
					case 11:
						AVN->IBKO.B8_LEFT_IN.button_5 = msgRead_1.data[1];			// нажатие кнопки 5
						break;
					case 13:
						AVN->IBKO.B8_LEFT_IN.button_6 = msgRead_1.data[1];			// нажатие кнопки 6
						break;
					case 16:
						AVN->IBKO.B8_LEFT_IN.button_0 = msgRead_1.data[1];			// нажатие кнопки 0
						break;
					case 5:
						AVN->IBKO.B8_LEFT_IN.button_RI = msgRead_1.data[1];			// нажатие кнопки РИ
						break;
					case 8:
						AVN->IBKO.B8_LEFT_IN.button_7 = msgRead_1.data[1];			// нажатие кнопки 7
						break;
					case 10:
						AVN->IBKO.B8_LEFT_IN.button_8 = msgRead_1.data[1];			// нажатие кнопки 8
						break;
					case 14:
						AVN->IBKO.B8_LEFT_IN.button_9 = msgRead_1.data[1];			// нажатие кнопки 9
						break;
					case 17:
						AVN->IBKO.B8_LEFT_IN.button_sbros = msgRead_1.data[1];		// нажатие кнопки Сброс
						break;
					default:

						AVN->IBKO.B8_LEFT_IN.button_0 = 0;
						AVN->IBKO.B8_LEFT_IN.button_1 = 0;
						AVN->IBKO.B8_LEFT_IN.button_2 = 0;
						AVN->IBKO.B8_LEFT_IN.button_3 = 0;
						AVN->IBKO.B8_LEFT_IN.button_4 = 0;
						AVN->IBKO.B8_LEFT_IN.button_5 = 0;
						AVN->IBKO.B8_LEFT_IN.button_6 = 0;
						AVN->IBKO.B8_LEFT_IN.button_7 = 0;
						AVN->IBKO.B8_LEFT_IN.button_8 = 0;
						AVN->IBKO.B8_LEFT_IN.button_9 = 0;
						AVN->IBKO.B8_LEFT_IN.button_APr = 0;
						AVN->IBKO.B8_LEFT_IN.button_CA = 0;
						AVN->IBKO.B8_LEFT_IN.button_psh_1 = 0;
						AVN->IBKO.B8_LEFT_IN.button_psh_2 = 0;
						AVN->IBKO.B8_LEFT_IN.button_psh_3 = 0;
						AVN->IBKO.B8_LEFT_IN.button_RI = 0;
						AVN->IBKO.B8_LEFT_IN.button_sbros = 0;
						AVN->IBKO.B8_LEFT_IN.button_vvod = 0;
						AVN->IBKO.B8_LEFT_IN.line1treug = 0;
						AVN->IBKO.B8_LEFT_IN.line2treug = 0;
						AVN->IBKO.B8_LEFT_IN.line3treug = 0;

						break;
					}
					break;
				case 43:														// 43 = Б8-50КР прав. (кнопки)
					switch (msgRead_1.data[1])
					{
					case 1:
						AVN->IBKO.B8_RIGHT_IN.line1treug = msgRead_1.data[1];		// нажатие кнопки Треугольник верх
						break;
					case 2:
						AVN->IBKO.B8_RIGHT_IN.line2treug = msgRead_1.data[1];		// нажатие кнопки Треугольник средн
						break;
					case 3:
						AVN->IBKO.B8_RIGHT_IN.line3treug = msgRead_1.data[1];		// нажатие кнопки Треугольник нижн
						break;
					case 20:
						AVN->IBKO.B8_RIGHT_IN.button_psh_1 = msgRead_1.data[1];		// нажатие кнопки ПШ 1
						break;
					case 19:
						AVN->IBKO.B8_RIGHT_IN.button_psh_2 = msgRead_1.data[1];		// нажатие кнопки ПШ 2
						break;
					case 18:
						AVN->IBKO.B8_RIGHT_IN.button_psh_3 = msgRead_1.data[1];		// нажатие кнопки ПШ 3
						break;
					case 4:
						AVN->IBKO.B8_RIGHT_IN.button_APr = msgRead_1.data[1];		// нажатие кнопки АПр
						break;
					case 6:
						AVN->IBKO.B8_RIGHT_IN.button_1 = msgRead_1.data[1];			// нажатие кнопки 1
						break;
					case 9:
						AVN->IBKO.B8_RIGHT_IN.button_2 = msgRead_1.data[1];			// нажатие кнопки 2
						break;
					case 12:
						AVN->IBKO.B8_RIGHT_IN.button_3 = msgRead_1.data[1];			// нажатие кнопки 3
						break;
					case 15:
						AVN->IBKO.B8_RIGHT_IN.button_vvod = msgRead_1.data[1];		// нажатие кнопки Ввод
						break;
					case 7:
						AVN->IBKO.B8_RIGHT_IN.button_4 = msgRead_1.data[1];			// нажатие кнопки 4
						break;
					case 10:
						AVN->IBKO.B8_RIGHT_IN.button_5 = msgRead_1.data[1];			// нажатие кнопки 5
						break;
					case 13:
						AVN->IBKO.B8_RIGHT_IN.button_6 = msgRead_1.data[1];			// нажатие кнопки 6
						break;
					case 16:
						AVN->IBKO.B8_RIGHT_IN.button_0 = msgRead_1.data[1];			// нажатие кнопки 0
						break;
					case 5:
						AVN->IBKO.B8_RIGHT_IN.button_RI = msgRead_1.data[1];			// нажатие кнопки РИ
						break;
					case 8:
						AVN->IBKO.B8_RIGHT_IN.button_7 = msgRead_1.data[1];			// нажатие кнопки 7
						break;
					case 11:
						AVN->IBKO.B8_RIGHT_IN.button_8 = msgRead_1.data[1];			// нажатие кнопки 8
						break;
					case 14:
						AVN->IBKO.B8_RIGHT_IN.button_9 = msgRead_1.data[1];			// нажатие кнопки 9
						break;
					case 17:
						AVN->IBKO.B8_RIGHT_IN.button_sbros = msgRead_1.data[1];		// нажатие кнопки Сброс
						break;
					default:

						AVN->IBKO.B8_RIGHT_IN.button_0 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_1 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_2 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_3 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_4 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_5 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_6 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_7 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_8 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_9 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_APr = 0;
						AVN->IBKO.B8_RIGHT_IN.button_CA = 0;
						AVN->IBKO.B8_RIGHT_IN.button_psh_1 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_psh_2 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_psh_3 = 0;
						AVN->IBKO.B8_RIGHT_IN.button_RI = 0;
						AVN->IBKO.B8_RIGHT_IN.button_sbros = 0;
						AVN->IBKO.B8_RIGHT_IN.button_vvod = 0;
						AVN->IBKO.B8_RIGHT_IN.line1treug = 0;
						AVN->IBKO.B8_RIGHT_IN.line2treug = 0;
						AVN->IBKO.B8_RIGHT_IN.line3treug = 0;

						break;
					}
					break;
				default:
					break;
				}

				switch (msgRead_1.data[0])
				{
					//---------- Галетники левого Б8-50КР
				case 82:
					for (int i = 3; i < 8; i++)													// проверка положения нижнего галетника Набор
					{
						if (BitIsSet(msgRead_1.data[1], i))
						{
							AVN->IBKO.B8_LEFT_IN.nabor = i - 3;
							//printf("B8_L.nabor %i \n", B8_L.nabor);
						}

						if (BitIsSet(msgRead_1.data[2], i))										// проверка положения нижнего галетника Громк (без РНУ 3)
						{
							AVN->IBKO.B8_LEFT_IN.gromk = i - 3;
							//printf("B8_L.gromk %i \n", B8_L.gromk);
						}
					}

					if (msgRead_1.data[3])														// проверка положения нижнего галетника Громк РНУ 3
					{
						AVN->IBKO.B8_LEFT_IN.gromk = 5;
						//printf("B8_L.gromk %i \n", B8_L.gromk);
					}

					//---------- анализ направления вращения верхнего галетника Набор

					nabor_value_left = msgRead_1.data[1];
					nabor_value_left &= lowMask;

					if (nabor_value_left_prev == 4 && nabor_value_left == 1)
					{
						AVN->IBKO.B8_LEFT_IN.nabor_roll = AVN->IBKO.B8_LEFT_IN.nabor_roll + 1;
					}
					else if (nabor_value_left_prev == 1 && nabor_value_left == 4)
					{
						AVN->IBKO.B8_LEFT_IN.nabor_roll = AVN->IBKO.B8_LEFT_IN.nabor_roll - 1;
					}
					else if (nabor_value_left_prev < nabor_value_left)
					{
						AVN->IBKO.B8_LEFT_IN.nabor_roll = AVN->IBKO.B8_LEFT_IN.nabor_roll + 1;
					}
					else if (nabor_value_left_prev > nabor_value_left)
					{
						AVN->IBKO.B8_LEFT_IN.nabor_roll = AVN->IBKO.B8_LEFT_IN.nabor_roll - 1;
					}
					else if (nabor_value_left_prev = nabor_value_left)
					{
						AVN->IBKO.B8_LEFT_IN.nabor_roll = AVN->IBKO.B8_LEFT_IN.nabor_roll;
					}

					nabor_value_left_prev = nabor_value_left;

					//---------- анализ направления вращения верхнего галетника Громк

					gromk_value_left = msgRead_1.data[2];
					gromk_value_left &= lowMask;

					if (gromk_value_left_prev == 4 && gromk_value_left == 1)
					{
						AVN->IBKO.B8_LEFT_IN.gromk_roll = AVN->IBKO.B8_LEFT_IN.gromk_roll + 1;
					}
					else if (gromk_value_left_prev == 1 && gromk_value_left == 4)
					{
						AVN->IBKO.B8_LEFT_IN.gromk_roll = AVN->IBKO.B8_LEFT_IN.gromk_roll - 1;
					}
					else if (gromk_value_left_prev < gromk_value_left)
					{
						AVN->IBKO.B8_LEFT_IN.gromk_roll = AVN->IBKO.B8_LEFT_IN.gromk_roll + 1;
					}
					else if (gromk_value_left_prev > gromk_value_left)
					{
						AVN->IBKO.B8_LEFT_IN.gromk_roll = AVN->IBKO.B8_LEFT_IN.gromk_roll - 1;
					}
					else if (gromk_value_left_prev = gromk_value_left)
					{
						AVN->IBKO.B8_LEFT_IN.gromk_roll = AVN->IBKO.B8_LEFT_IN.gromk_roll;
					}

					gromk_value_left_prev = gromk_value_left;

					//printf("roll %i \n", B8_L.nabor_roll);

					break;
					//---------- Галетники правого Б8-50КР
				case 81:
					for (int i = 3; i < 8; i++)											// проверка положения нижнего галетника Набор
					{
						if (BitIsSet(msgRead_1.data[1], i))
						{
							AVN->IBKO.B8_RIGHT_IN.nabor = i - 3;
						}

						if (BitIsSet(msgRead_1.data[2], i))								// проверка положения нижнего галетника Громк (без РНУ 3)
						{
							AVN->IBKO.B8_RIGHT_IN.gromk = i - 3;
						}
					}

					if (msgRead_1.data[3])												// проверка положения нижнего галетника Громк РНУ 3
					{
						AVN->IBKO.B8_RIGHT_IN.gromk = 5;
					}

					//---------- анализ направления вращения верхнего галетника Набор

					nabor_value_right = msgRead_1.data[1];
					nabor_value_right &= lowMask;

					if (nabor_value_right_prev == 4 && nabor_value_right == 1)
					{
						AVN->IBKO.B8_RIGHT_IN.nabor_roll = AVN->IBKO.B8_RIGHT_IN.nabor_roll + 1;
					}
					else if (nabor_value_right_prev == 1 && nabor_value_right == 4)
					{
						AVN->IBKO.B8_RIGHT_IN.nabor_roll = AVN->IBKO.B8_RIGHT_IN.nabor_roll - 1;
					}
					else if (nabor_value_right_prev < nabor_value_right)
					{
						AVN->IBKO.B8_RIGHT_IN.nabor_roll = AVN->IBKO.B8_RIGHT_IN.nabor_roll + 1;
					}
					else if (nabor_value_right_prev > nabor_value_right)
					{
						AVN->IBKO.B8_RIGHT_IN.nabor_roll = AVN->IBKO.B8_RIGHT_IN.nabor_roll - 1;
					}
					else
					{
						AVN->IBKO.B8_RIGHT_IN.nabor_roll = AVN->IBKO.B8_RIGHT_IN.nabor_roll;
					}

					nabor_value_right_prev = nabor_value_right;

					//---------- анализ направления вращения верхнего галетника Громк

					gromk_value_right = msgRead_1.data[2];
					gromk_value_right &= lowMask;

					if (gromk_value_right_prev == 4 && gromk_value_right == 1)
					{
						AVN->IBKO.B8_RIGHT_IN.gromk_roll = AVN->IBKO.B8_RIGHT_IN.gromk_roll + 1;
					}
					else if (gromk_value_right_prev == 1 && gromk_value_right == 4)
					{
						AVN->IBKO.B8_RIGHT_IN.gromk_roll = AVN->IBKO.B8_RIGHT_IN.gromk_roll - 1;
					}
					else if (gromk_value_right_prev < gromk_value_right)
					{
						AVN->IBKO.B8_RIGHT_IN.gromk_roll = AVN->IBKO.B8_RIGHT_IN.gromk_roll + 1;
					}
					else if (gromk_value_right_prev > gromk_value_right)
					{
						AVN->IBKO.B8_RIGHT_IN.gromk_roll = AVN->IBKO.B8_RIGHT_IN.gromk_roll - 1;
					}
					else
					{
						AVN->IBKO.B8_RIGHT_IN.gromk_roll = AVN->IBKO.B8_RIGHT_IN.gromk_roll;
					}

					gromk_value_right_prev = gromk_value_right;

					break;

					/*default:

						B8_L.nabor_roll = 0;
						B8_R.nabor_roll = 0;
						B8_L.gromk_roll = 0;
						B8_R.gromk_roll = 0;

						break;*/
				}

				//---------- Приборы МФПУ левый и правый ---------------------------------------------------
				switch (msgRead_1.data[0])											// МФПУ левый
				{
				case 66:
					switch (msgRead_1.data[1])
					{
					case 1:
						Buttons[0][0] = msgRead_1.data[1];
						//Power_MFPU1 = msgRead_1.data[1];
						break;
					case 2:
						Buttons[1][0] = msgRead_1.data[1];
						break;
					case 3:
						Buttons[2][0] = msgRead_1.data[1];
						break;
					case 4:
						Buttons[3][0] = msgRead_1.data[1];
						break;
					case 5:
						Buttons[4][0] = msgRead_1.data[1];
						break;
					case 6:
						Buttons[5][0] = msgRead_1.data[1];
						break;
					case 7:
						Buttons[20][0] = msgRead_1.data[1];
						break;
					case 8:
						Buttons[12][0] = msgRead_1.data[1];
						break;
					case 9:
						Buttons[28][0] = msgRead_1.data[1];
						break;
					case 10:
						Buttons[38][0] = msgRead_1.data[1];
						break;
					case 11:
						Buttons[48][0] = msgRead_1.data[1];
						break;
					case 12:
						Buttons[58][0] = msgRead_1.data[1];
						break;
					case 13:
						Buttons[68][0] = msgRead_1.data[1];
						break;
					case 14:
						Buttons[59][0] = msgRead_1.data[1];
						break;
					case 15:
						Buttons[49][0] = msgRead_1.data[1];
						break;
					case 16:
						Buttons[39][0] = msgRead_1.data[1];
						break;
					case 17:
						Buttons[69][0] = msgRead_1.data[1];
						break;
					case 18:
						Buttons[60][0] = msgRead_1.data[1];
						break;
					case 19:
						Buttons[50][0] = msgRead_1.data[1];
						break;
					case 20:
						Buttons[40][0] = msgRead_1.data[1];
						break;
					case 21:
						Buttons[29][0] = msgRead_1.data[1];
						break;
					case 22:
						Buttons[13][0] = msgRead_1.data[1];
						break;
					case 23:
						Buttons[21][0] = msgRead_1.data[1];
						break;
					case 24:
						Buttons[30][0] = msgRead_1.data[1];
						break;
					case 25:
						Buttons[70][0] = msgRead_1.data[1];
						break;
					case 26:
						Buttons[61][0] = msgRead_1.data[1];
						break;
					case 27:
						Buttons[51][0] = msgRead_1.data[1];
						break;
					case 28:
						Buttons[41][0] = msgRead_1.data[1];
						break;
					case 29:
						Buttons[71][0] = msgRead_1.data[1];
						break;
					case 30:
						Buttons[62][0] = msgRead_1.data[1];
						break;
					case 31:
						Buttons[52][0] = msgRead_1.data[1];
						break;
					case 32:
						Buttons[42][0] = msgRead_1.data[1];
						break;
					case 33:
						Buttons[14][0] = msgRead_1.data[1];
						break;
					case 34:
						Buttons[22][0] = msgRead_1.data[1];
						break;
					case 35:
						Buttons[31][0] = msgRead_1.data[1];
						break;
					case 36:
						Buttons[32][0] = msgRead_1.data[1];
						break;
					case 37:
						Buttons[23][0] = msgRead_1.data[1];
						break;
					case 38:
						Buttons[15][0] = msgRead_1.data[1];
						break;
					case 39:
						Buttons[33][0] = msgRead_1.data[1];
						break;
					case 40:
						Buttons[24][0] = msgRead_1.data[1];
						break;
					case 41:
						Buttons[16][0] = msgRead_1.data[1];
						break;
					case 42:
						Buttons[67][0] = msgRead_1.data[1];
						break;
					case 43:
						Buttons[57][0] = msgRead_1.data[1];
						break;
					case 44:
						Buttons[17][0] = msgRead_1.data[1];
						break;
					case 45:
						Buttons[36][0] = msgRead_1.data[1];
						break;
					case 46:
						Buttons[18][0] = msgRead_1.data[1];
						break;
					case 47:
						Buttons[37][0] = msgRead_1.data[1];
						break;
					case 48:
						Buttons[46][0] = msgRead_1.data[1];
						break;
					case 49:
						Buttons[47][0] = msgRead_1.data[1];
						break;
					case 50:
						Buttons[26][0] = msgRead_1.data[1];
						break;
					case 51:
						Buttons[9][0] = msgRead_1.data[1];
						break;
					case 52:
						Buttons[10][0] = msgRead_1.data[1];
						break;
					case 53:
						Buttons[11][0] = msgRead_1.data[1];
						break;
					case 54:
						Buttons[6][0] = msgRead_1.data[1];
						break;
					case 55:
						Buttons[7][0] = msgRead_1.data[1];
						break;
					case 56:
						Buttons[8][0] = msgRead_1.data[1];
						break;

					default:
						Buttons[0][0] = 0;
						Buttons[1][0] = 0;
						Buttons[2][0] = 0;
						Buttons[3][0] = 0;
						Buttons[4][0] = 0;
						Buttons[5][0] = 0;
						Buttons[20][0] = 0;
						Buttons[12][0] = 0;
						Buttons[28][0] = 0;
						Buttons[38][0] = 0;
						Buttons[48][0] = 0;
						Buttons[58][0] = 0;
						Buttons[68][0] = 0;
						Buttons[59][0] = 0;
						Buttons[49][0] = 0;
						Buttons[39][0] = 0;
						Buttons[69][0] = 0;
						Buttons[60][0] = 0;
						Buttons[50][0] = 0;
						Buttons[40][0] = 0;
						Buttons[29][0] = 0;
						Buttons[13][0] = 0;
						Buttons[21][0] = 0;
						Buttons[30][0] = 0;
						Buttons[70][0] = 0;
						Buttons[61][0] = 0;
						Buttons[51][0] = 0;
						Buttons[41][0] = 0;
						Buttons[71][0] = 0;
						Buttons[62][0] = 0;
						Buttons[52][0] = 0;
						Buttons[42][0] = 0;
						Buttons[14][0] = 0;
						Buttons[22][0] = 0;
						Buttons[31][0] = 0;
						Buttons[32][0] = 0;
						Buttons[23][0] = 0;
						Buttons[15][0] = 0;
						Buttons[33][0] = 0;
						Buttons[24][0] = 0;
						Buttons[16][0] = 0;
						Buttons[67][0] = 0;
						Buttons[57][0] = 0;
						Buttons[17][0] = 0;
						Buttons[36][0] = 0;
						Buttons[18][0] = 0;
						Buttons[37][0] = 0;
						Buttons[46][0] = 0;
						Buttons[47][0] = 0;
						Buttons[26][0] = 0;
						Buttons[9][0] = 0;
						Buttons[10][0] = 0;
						Buttons[11][0] = 0;
						Buttons[6][0] = 0;
						Buttons[7][0] = 0;
						Buttons[8][0] = 0;
						break;
					}
					break;

				case 67:
					switch (msgRead_1.data[1])
					{
					case 1:
						Buttons[72][0] = msgRead_1.data[1];
						break;
					case 2:
						Buttons[63][0] = msgRead_1.data[1];
						break;
					case 3:
						Buttons[53][0] = msgRead_1.data[1];
						break;
					case 4:
						Buttons[43][0] = msgRead_1.data[1];
						break;
					case 5:
						Buttons[64][0] = msgRead_1.data[1];
						break;
					case 6:
						Buttons[54][0] = msgRead_1.data[1];
						break;
					case 7:
						Buttons[44][0] = msgRead_1.data[1];
						break;
					case 8:
						Buttons[34][0] = msgRead_1.data[1];
						break;
					case 9:
						Buttons[65][0] = msgRead_1.data[1];
						break;
					case 10:
						Buttons[55][0] = msgRead_1.data[1];
						break;
					case 11:
						Buttons[45][0] = msgRead_1.data[1];
						break;
					case 12:
						Buttons[35][0] = msgRead_1.data[1];
						break;
					case 13:
						Buttons[73][0] = msgRead_1.data[1];
						break;
					case 14:
						Buttons[66][0] = msgRead_1.data[1];
						break;
					case 15:
						Buttons[56][0] = msgRead_1.data[1];
						break;
					case 16:
						Buttons[25][0] = msgRead_1.data[1];
						break;
					case 17:
						Buttons[19][0] = msgRead_1.data[1];
						break;
					case 18:
						Buttons[27][0] = msgRead_1.data[1];
						break;

					default:
						Buttons[72][0] = 0;
						Buttons[63][0] = 0;
						Buttons[53][0] = 0;
						Buttons[43][0] = 0;
						Buttons[64][0] = 0;
						Buttons[54][0] = 0;
						Buttons[44][0] = 0;
						Buttons[34][0] = 0;
						Buttons[65][0] = 0;
						Buttons[55][0] = 0;
						Buttons[45][0] = 0;
						Buttons[35][0] = 0;
						Buttons[73][0] = 0;
						Buttons[66][0] = 0;
						Buttons[56][0] = 0;
						Buttons[25][0] = 0;
						Buttons[19][0] = 0;
						Buttons[27][0] = 0;
						break;

					}
					break;
				default:
					break;

				}
				/***************************************/
				switch (msgRead_1.data[0])											// МФПУ правый
				{
				case 64:
					switch (msgRead_1.data[1])
					{
					case 1:
						Buttons[0][1] = msgRead_1.data[1];
						break;
					case 2:
						Buttons[1][1] = msgRead_1.data[1];
						break;
					case 3:
						Buttons[2][1] = msgRead_1.data[1];
						break;
					case 4:
						Buttons[3][1] = msgRead_1.data[1];
						break;
					case 5:
						Buttons[4][1] = msgRead_1.data[1];
						break;
					case 6:
						Buttons[5][1] = msgRead_1.data[1];
						break;
					case 7:
						Buttons[20][1] = msgRead_1.data[1];
						break;
					case 8:
						Buttons[12][1] = msgRead_1.data[1];
						break;
					case 9:
						Buttons[28][1] = msgRead_1.data[1];
						break;
					case 10:
						Buttons[38][1] = msgRead_1.data[1];
						break;
					case 11:
						Buttons[48][1] = msgRead_1.data[1];
						break;
					case 12:
						Buttons[58][1] = msgRead_1.data[1];
						break;
					case 13:
						Buttons[68][1] = msgRead_1.data[1];
						break;
					case 14:
						Buttons[59][1] = msgRead_1.data[1];
						break;
					case 15:
						Buttons[49][1] = msgRead_1.data[1];
						break;
					case 16:
						Buttons[39][1] = msgRead_1.data[1];
						break;
					case 17:
						Buttons[69][1] = msgRead_1.data[1];
						break;
					case 18:
						Buttons[60][1] = msgRead_1.data[1];
						break;
					case 19:
						Buttons[50][1] = msgRead_1.data[1];
						break;
					case 20:
						Buttons[40][1] = msgRead_1.data[1];
						break;
					case 21:
						Buttons[29][1] = msgRead_1.data[1];
						break;
					case 22:
						Buttons[13][1] = msgRead_1.data[1];
						break;
					case 23:
						Buttons[21][1] = msgRead_1.data[1];
						break;
					case 24:
						Buttons[30][1] = msgRead_1.data[1];
						break;
					case 25:
						Buttons[70][1] = msgRead_1.data[1];
						break;
					case 26:
						Buttons[61][1] = msgRead_1.data[1];
						break;
					case 27:
						Buttons[51][1] = msgRead_1.data[1];
						break;
					case 28:
						Buttons[41][1] = msgRead_1.data[1];
						break;
					case 29:
						Buttons[71][1] = msgRead_1.data[1];
						break;
					case 30:
						Buttons[62][1] = msgRead_1.data[1];
						break;
					case 31:
						Buttons[52][1] = msgRead_1.data[1];
						break;
					case 32:
						Buttons[42][1] = msgRead_1.data[1];
						break;
					case 33:
						Buttons[14][1] = msgRead_1.data[1];
						break;
					case 34:
						Buttons[22][1] = msgRead_1.data[1];
						break;
					case 35:
						Buttons[31][1] = msgRead_1.data[1];
						break;
					case 36:
						Buttons[32][1] = msgRead_1.data[1];
						break;
					case 37:
						Buttons[23][1] = msgRead_1.data[1];
						break;
					case 38:
						Buttons[15][1] = msgRead_1.data[1];
						break;
					case 39:
						Buttons[33][1] = msgRead_1.data[1];
						break;
					case 40:
						Buttons[24][1] = msgRead_1.data[1];
						break;
					case 41:
						Buttons[16][1] = msgRead_1.data[1];
						break;
					case 42:
						Buttons[67][1] = msgRead_1.data[1];
						break;
					case 43:
						Buttons[57][1] = msgRead_1.data[1];
						break;
					case 44:
						Buttons[17][1] = msgRead_1.data[1];
						break;
					case 45:
						Buttons[36][1] = msgRead_1.data[1];
						break;
					case 46:
						Buttons[18][1] = msgRead_1.data[1];
						break;
					case 47:
						Buttons[37][1] = msgRead_1.data[1];
						break;
					case 48:
						Buttons[46][1] = msgRead_1.data[1];
						break;
					case 49:
						Buttons[47][1] = msgRead_1.data[1];
						break;
					case 50:
						Buttons[26][1] = msgRead_1.data[1];
						break;
					case 51:
						Buttons[9][1] = msgRead_1.data[1];
						break;
					case 52:
						Buttons[10][1] = msgRead_1.data[1];
						break;
					case 53:
						Buttons[11][1] = msgRead_1.data[1];
						break;
					case 54:
						Buttons[6][1] = msgRead_1.data[1];
						break;
					case 55:
						Buttons[7][1] = msgRead_1.data[1];
						break;
					case 56:
						Buttons[8][1] = msgRead_1.data[1];
						break;

					default:
						Buttons[0][1] = 0;
						Buttons[1][1] = 0;
						Buttons[2][1] = 0;
						Buttons[3][1] = 0;
						Buttons[4][1] = 0;
						Buttons[5][1] = 0;
						Buttons[20][1] = 0;
						Buttons[12][1] = 0;
						Buttons[28][1] = 0;
						Buttons[38][1] = 0;
						Buttons[48][1] = 0;
						Buttons[58][1] = 0;
						Buttons[68][1] = 0;
						Buttons[59][1] = 0;
						Buttons[49][1] = 0;
						Buttons[39][1] = 0;
						Buttons[69][1] = 0;
						Buttons[60][1] = 0;
						Buttons[50][1] = 0;
						Buttons[40][1] = 0;
						Buttons[29][1] = 0;
						Buttons[13][1] = 0;
						Buttons[21][1] = 0;
						Buttons[30][1] = 0;
						Buttons[70][1] = 0;
						Buttons[61][1] = 0;
						Buttons[51][1] = 0;
						Buttons[41][1] = 0;
						Buttons[71][1] = 0;
						Buttons[62][1] = 0;
						Buttons[52][1] = 0;
						Buttons[42][1] = 0;
						Buttons[14][1] = 0;
						Buttons[22][1] = 0;
						Buttons[31][1] = 0;
						Buttons[32][1] = 0;
						Buttons[23][1] = 0;
						Buttons[15][1] = 0;
						Buttons[33][1] = 0;
						Buttons[24][1] = 0;
						Buttons[16][1] = 0;
						Buttons[67][1] = 0;
						Buttons[57][1] = 0;
						Buttons[17][1] = 0;
						Buttons[36][1] = 0;
						Buttons[18][1] = 0;
						Buttons[37][1] = 0;
						Buttons[46][1] = 0;
						Buttons[47][1] = 0;
						Buttons[26][1] = 0;
						Buttons[9][1] = 0;
						Buttons[10][1] = 0;
						Buttons[11][1] = 0;
						Buttons[6][1] = 0;
						Buttons[7][1] = 0;
						Buttons[8][1] = 0;
						break;
					}
					break;

				case 65:
					switch (msgRead_1.data[1])
					{
					case 1:
						Buttons[72][1] = msgRead_1.data[1];
						break;
					case 2:
						Buttons[63][1] = msgRead_1.data[1];
						break;
					case 3:
						Buttons[53][1] = msgRead_1.data[1];
						break;
					case 4:
						Buttons[43][1] = msgRead_1.data[1];
						break;
					case 5:
						Buttons[64][1] = msgRead_1.data[1];
						break;
					case 6:
						Buttons[54][1] = msgRead_1.data[1];
						break;
					case 7:
						Buttons[44][1] = msgRead_1.data[1];
						break;
					case 8:
						Buttons[34][1] = msgRead_1.data[1];
						break;
					case 9:
						Buttons[65][1] = msgRead_1.data[1];
						break;
					case 10:
						Buttons[55][1] = msgRead_1.data[1];
						break;
					case 11:
						Buttons[45][1] = msgRead_1.data[1];
						break;
					case 12:
						Buttons[35][1] = msgRead_1.data[1];
						break;
					case 13:
						Buttons[73][1] = msgRead_1.data[1];
						break;
					case 14:
						Buttons[66][1] = msgRead_1.data[1];
						break;
					case 15:
						Buttons[56][1] = msgRead_1.data[1];
						break;
					case 16:
						Buttons[25][1] = msgRead_1.data[1];
						break;
					case 17:
						Buttons[19][1] = msgRead_1.data[1];
						break;
					case 18:
						Buttons[27][1] = msgRead_1.data[1];
						break;

					default:
						Buttons[72][1] = 0;
						Buttons[63][1] = 0;
						Buttons[53][1] = 0;
						Buttons[43][1] = 0;
						Buttons[64][1] = 0;
						Buttons[54][1] = 0;
						Buttons[44][1] = 0;
						Buttons[34][1] = 0;
						Buttons[65][1] = 0;
						Buttons[55][1] = 0;
						Buttons[45][1] = 0;
						Buttons[35][1] = 0;
						Buttons[73][1] = 0;
						Buttons[66][1] = 0;
						Buttons[56][1] = 0;
						Buttons[25][1] = 0;
						Buttons[19][1] = 0;
						Buttons[27][1] = 0;
						break;

					}
					break;
					/* default:
						break; */

				}

				//---------- ввод кнопок МФИ
				switch (msgRead_1.data[0])
				{
					//----- Левый МФИ
				case 63:
					switch (msgRead_1.data[1])
					{
						//---------- кнопки МФК (нижние)
					case 1:
						Buttons_MFK[20][0] = msgRead_1.data[1];
						break;
					case 2:
						Buttons_MFK[19][0] = msgRead_1.data[1];
						break;
					case 3:
						Buttons_MFK[18][0] = msgRead_1.data[1];
						break;
					case 4:
						Buttons_MFK[17][0] = msgRead_1.data[1];
						break;
					case 5:
						Buttons_MFK[16][0] = msgRead_1.data[1];
						break;
					case 6:
						Buttons_MFK[15][0] = msgRead_1.data[1];
						break;
					case 7:
						Buttons_MFK[14][0] = msgRead_1.data[1];
						break;
					case 8:
						Buttons_MFK[13][0] = msgRead_1.data[1];
						break;
					case 9:
						Buttons_MFK[12][0] = msgRead_1.data[1];
						break;
					case 10:
						Buttons_MFK[11][0] = msgRead_1.data[1];
						break;
					case 11:
						Buttons_MFK[10][0] = msgRead_1.data[1];
						break;
					case 12:
						Buttons_MFK[9][0] = msgRead_1.data[1];
						break;

						//---------- кнопки ФК (верхние 12 штук)
					case 13:
						Buttons_FK[12][0] = msgRead_1.data[1];
						break;
					case 14:
						Buttons_FK[11][0] = msgRead_1.data[1];
						break;
					case 15:
						Buttons_FK[10][0] = msgRead_1.data[1];
						break;
					case 16:
						Buttons_FK[9][0] = msgRead_1.data[1];
						break;
					case 17:
						Buttons_FK[8][0] = msgRead_1.data[1];
						break;
					case 18:
						Buttons_FK[7][0] = msgRead_1.data[1];
						break;
					case 19:
						Buttons_FK[6][0] = msgRead_1.data[1];
						break;
					case 20:
						Buttons_FK[5][0] = msgRead_1.data[1];
						break;
					case 21:
						Buttons_FK[4][0] = msgRead_1.data[1];
						break;
					case 22:
						Buttons_FK[3][0] = msgRead_1.data[1];
						break;
					case 23:
						Buttons_FK[2][0] = msgRead_1.data[1];
						break;
					case 24:
						Buttons_FK[1][0] = msgRead_1.data[1];
						break;

						//---------- кнопки МФК (боковые)
					case 33:
						Buttons_MFK[21][0] = msgRead_1.data[1];
						break;
					case 34:
						Buttons_MFK[22][0] = msgRead_1.data[1];
						break;
					case 35:
						Buttons_MFK[23][0] = msgRead_1.data[1];
						break;
					case 36:
						Buttons_MFK[24][0] = msgRead_1.data[1];
						break;
					case 37:
						Buttons_MFK[25][0] = msgRead_1.data[1];
						break;
					case 38:
						Buttons_MFK[26][0] = msgRead_1.data[1];
						break;
					case 39:
						Buttons_MFK[27][0] = msgRead_1.data[1];
						break;
					case 40:
						Buttons_MFK[28][0] = msgRead_1.data[1];
						break;
					case 42:
						Buttons_MFK[7][0] = msgRead_1.data[1];
						break;
					case 43:
						Buttons_MFK[8][0] = msgRead_1.data[1];
						break;
					case 45:
						Buttons_MFK[4][0] = msgRead_1.data[1];
						break;
					case 46:
						Buttons_MFK[5][0] = msgRead_1.data[1];
						break;
					case 47:
						Buttons_MFK[6][0] = msgRead_1.data[1];
						break;
					case 48:
						Buttons_MFK[1][0] = msgRead_1.data[1];
						break;
					case 49:
						Buttons_MFK[2][0] = msgRead_1.data[1];
						break;
					case 50:
						Buttons_MFK[3][0] = msgRead_1.data[1];
						break;
					default:
						for (int i = 1; i < 13; i++)
						{
							Buttons_FK[i][0] = 0;
						}
						for (int j = 1; j < 29; j++)
						{
							Buttons_MFK[j][0] = 0;
						}
						break;
					}
					break;
					//----- Средний МФИ
				case 61:
					switch (msgRead_1.data[1])
					{
						//---------- кнопки МФК (нижние)
					case 1:
						Buttons_MFK[20][1] = msgRead_1.data[1];
						break;
					case 2:
						Buttons_MFK[19][1] = msgRead_1.data[1];
						break;
					case 3:
						Buttons_MFK[18][1] = msgRead_1.data[1];
						break;
					case 4:
						Buttons_MFK[17][1] = msgRead_1.data[1];
						break;
					case 5:
						Buttons_MFK[16][1] = msgRead_1.data[1];
						break;
					case 6:
						Buttons_MFK[15][1] = msgRead_1.data[1];
						break;
					case 7:
						Buttons_MFK[14][1] = msgRead_1.data[1];
						break;
					case 8:
						Buttons_MFK[13][1] = msgRead_1.data[1];
						break;
					case 9:
						Buttons_MFK[12][1] = msgRead_1.data[1];
						break;
					case 10:
						Buttons_MFK[11][1] = msgRead_1.data[1];
						break;
					case 11:
						Buttons_MFK[10][1] = msgRead_1.data[1];
						break;
					case 12:
						Buttons_MFK[9][1] = msgRead_1.data[1];
						break;

						//---------- кнопки ФК (верхние 12 штук)
					case 13:
						Buttons_FK[12][1] = msgRead_1.data[1];
						break;
					case 14:
						Buttons_FK[11][1] = msgRead_1.data[1];
						break;
					case 15:
						Buttons_FK[10][1] = msgRead_1.data[1];
						break;
					case 16:
						Buttons_FK[9][1] = msgRead_1.data[1];
						break;
					case 17:
						Buttons_FK[8][1] = msgRead_1.data[1];
						break;
					case 18:
						Buttons_FK[7][1] = msgRead_1.data[1];
						break;
					case 19:
						Buttons_FK[6][1] = msgRead_1.data[1];
						break;
					case 20:
						Buttons_FK[5][1] = msgRead_1.data[1];
						break;
					case 21:
						Buttons_FK[4][1] = msgRead_1.data[1];
						break;
					case 22:
						Buttons_FK[3][1] = msgRead_1.data[1];
						break;
					case 23:
						Buttons_FK[2][1] = msgRead_1.data[1];
						break;
					case 24:
						Buttons_FK[1][1] = msgRead_1.data[1];
						break;

						//---------- кнопки МФК (боковые)
					case 33:
						Buttons_MFK[21][1] = msgRead_1.data[1];
						break;
					case 34:
						Buttons_MFK[22][1] = msgRead_1.data[1];
						break;
					case 35:
						Buttons_MFK[23][1] = msgRead_1.data[1];
						break;
					case 36:
						Buttons_MFK[24][1] = msgRead_1.data[1];
						break;
					case 37:
						Buttons_MFK[25][1] = msgRead_1.data[1];
						break;
					case 38:
						Buttons_MFK[26][1] = msgRead_1.data[1];
						break;
					case 39:
						Buttons_MFK[27][1] = msgRead_1.data[1];
						break;
					case 40:
						Buttons_MFK[28][1] = msgRead_1.data[1];
						break;
					case 42:
						Buttons_MFK[7][1] = msgRead_1.data[1];
						break;
					case 43:
						Buttons_MFK[8][1] = msgRead_1.data[1];
						break;
					case 45:
						Buttons_MFK[4][1] = msgRead_1.data[1];
						break;
					case 46:
						Buttons_MFK[5][1] = msgRead_1.data[1];
						break;
					case 47:
						Buttons_MFK[6][1] = msgRead_1.data[1];
						break;
					case 48:
						Buttons_MFK[1][1] = msgRead_1.data[1];
						break;
					case 49:
						Buttons_MFK[2][1] = msgRead_1.data[1];
						break;
					case 50:
						Buttons_MFK[3][1] = msgRead_1.data[1];
						break;
					default:
						for (int i = 1; i < 13; i++)
						{
							Buttons_FK[i][1] = 0;
						}
						for (int j = 1; j < 29; j++)
						{
							Buttons_MFK[j][1] = 0;
						}
						break;
					}
					break;
					//----- Правый МФИ
				case 62:
					switch (msgRead_1.data[1])
					{
						//---------- кнопки МФК (нижние)
					case 1:
						Buttons_MFK[20][2] = msgRead_1.data[1];
						break;
					case 2:
						Buttons_MFK[19][2] = msgRead_1.data[1];
						break;
					case 3:
						Buttons_MFK[18][2] = msgRead_1.data[1];
						break;
					case 4:
						Buttons_MFK[17][2] = msgRead_1.data[1];
						break;
					case 5:
						Buttons_MFK[16][2] = msgRead_1.data[1];
						break;
					case 6:
						Buttons_MFK[15][2] = msgRead_1.data[1];
						break;
					case 7:
						Buttons_MFK[14][2] = msgRead_1.data[1];
						break;
					case 8:
						Buttons_MFK[13][2] = msgRead_1.data[1];
						break;
					case 9:
						Buttons_MFK[12][2] = msgRead_1.data[1];
						break;
					case 10:
						Buttons_MFK[11][2] = msgRead_1.data[1];
						break;
					case 11:
						Buttons_MFK[10][2] = msgRead_1.data[1];
						break;
					case 12:
						Buttons_MFK[9][2] = msgRead_1.data[1];
						break;

						//---------- кнопки ФК (верхние 12 штук)
					case 13:
						Buttons_FK[12][2] = msgRead_1.data[1];
						break;
					case 14:
						Buttons_FK[11][2] = msgRead_1.data[1];
						break;
					case 15:
						Buttons_FK[10][2] = msgRead_1.data[1];
						break;
					case 16:
						Buttons_FK[9][2] = msgRead_1.data[1];
						break;
					case 17:
						Buttons_FK[8][2] = msgRead_1.data[1];
						break;
					case 18:
						Buttons_FK[7][2] = msgRead_1.data[1];
						break;
					case 19:
						Buttons_FK[6][2] = msgRead_1.data[1];
						break;
					case 20:
						Buttons_FK[5][2] = msgRead_1.data[1];
						break;
					case 21:
						Buttons_FK[4][2] = msgRead_1.data[1];
						break;
					case 22:
						Buttons_FK[3][2] = msgRead_1.data[1];
						break;
					case 23:
						Buttons_FK[2][2] = msgRead_1.data[1];
						break;
					case 24:
						Buttons_FK[1][2] = msgRead_1.data[1];
						break;

						//---------- кнопки МФК (боковые)
					case 33:
						Buttons_MFK[21][2] = msgRead_1.data[1];
						break;
					case 34:
						Buttons_MFK[22][2] = msgRead_1.data[1];
						break;
					case 35:
						Buttons_MFK[23][2] = msgRead_1.data[1];
						break;
					case 36:
						Buttons_MFK[24][2] = msgRead_1.data[1];
						break;
					case 37:
						Buttons_MFK[25][2] = msgRead_1.data[1];
						break;
					case 38:
						Buttons_MFK[26][2] = msgRead_1.data[1];
						break;
					case 39:
						Buttons_MFK[27][2] = msgRead_1.data[1];
						break;
					case 40:
						Buttons_MFK[28][2] = msgRead_1.data[1];
						break;
					case 42:
						Buttons_MFK[7][2] = msgRead_1.data[1];
						break;
					case 43:
						Buttons_MFK[8][2] = msgRead_1.data[1];
						break;
					case 45:
						Buttons_MFK[4][2] = msgRead_1.data[1];
						break;
					case 46:
						Buttons_MFK[5][2] = msgRead_1.data[1];
						break;
					case 47:
						Buttons_MFK[6][2] = msgRead_1.data[1];
						break;
					case 48:
						Buttons_MFK[1][2] = msgRead_1.data[1];
						break;
					case 49:
						Buttons_MFK[2][2] = msgRead_1.data[1];
						break;
					case 50:
						Buttons_MFK[3][2] = msgRead_1.data[1];
						break;
					default:
						for (int i = 1; i < 13; i++)
						{
							Buttons_FK[i][2] = 0;
						}
						for (int j = 1; j < 29; j++)
						{
							Buttons_MFK[j][2] = 0;
						}
						break;
					}
					break;
				default:
					break;
				}

				//----- СО-2010
				switch (msgRead_1.data[0])
				{
				case 60:
					switch (msgRead_1.data[1])
					{

					case 1:
						// FID кнопка
						soStruct.buttons[0] = true;//msgRead_1.data[1];
						break;
					case 2:
						// > кнопка
						soStruct.buttons[1] = true;//msgRead_1.data[1];
						break;
					case 3:
						// ^ кнопка
						soStruct.buttons[2] = true;//msgRead_1.data[1];
						break;
					case 4:
						// КОД кнопка
						if (msgRead_1.length <= 2) {
							soStruct.buttons[3] = true;//msgRead_1.data[1];
							soStruct.enteredCodeA = 0;
						}
						else if (msgRead_1.length > 2) {
							soStruct.buttons[3] = true;//msgRead_1.data[1];
							soStruct.enteredCodeA = msgRead_1.data[2] * 1000 + msgRead_1.data[3] * 100 + msgRead_1.data[4] * 10 + msgRead_1.data[5];
						}
						break;
					case 5:
						// ТЕСТ кнопка
						soStruct.buttons[4] = true;//msgRead_1.data[1];
						break;
					case 6:
						// ПВП кнопка
						soStruct.buttons[5] = true;//msgRead_1.data[1];
						break;
					case 7:
						// ВЫС кнопка
						soStruct.buttons[6] = true;//msgRead_1.data[1];
						break;
					case 8:
						// ACS кнопка
						soStruct.buttons[7] = true;//msgRead_1.data[1];
						break;
					case 9:
						// УВД кнопка
						soStruct.buttons[8] = true;//msgRead_1.data[1];
						break;
					case 10:
						// ГОТ кнопка
						soStruct.buttons[9] = true;//msgRead_1.data[1];
						break;
					case 11:
						// АВАР кнопка
						soStruct.buttons[10] = true;//msgRead_1.data[1];
						break;
					case 12:
						// ЗНАК кнопка
						soStruct.buttons[11] = true;//msgRead_1.data[1];
						break;
					case 23:
						//ЯРКОСТЬ реализована на контроллере
						break;
						//default:
						//	// обнуление нажатий
						//	memset( soStruct.buttons, 0, 12 );
						//	break;
					}
				}//switch ( msgRead_1.data[0] )
			}//if (msgRead_1.id == ERRORID)
		}//if (nRet == SUCCESS)
		else if (nRet == TIME_OUT)
		{
			//sut->RMI_IN.DevOtk2 = true;
			//std::cout << "can18 failure (TIME_OUT)" << std::endl;
		}//if (nRet == SUCCESS)
		else
		{
			sut->RMI_IN.DevOtk2 = true;
			std::cout << "can18 failure (ERROR)" << std::endl;
		}//if (nRet == SUCCESS)

		nRet = acCanRead(hCANDevice_2, &msgRead_2, ulReadCountPort2, &ulNumberOfReadPort2, &ovReceive_2);
		if (nRet == SUCCESS)
		{
			if (msgRead_2.id == ERRORID)
			{
				//						printf("Incorrect package! ERRORID! \n");
										//return 0;
			}
			else
			{
				int len_2 = msgRead_2.length;
				if (len_2 > 8 || len_2 < 0)
				{
					len_2 = 8;
				}
				for (int i = 0; i < len_2; i++)
				{
					data_2[i] = msgRead_2.data[i];
				}

				switch (msgRead_2.data[0])
				{
				case 71:
					AVN->PCIE_1680_E1_1[0] = BitIsSet(msgRead_2.data[2], 0); // СОК

					AVN->PCIE_1680_E1_1[1] = BitIsSet(msgRead_2.data[2], 1); // РЕГИСТРАТ

					AVN->PCIE_1680_E1_1[2] = BitIsSet(msgRead_2.data[2], 2);	// ТАБЛО СИГНАЛ

					AVN->PCIE_1680_E1_1[3] = BitIsSet(msgRead_2.data[2], 3);	// СИГНАЛ ПОЖАР

					AVN->PCIE_1680_E1_1[4] = BitIsSet(msgRead_2.data[2], 4);	// СИГНАЛ ПОЖАР БАГАЖ

					AVN->PCIE_1680_E1_1[5] = BitIsSet(msgRead_2.data[2], 5);	// ШАССИ

					AVN->PCIE_1680_E1_1[6] = BitIsSet(msgRead_2.data[2], 6);	// ЗАКРЫЛКИ

					AVN->PCIE_1680_E1_1[7] = BitIsSet(msgRead_2.data[2], 7);	// ТРИММЕРЫ		
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[8] = BitIsSet(msgRead_2.data[1], 0);		  // ИНТЕРЦ

					AVN->PCIE_1680_E1_1[9] = BitIsSet(msgRead_2.data[1], 1);		  // ЭЛЕКТР ТРИММЕРЫ

					AVN->PCIE_1680_E1_1[10] = BitIsSet(msgRead_2.data[1], 2);		  // АП

					AVN->PCIE_1680_E1_1[11] = BitIsSet(msgRead_2.data[1], 3);		  // УПРАВЛ КУРСОМ

					AVN->PCIE_1680_E1_1[12] = BitIsSet(msgRead_2.data[1], 4);		  // КОНЦЕВОЙ БАК ЛЕВ

					AVN->PCIE_1680_E1_1[13] = BitIsSet(msgRead_2.data[1], 5);		  // КОНЦЕВОЙ БАК ПРАВ

					AVN->PCIE_1680_E1_1[14] = BitIsSet(msgRead_2.data[1], 6);		  // ОБДУВ НАВ

					AVN->PCIE_1680_E1_1[15] = BitIsSet(msgRead_2.data[1], 7);		  // ВЕНТ ГО
					break;

				case 72:
					AVN->PCIE_1680_E1_1[16] = BitIsSet(msgRead_2.data[2], 0); // N1_S1A

					AVN->PCIE_1680_E1_1[17] = BitIsSet(msgRead_2.data[2], 1); // S3B

					AVN->PCIE_1680_E1_1[18] = BitIsSet(msgRead_2.data[2], 2); // N1_S2B

					AVN->PCIE_1680_E1_1[19] = BitIsSet(msgRead_2.data[2], 3); // N2_S1A

					AVN->PCIE_1680_E1_1[20] = BitIsSet(msgRead_2.data[2], 4); // S1B

					AVN->PCIE_1680_E1_1[21] = BitIsSet(msgRead_2.data[2], 5); //S3A

					AVN->PCIE_1680_E1_1[22] = BitIsSet(msgRead_2.data[2], 6); // N2_S2B

					AVN->PCIE_1680_E1_1[23] = BitIsSet(msgRead_2.data[2], 7); // ОБОГРЕВ ДАУ
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[24] = BitIsSet(msgRead_2.data[1], 0); // ОБОГРЕВ СТЕКОЛ ЛЕВ

					AVN->PCIE_1680_E1_1[25] = BitIsSet(msgRead_2.data[1], 1); // ОБОГРЕВ СТЕКОЛ ПРАВ

					AVN->PCIE_1680_E1_1[26] = BitIsSet(msgRead_2.data[1], 2); // СЕПАРАТОР ЛЬДА ЛЕВ

					AVN->PCIE_1680_E1_1[27] = BitIsSet(msgRead_2.data[1], 3); // СЕПАРАТОР ЛЬДА ПРАВ

					AVN->PCIE_1680_E1_1[28] = BitIsSet(msgRead_2.data[1], 4); // ПЛАНЕР

					AVN->PCIE_1680_E1_1[29] = BitIsSet(msgRead_2.data[1], 5); // ПВД 1

					AVN->PCIE_1680_E1_1[30] = BitIsSet(msgRead_2.data[1], 6); // ПВД 2

					AVN->PCIE_1680_E1_1[31] = BitIsSet(msgRead_2.data[1], 7); // СВАЛ
					break;

				case 73:
					AVN->PCIE_1680_E1_1[32] = BitIsSet(msgRead_2.data[2], 0); // АККУМ 1

					AVN->PCIE_1680_E1_1[33] = BitIsSet(msgRead_2.data[2], 1); // АККУМ 2

					AVN->PCIE_1680_E1_1[34] = BitIsSet(msgRead_2.data[2], 2); // ПРЕОБР 36В 1

					AVN->PCIE_1680_E1_1[35] = BitIsSet(msgRead_2.data[2], 3); // ПРЕОБР 36В 2

					AVN->PCIE_1680_E1_1[36] = BitIsSet(msgRead_2.data[2], 4); // ГЕН ПЕРЕМ ЛЕВ

					AVN->PCIE_1680_E1_1[37] = BitIsSet(msgRead_2.data[2], 5); // ГЕН ПЕРЕМ ПРАВ

					AVN->PCIE_1680_E1_1[38] = BitIsSet(msgRead_2.data[2], 6); // ГЕН ПОСТ ЛЕВ

					AVN->PCIE_1680_E1_1[39] = BitIsSet(msgRead_2.data[2], 7); // ГЕН ПОСТ ПРАВ
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[40] = BitIsSet(msgRead_2.data[1], 0); // КОНТРОЛЛЕР 1

					AVN->PCIE_1680_E1_1[41] = BitIsSet(msgRead_2.data[1], 1); // КОНТРОЛЛЕР 2

					AVN->PCIE_1680_E1_1[42] = BitIsSet(msgRead_2.data[1], 2); // МФИ ЛЕВ

					AVN->PCIE_1680_E1_1[43] = BitIsSet(msgRead_2.data[1], 3); // МФИ СРЕДН

					AVN->PCIE_1680_E1_1[44] = BitIsSet(msgRead_2.data[1], 4); // МФИ ПРАВ
					break;

				case 74:
					AVN->PCIE_1680_E1_1[48] = BitIsSet(msgRead_2.data[2], 0); // ЗАПУСК ЛЕВ

					AVN->PCIE_1680_E1_1[49] = BitIsSet(msgRead_2.data[2], 1); // ЗАПУСК ПРАВ

					AVN->PCIE_1680_E1_1[50] = BitIsSet(msgRead_2.data[2], 2); // ЭБО ЛЕВ

					AVN->PCIE_1680_E1_1[51] = BitIsSet(msgRead_2.data[2], 3); // ЭБО ПРАВ

					AVN->PCIE_1680_E1_1[52] = BitIsSet(msgRead_2.data[2], 4); // ФЛЮГИРОВАНИЕ АВТОМАТ КРЕН ЛЕВ

					AVN->PCIE_1680_E1_1[53] = BitIsSet(msgRead_2.data[2], 5); // ФЛЮГИРОВАНИЕ АВТОМАТ КРЕН ПРАВ
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[56] = BitIsSet(msgRead_2.data[1], 0); // БСОИ 1

					AVN->PCIE_1680_E1_1[57] = BitIsSet(msgRead_2.data[1], 1); // БСОИ 2

					AVN->PCIE_1680_E1_1[58] = BitIsSet(msgRead_2.data[1], 2); // ПНС 1

					AVN->PCIE_1680_E1_1[59] = BitIsSet(msgRead_2.data[1], 3); // ПНС 2

					AVN->PCIE_1680_E1_1[60] = BitIsSet(msgRead_2.data[1], 4); // ПНС 3
					break;

				case 75:
					AVN->PCIE_1680_E1_1[64] = BitIsSet(msgRead_2.data[2], 0); // ТОПЛ НАСОС ЛЕВ

					AVN->PCIE_1680_E1_1[65] = BitIsSet(msgRead_2.data[2], 1); // ТОПЛ НАСОС ПРАВ

					AVN->PCIE_1680_E1_1[66] = BitIsSet(msgRead_2.data[2], 2); // КОЛЬЦ ТОПЛ

					AVN->PCIE_1680_E1_1[67] = BitIsSet(msgRead_2.data[2], 3); // ИЗОЛ КЛАПАН ЛЕВ

					AVN->PCIE_1680_E1_1[68] = BitIsSet(msgRead_2.data[2], 4); // ИЗОЛ КЛАПАН ПРАВ
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[72] = BitIsSet(msgRead_2.data[1], 0); // ОТВЕТЧИК ГО

					AVN->PCIE_1680_E1_1[73] = BitIsSet(msgRead_2.data[1], 1); // ОТВЕТЧИК УВД

					AVN->PCIE_1680_E1_1[74] = BitIsSet(msgRead_2.data[1], 2); // МАРКЕР

					AVN->PCIE_1680_E1_1[75] = BitIsSet(msgRead_2.data[1], 3); // ПУИ 1

					AVN->PCIE_1680_E1_1[76] = BitIsSet(msgRead_2.data[1], 4); // ПУИ 2

					AVN->PCIE_1680_E1_1[77] = BitIsSet(msgRead_2.data[1], 5); // ЦВМ

					AVN->PCIE_1680_E1_1[78] = BitIsSet(msgRead_2.data[1], 6); // БИНС

					AVN->PCIE_1680_E1_1[79] = BitIsSet(msgRead_2.data[1], 7); // ИСРП
					break;

				case 76:
					AVN->PCIE_1680_E1_1[80] = BitIsSet(msgRead_2.data[2], 0); // СПУ

					AVN->PCIE_1680_E1_1[81] = BitIsSet(msgRead_2.data[2], 1); // ПУ 1

					AVN->PCIE_1680_E1_1[82] = BitIsSet(msgRead_2.data[2], 2); // ПУ 2

					AVN->PCIE_1680_E1_1[83] = BitIsSet(msgRead_2.data[2], 3); // УКВ 1

					AVN->PCIE_1680_E1_1[84] = BitIsSet(msgRead_2.data[2], 4); // УКВ 2

					AVN->PCIE_1680_E1_1[85] = BitIsSet(msgRead_2.data[2], 5); // КВ

					AVN->PCIE_1680_E1_1[86] = BitIsSet(msgRead_2.data[2], 6); // Б/ТЕХН

					AVN->PCIE_1680_E1_1[87] = BitIsSet(msgRead_2.data[2], 7); // АБОНЕНТ
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[88] = BitIsSet(msgRead_2.data[1], 0); // АРК

					AVN->PCIE_1680_E1_1[89] = BitIsSet(msgRead_2.data[1], 1); // РСБН

					AVN->PCIE_1680_E1_1[90] = BitIsSet(msgRead_2.data[1], 2); // ВОР ИЛС

					AVN->PCIE_1680_E1_1[91] = BitIsSet(msgRead_2.data[1], 3); // ДМЕ

					AVN->PCIE_1680_E1_1[92] = BitIsSet(msgRead_2.data[1], 4); // СВС 1

					AVN->PCIE_1680_E1_1[93] = BitIsSet(msgRead_2.data[1], 5); // СВС 2

					AVN->PCIE_1680_E1_1[94] = BitIsSet(msgRead_2.data[1], 6); // МЕТЕО

					AVN->PCIE_1680_E1_1[95] = BitIsSet(msgRead_2.data[1], 7); // РВ
					break;

				case 77:
					AVN->PCIE_1680_E1_1[96] = BitIsSet(msgRead_2.data[2], 0); // ФАРА 1 РУЛ

					AVN->PCIE_1680_E1_1[97] = BitIsSet(msgRead_2.data[2], 1); // ФАРА 1 ПОСАД

					AVN->PCIE_1680_E1_1[98] = BitIsSet(msgRead_2.data[2], 2); // ФАРА 2 РУЛ

					AVN->PCIE_1680_E1_1[99] = BitIsSet(msgRead_2.data[2], 3); // ФАРА 2 ПОСАД

					AVN->PCIE_1680_E1_1[100] = BitIsSet(msgRead_2.data[2], 4); // АНО

					AVN->PCIE_1680_E1_1[101] = BitIsSet(msgRead_2.data[2], 5); // МАЯК

					AVN->PCIE_1680_E1_1[102] = BitIsSet(msgRead_2.data[2], 6); // ПИЛОТ КАБИНА

					AVN->PCIE_1680_E1_1[103] = BitIsSet(msgRead_2.data[2], 7); // ПРИБОРЫ НАВИГАЦ
//--------------------------------------------------------------------
					AVN->PCIE_1680_E1_1[104] = BitIsSet(msgRead_2.data[1], 0); // РЕЗЕРВ ОСВЕЩ

					AVN->PCIE_1680_E1_1[105] = BitIsSet(msgRead_2.data[1], 1); // ПАС КАБИНА 1/3

					AVN->PCIE_1680_E1_1[106] = BitIsSet(msgRead_2.data[1], 2); // ПАС КАБИНА 2/3

					AVN->PCIE_1680_E1_1[107] = BitIsSet(msgRead_2.data[1], 3); // СВЕТОВОЕ ТАБЛО

					AVN->PCIE_1680_E1_1[108] = BitIsSet(msgRead_2.data[1], 4); // СВЕТ

					AVN->PCIE_1680_E1_1[109] = BitIsSet(msgRead_2.data[1], 5); // ВЕНТИЛЯТОР
					break;
				}
			}

		} // if (nRet == SUCCESS)
		else if (nRet == TIME_OUT)
		{
			//sut->RMI_IN.DevOtk1 = true;
			//std::cout << "can17 failure (TIME_OUT)" << std::endl;
		} // if (nRet == TIME_OUT)
		else
		{
			sut->RMI_IN.DevOtk1 = true;
			std::cout << "can17 failure (ERROR)" << std::endl;
		}

		//close_all = sut->Close;

		std::cout << indik_external[0] << " ";

		//////////////////////////////////////    WRITE FROM CAN  ///////////////////////////////////////////////////////////////////////
		EnterCriticalSection(&psc);

		if (AVN)
		{
			memcpy(&AVN->IBKO.MFI_IN.Buttons_FK, &Buttons_FK, sizeof(bool) * 13 * 3);
			memcpy(&AVN->IBKO.MFI_IN.Buttons_MFK, &Buttons_MFK, sizeof(bool) * 29 * 3);
			memcpy(&AVN->IBKO.MFPU_IN.Buttons, &Buttons, sizeof(bool) * 74 * 2);
			//memcpy(&AVN->IBKO.B8_LEFT_IN, &B8_L, sizeof(SHB8_LEFT_IN));
			//memcpy(&AVN->IBKO.B8_RIGHT_IN, &B8_R, sizeof(SHB8_LEFT_IN));
			memcpy(&AVN->IBKO.MFI_IN.brightness, &brightness, sizeof(int) * 3);
			memcpy(&AVN->IBKO.MFI_IN.brightness_button, &brightness_button, sizeof(bool) * 3);
			memcpy(&AVN->IBKO.MFI_IN.indik_internal, &indik_internal, sizeof(int) * 3);
			memcpy(&AVN->IBKO.MFI_IN.indik_external, &indik_external, sizeof(int) * 3);
			memcpy(&AVN->IBKO.MFI_IN.indik_button, &indik_button, sizeof(bool) * 3);

			//cab->ISRP26.Brightness_minus = Brightness_minus;			// нажатие кнопки уменьшения яркости (true - нажата, false - не нажата)
			//cab->ISRP26.Brightness_plus = Brightness_plus;	        // нажатие кнопки увеличения яркости (true - нажата, false - не нажата)
			//cab->ISRP26.Fast_Korrection = Fast_Korrection;			// Нажатие кнопки ускоренной коррекции
			//cab->ISRP26.HDG_press = HDG_press;			// Признак кратковременного нажатия кремальеры HDG;
			//cab->ISRP26.BARO_press = BARO_press;			// Признак кратковременного нажатия кремальеры BARO;
			//cab->ISRP26.BARO_roll = BARO_roll;			// Признак вращения ручки BARO (true - вращение, показывать окно, false - скрыть окно)
			//cab->ISRP26.HDG_roll = HDG_roll;			// вращение ручки HDG, стартовое значение равно 0. при вращении против часовой на 1 щелчок значение HDG должно уменьшаться на единицу,
			//															// при вращении по часовой увеличиваться на единицу. число может быть "плавающим", его обработку осуществляю сам
			//cab->Kontur1 = Kontur1;

			AVN->CAB.ISRP26.Brightness_minus = Brightness_minus;
			AVN->CAB.ISRP26.Brightness_plus = Brightness_plus;
			AVN->CAB.ISRP26.Fast_Korrection = Fast_Korrection;
			AVN->CAB.ISRP26.HDG_press = HDG_press;
			AVN->CAB.ISRP26.BARO_press = BARO_press;
			AVN->CAB.ISRP26.BARO_roll = BARO_roll / 2.;
			AVN->CAB.ISRP26.HDG_roll = HDG_roll / 2.;
			AVN->COM.PRIB.ISRP26.BARO_press = BARO_press;
			AVN->COM.PRIB.ISRP26.HDG_press = HDG_press;
			AVN->COM.PRIB.ISRP26.BARO_press = BARO_press;
			AVN->COM.PRIB.ISRP26.BARO_roll = BARO_roll / 2.;
			AVN->COM.PRIB.ISRP26.HDG_roll = HDG_roll / 2.;
			AVN->CAB.Kontur1 = Kontur1;

			memcpy(&AVN->CAN.fromCan.buttons[0], &soStruct.buttons[0], sizeof(bool) * 12);
			AVN->CAN.fromCan.enteredCodeA = soStruct.enteredCodeA;
			ZeroMemory(&soStruct.buttons[0], sizeof(bool) * 12);		// обнуление признаков нажатий кнопок на СО-2010, т.к. с контроллера не приходит признак отжатия кнопок
			//ZeroMemory( &brightness, sizeof(int) * 3);
			//ZeroMemory( &indik_internal, sizeof(int) * 3);
			//ZeroMemory( &indik_external, sizeof(int) * 3);
		}

		LeaveCriticalSection(&psc);
		//====================================  Write FROM CAN  ====================================================
	}//while(!closeAll)

	_itoa((int)Kontur1, kontur, 10);
	WritePrivateProfileStringA("PRIB", "Kontur", kontur, "CAN_Settings.ini");
	//========================================================
	if (ovReceive_1.hEvent)
	{
		CloseHandle(ovReceive_1.hEvent);
	}

	if (ovReceive_2.hEvent)
	{
		CloseHandle(ovReceive_2.hEvent);
	}

	/*acCanClose(hCANDevice_1);
	acCanClose(hCANDevice_2);
	hCANDevice_1 = INVALID_HANDLE_VALUE;
	hCANDevice_2 = INVALID_HANDLE_VALUE;*/

	CloseHandle(hTimer);
	DeleteCriticalSection(&psc);

	CloseHandle(hReceiveThread);
	hReceiveThread = 0;

	//UnmapViewOfFile(AVN);
	//CloseHandle(hMapFile);

	return 0;
}

DWORD WINAPI SendThreadMethod(LPVOID)
{
	HANDLE hTimer		= NULL;							// идентификатор объекта-таймера 
	LARGE_INTEGER li;

	DWORD dwTimeout		= 500;							//ms
	li.QuadPart			= -1000000;						//1000 ms do begin    //1 сек = -10 000 000 тиков по 100 ns;

	hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
//	SetWaitableTimer(hTimer, &li, 20, NULL, NULL, FALSE);	//Period=20ms
//	SetWaitableTimer(hTimer, &li, 50, NULL, NULL, FALSE);	//Period=50ms
	SetWaitableTimer(hTimer, &li, 100, NULL, NULL, FALSE);	//Period=100ms
//	SetWaitableTimer(hTimer, &li, 150, NULL, NULL, FALSE);	//Period=150ms

	while (!close_all)
	{
		DWORD temp = WaitForSingleObject(hTimer, dwTimeout);
		//////////////////////////////////////   Write TO CAN  /////////////////////////////////////////////////////

		// Б8-50КР левый
		if (data_1[0] == 42)
		{
			ZeroMemory(&msgWrite[4], sizeof(canmsg_t));
			msgWrite[4].id = 142;
			msgWrite[4].length = 2;
			msgWrite[4].data[0] = 42;
			msgWrite[4].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[4], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// Б8-50КР правый
		if (data_1[0] == 43)
		{
			ZeroMemory(&msgWrite[5], sizeof(canmsg_t));
			msgWrite[5].id = 143;
			msgWrite[5].length = 2;
			msgWrite[5].data[0] = 43;
			msgWrite[5].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[5], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// МФИ правый
		if (data_1[0] == 61)
		{
			ZeroMemory(&msgWrite[6], sizeof(canmsg_t));
			msgWrite[6].id = 161;
			msgWrite[6].length = 2;
			msgWrite[6].data[0] = 61;
			msgWrite[6].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[6], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// МФИ средний
		if (data_1[0] == 62)
		{
			ZeroMemory(&msgWrite[7], sizeof(canmsg_t));
			msgWrite[7].id = 162;
			msgWrite[7].length = 2;
			msgWrite[7].data[0] = 62;
			msgWrite[7].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[7], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// МФИ левый
		if (data_1[0] == 63)
		{
			ZeroMemory(&msgWrite[8], sizeof(canmsg_t));
			msgWrite[8].id = 163;
			msgWrite[8].length = 2;
			msgWrite[8].data[0] = 63;
			msgWrite[8].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[8], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// МФПУ левый
		if (data_1[0] == 64)
		{
			ZeroMemory(&msgWrite[0], sizeof(canmsg_t));
			msgWrite[0].id = 164;
			msgWrite[0].length = 2;
			msgWrite[0].data[0] = 64;
			msgWrite[0].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[0], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		if (data_1[0] == 65)
		{
			ZeroMemory(&msgWrite[1], sizeof(canmsg_t));
			msgWrite[1].id = 165;
			msgWrite[1].length = 2;
			msgWrite[1].data[0] = 65;
			msgWrite[1].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[1], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// МФПУ правый
		if (data_1[0] == 66)
		{
			ZeroMemory(&msgWrite[2], sizeof(canmsg_t));
			msgWrite[2].id = 166;
			msgWrite[2].length = 2;
			msgWrite[2].data[0] = 66;
			msgWrite[2].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[2], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		if (data_1[0] == 67)
		{
			ZeroMemory(&msgWrite[3], sizeof(canmsg_t));
			msgWrite[3].id = 167;
			msgWrite[3].length = 2;
			msgWrite[3].data[0] = 67;
			msgWrite[3].data[1] = msgRead_1.data[1];

			nRet = acCanWrite(hCANDevice_1, &msgWrite[3], 1, &ulNumberOfWritten, &ovReceive_1);
		}

		// ПОТОЛОЧНАЯ ПАНЕЛЬ (подтверждение)
		// ряды считаются сверху вниз
		// 1-й ряд
		if (data_2[0] == 71)
		{
			ZeroMemory(&msgWrite[13], sizeof(canmsg_t));
			msgWrite[13].id = 171;
			msgWrite[13].length = 3;
			msgWrite[13].data[0] = msgRead_2.data[0];
			msgWrite[13].data[1] = msgRead_2.data[1];
			msgWrite[13].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[13], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		// 2-й ряд
		if (data_2[0] == 72)
		{
			ZeroMemory(&msgWrite[14], sizeof(canmsg_t));
			msgWrite[14].id = 172;
			msgWrite[14].length = 3;
			msgWrite[14].data[0] = msgRead_2.data[0];
			msgWrite[14].data[1] = msgRead_2.data[1];
			msgWrite[14].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[14], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		// 3-й ряд
		if (data_2[0] == 73)
		{
			ZeroMemory(&msgWrite[15], sizeof(canmsg_t));
			msgWrite[15].id = 173;
			msgWrite[15].length = 3;
			msgWrite[15].data[0] = msgRead_2.data[0];
			msgWrite[15].data[1] = msgRead_2.data[1];
			msgWrite[15].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[15], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		// 4-й ряд
		if (data_2[0] == 74)
		{
			ZeroMemory(&msgWrite[16], sizeof(canmsg_t));
			msgWrite[16].id = 174;
			msgWrite[16].length = 3;
			msgWrite[16].data[0] = msgRead_2.data[0];
			msgWrite[16].data[1] = msgRead_2.data[1];
			msgWrite[16].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[16], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		// 5-й ряд
		if (data_2[0] == 75)
		{
			ZeroMemory(&msgWrite[17], sizeof(canmsg_t));
			msgWrite[17].id = 175;
			msgWrite[17].length = 3;
			msgWrite[17].data[0] = msgRead_2.data[0];
			msgWrite[17].data[1] = msgRead_2.data[1];
			msgWrite[17].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[17], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		// 6-й ряд
		if (data_2[0] == 76)
		{
			ZeroMemory(&msgWrite[18], sizeof(canmsg_t));
			msgWrite[18].id = 176;
			msgWrite[18].length = 3;
			msgWrite[18].data[0] = msgRead_2.data[0];
			msgWrite[18].data[1] = msgRead_2.data[1];
			msgWrite[18].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[18], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		// 7-й ряд
		if (data_2[0] == 77)
		{
			ZeroMemory(&msgWrite[19], sizeof(canmsg_t));
			msgWrite[19].id = 177;
			msgWrite[19].length = 3;
			msgWrite[19].data[0] = msgRead_2.data[0];
			msgWrite[19].data[1] = msgRead_2.data[1];
			msgWrite[19].data[2] = msgRead_2.data[2];

			nRet = acCanWrite(hCANDevice_2, &msgWrite[19], 1, &ulNumberOfWrittenPort2, &ovReceive_2);
		}
		////////////////////////////
		for (int i = 0; i < 8; i++)
		{
			data_1[i] = 0;
			data_2[i] = 0;
		}

		// выдача в CAN

		/* Выдача на шарик и компас */

		if (abs(old_soMessage - AVN->CAN.toCan.message) >= 1 || old_soDisplayedCode != AVN->CAN.toCan.displayedCode)
		{
			// выдача на СО-2010
			ZeroMemory(&msgWrite[9], sizeof(canmsg_t));
			msgWrite[9].id = 60;
			msgWrite[9].length = 6;
			msgWrite[9].data[0] = 60;
			msgWrite[9].data[1] = AVN->CAN.toCan.message;
			if (AVN->CAN.toCan.message != 10) {
				msgWrite[9].data[2] = AVN->CAN.toCan.displayedCode / 1000;
				msgWrite[9].data[3] = (AVN->CAN.toCan.displayedCode / 100) % 10;
				msgWrite[9].data[4] = (AVN->CAN.toCan.displayedCode / 10) % 10;
				msgWrite[9].data[5] = AVN->CAN.toCan.displayedCode % 10;
			}
			else {
				msgWrite[9].data[2] = 0;
				msgWrite[9].data[3] = 1;
			}

			nRet = acCanWrite(hCANDevice_1, &msgWrite[9], 1, &ulNumberOfWritten, &ovReceive_1);
			old_soMessage = AVN->CAN.toCan.message;
			old_soDisplayedCode = AVN->CAN.toCan.displayedCode;
		}

		if (fabs(old_arkFrequency - AVN->CAN.toCan.ark_Freq) >= 0.5)
		{
			// выдача на АРК-40
			USHORT arkFreq_16b = AVN->CAN.toCan.ark_Freq;			// 2-байтовая переменная, хранящая значение частоты АРК без дробной части
			ZeroMemory(&msgWrite[10], sizeof(canmsg_t));
			msgWrite[10].id = 51;
			msgWrite[10].length = 4;
			msgWrite[10].data[0] = 51;
			if (arkFreq_16b == 65535)
			{
				msgWrite[10].data[1] = 255;
				msgWrite[10].data[2] = 255;
				msgWrite[10].data[3] = 255;
			}
			else
			{
				msgWrite[10].data[1] = hiBite(arkFreq_16b);
				msgWrite[10].data[2] = loBite(arkFreq_16b);
				if ((AVN->CAN.toCan.ark_Freq - arkFreq_16b) > 0.00005) {
					msgWrite[10].data[3] = 5;
				}
				else {
					msgWrite[10].data[3] = 0;
				}
			}

			nRet = acCanWrite(hCANDevice_1, &msgWrite[10], 1, &ulNumberOfWritten, &ovReceive_1);
			old_arkFrequency = AVN->CAN.toCan.ark_Freq;
		}

		if (fabs(old_kompasKurs - AVN->CAN.toCan.KompasKurs) >= 1.)
		{
			// выдача на компас
			USHORT kompasKurs = AVN->CAN.toCan.KompasKurs;
			ZeroMemory(&msgWrite[11], sizeof(canmsg_t));
			msgWrite[11].id = 80;
			msgWrite[11].length = 3;
			msgWrite[11].data[0] = 80;
			// в старший байт записывается 0, если курс меньше 255, 1, если курс больше 255
			// в младший байт записывается значние от 0 до 255 (в итоге получается комбинация младшего и старшего байт)
			msgWrite[11].data[1] = kompasKurs / 255;
			msgWrite[11].data[2] = kompasKurs - (msgWrite[11].data[1] * 255);

			nRet = acCanWrite(hCANDevice_1, &msgWrite[11], 1, &ulNumberOfWritten, &ovReceive_1);
			old_kompasKurs = AVN->CAN.toCan.KompasKurs;
		}

		if (fabs(old_sharik - AVN->CAN.toCan.Sharik) >= 1.)
		{
			// выдача на Шарик
			ZeroMemory(&msgWrite[12], sizeof(canmsg_t));
			msgWrite[12].id = 88;
			msgWrite[12].length = 2;
			msgWrite[12].data[0] = 88;
			msgWrite[12].data[1] = (AVN->CAN.toCan.Sharik + 1) * 50;

			nRet = acCanWrite(hCANDevice_1, &msgWrite[12], 1, &ulNumberOfWritten, &ovReceive_1);
			old_sharik = AVN->CAN.toCan.Sharik;
		}
		//==================================end  Write   TO  CAN  =================================================

		//close_all = sut->Close;
	}

	CloseHandle(hTimer);
	CloseHandle(hSendThread);
	hSendThread = 0;

	return 0;
}

int main()
{
	//PROCESS_INFORMATION hProc;

	HANDLE curProcess = GetCurrentProcess();

	SetPriorityClass(curProcess, ABOVE_NORMAL_PRIORITY_CLASS); // NORMAL_PRIORITY_CLASS);//BELOW_NORMAL_PRIORITY_CLASS);//IDLE_PRIORITY_CLASS);

	/* Считывание имени порта из ini-файла */
	GetPrivateProfileString(L"Port_1", L"PortName_1", L"can1", portName_1, 7, L"CAN_Settings.ini");
	GetPrivateProfileString(L"Port_2", L"PortName_2", L"can2", portName_2, 7, L"CAN_Settings.ini");

	/* Инициализация первого порта CAN */
	hCANDevice_1 = acCanOpen(portName_1, FALSE);
	if (hCANDevice_1 == INVALID_HANDLE_VALUE)
	{
		std::cout << "hCANDevice_1 acCanOpen error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}
	else
	{
		std::cout << "hCANDevice_1 acCanOpen OK" << std::endl;
	}

	nRet = acEnterResetMode(hCANDevice_1);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acEnterResetMode error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acClearRxFifo(hCANDevice_1);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acClearRxFifo error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetBaud(hCANDevice_1, CAN_TIMING_500K);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acSetBaud error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetAcceptanceFilterMask(hCANDevice_1, 0xffffffff);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acSetAcceptanceFilterMask error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetAcceptanceFilterCode(hCANDevice_1, 0xffffffff);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acSetAcceptanceFilterCode error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetTimeOut(hCANDevice_1, ulReadTimeOut, ulWriteTimeOut);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acSetTimeOut error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acEnterWorkMode(hCANDevice_1);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_1 acEnterWorkMode error!" << std::endl;
		acCanClose(hCANDevice_1);
		hCANDevice_1 = INVALID_HANDLE_VALUE;
		return 0;
	}
	else
	{
		std::cout << "hCANDevice_1 acEnterWorkMode OK" << std::endl;
	}
 
	/* Инициализация второго порта CAN */

	hCANDevice_2 = acCanOpen(portName_2, FALSE);
	if (hCANDevice_2 == INVALID_HANDLE_VALUE)
	{
		std::cout << "hCANDevice_2 acCanOpen_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}
	else
	{
		std::cout << "hCANDevice_2 acCanOpen_2 OK" << std::endl;
	}

	nRet = acEnterResetMode(hCANDevice_2);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acEnterResetMode_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acClearRxFifo(hCANDevice_2);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acClearRxFifo_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetBaud(hCANDevice_2, CAN_TIMING_500K);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acSetBaud_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetAcceptanceFilterMask(hCANDevice_2, 0xffffffff);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acSetAcceptanceFilterMask_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetAcceptanceFilterCode(hCANDevice_2, 0xffffffff);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acSetAcceptanceFilterCode_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acSetTimeOut(hCANDevice_2, ulReadTimeOutPort2, ulWriteTimeOutPort2);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acSetTimeOut_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}

	nRet = acEnterWorkMode(hCANDevice_2);
	if (nRet < 0)
	{
		std::cout << "hCANDevice_2 acEnterWorkMode_2 error!" << std::endl;
		acCanClose(hCANDevice_2);
		hCANDevice_2 = INVALID_HANDLE_VALUE;
		return 0;
	}
	else
	{
		std::cout << "hCANDevice_2 acEnterWorkMode_2 OK" << std::endl;
	}
//
//------------
//  
// 
	//ZeroMemory(AVN, sizeof(SHAVN_410));
	//ZeroMemory(cab, sizeof())
	ZeroMemory(&ovReceive_1, sizeof(OVERLAPPED));
	ZeroMemory(&ovReceive_2, sizeof(OVERLAPPED));
	ZeroMemory(&msgRead_1, sizeof(canmsg_t));
	ZeroMemory(&msgRead_2, sizeof(canmsg_t));
	ZeroMemory(&msgWrite, sizeof(canmsg_t));
	//ZeroMemory(&B8_L, sizeof(SHB8_LEFT_IN));
	//ZeroMemory(&B8_R, sizeof(SHB8_LEFT_IN));
	ZeroMemory(Buttons_FK, sizeof(bool) * 13 * 3);
	ZeroMemory(Buttons_MFK, sizeof(bool) * 29 * 3);
	ZeroMemory(Buttons, sizeof(bool) * 74 * 2);
	ZeroMemory(&soStruct, sizeof(soStruct_t));

	ovReceive_1.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ovReceive_2.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* **********  AVN filemapping opening   ************* */

	

	hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHAVN_410), shMemName);
	//hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shMemName);
	if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "hMapFile OpenFileMapping error!" << std::endl;
		return 0;
	}

	AVN = (SHAVN_410*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SHAVN_410));
	if (AVN == NULL || AVN == INVALID_HANDLE_VALUE)
	{
		std::cout << "AVN MapViewOfFile error!" << std::endl;
		return 0;
	}

	Sleep(50);

	/* **********  Sut filemapping opening   ************* */
	hMapFileSUT = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SHSUT), shSUTMemName);
	//hMapFileSUT = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shSUTMemName);
	if (hMapFileSUT == NULL || hMapFileSUT == INVALID_HANDLE_VALUE)
	{
		std::cout << "hMapFileSUT OpenFileMapping error!" << std::endl;
		return 0;
	}

	sut = (SHSUT*)MapViewOfFile(hMapFileSUT, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SHSUT));
	if (sut == NULL || sut == INVALID_HANDLE_VALUE)
	{
		std::cout << "sut MapViewOfFile error!" << std::endl;
		return 0;
	}

	/*********    Threads creating         *************/
	
	hReceiveThread	= CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ReceiveThreadMethod, 0, 0, &dwReceiveThreadID);
	if (!hReceiveThread)
	{
		std::cout << "Не создан поток приема" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "CAN receive thread created OK!" << std::endl;
	}
	SetThreadPriority(hReceiveThread, THREAD_PRIORITY_NORMAL); //THREAD_PRIORITY_NORMAL);

	hSendThread		= CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SendThreadMethod, 0, 0, &dwSendThreadID);
	if (!hSendThread)
	{
		std::cout << "Не создан поток передачи" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "CAN send thread created OK!" << std::endl;
	}
	SetThreadPriority(hSendThread, THREAD_PRIORITY_NORMAL); //THREAD_PRIORITY_NORMAL);
//
//------------
//

	while(hReceiveThread != 0 || hSendThread != 0)
	{
		close_all = sut->Close;
		Sleep(1000);
	}
	

//	system("pause");

	acCanClose(hCANDevice_1);
	acCanClose(hCANDevice_2);

	hCANDevice_1 = INVALID_HANDLE_VALUE;
	hCANDevice_2 = INVALID_HANDLE_VALUE;

	UnmapViewOfFile(AVN);
	UnmapViewOfFile(sut);

	CloseHandle(hMapFile);
	CloseHandle(hMapFileSUT);

	//return 1;
}

#define _ini_pathT_
#ifdef _ini_pathT_
char* ini_pathT(char* T)
{
	char* arr = new char[MAX_PATH];

	GetModuleFileNameA(NULL, arr, MAX_PATH);

	for (int i = strlen(arr) - 1; i > 0; i--)
		if (arr[i] == '\\')
		{
			arr[i + 1] = '\0';
			break;
		}

	//strcat(arr, "SettingsAdvan.ini");
	strcat(arr, T);
	return arr;
}
#else
char* ini_pathT(char* T);
#endif