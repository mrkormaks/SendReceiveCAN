/*
===========================================================================

... для описания ...

===========================================================================
*/

#pragma pack( push,1)

// подструктура СО2010
typedef struct {
	bool				buttons[12];		// состояние кнопок СО2010
	int					codeA;					// код режима А
	int 				codeFID;				// код режима FID
	int 				codePVP;				// код режима PVP
	int 				message;				// сообщение, отправляемое и отображаемое на СО2010
} soStruct_t;

// подструктура АРК40
typedef struct {
	double 				frequency;				// частота, отправляемая и отображаемая на АРК40
} arkStruct_t;

// главная структура CAN
typedef struct canStruct_c {					
	soStruct_t			so2010;					// СО2010 
	arkStruct_t			ark40;					// АРК40
} canStruct_t;

#pragma pack( pop )