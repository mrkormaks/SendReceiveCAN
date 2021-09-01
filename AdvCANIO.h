// #############################################################################
// *****************************************************************************
//                  Copyright (c) 2009, Advantech Automation Corp.
//      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
//               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.
//
//    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
//               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
// *****************************************************************************

// #############################################################################
//
// File:    AdvCANIO.h
// Created: 4/8/2009
// Version: 1.0
// Description: Implements IO function about how to access CAN WDM&CE driver
//
// -----------------------------------------------------------------------------

#define SUCCESS                           0                                       //Status definition : success
#define OPERATION_ERROR                   -1                                      //Status definition : device error or parameter error
#define TIME_OUT                          -2                                      //Status definition : time out


/*****************************************************************************
*
*    acCanOpen
*
*    Purpose:
*        Open can port by name 
*		
*
*    Arguments:
*        PortName            - port name
*        synchronization     - TRUE, synchronization ; FALSE, asynchronous
*
*    Returns:
*        Hanlde of device 
*
*****************************************************************************/
HANDLE acCanOpen(TCHAR *PortName, BOOL synchronization=FALSE)
{
	HANDLE hDevice;
	TCHAR CanName[20] = TEXT("\\\\.\\");
	_tcscat(CanName, PortName);
	hDevice = NULL;
	if(synchronization)
	{
		hDevice = CreateFile(CanName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	}
	else{
		hDevice = CreateFile(CanName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
			NULL);
	}
	
	return hDevice;
}

/*****************************************************************************
*
*    acCanClose
*
*    Purpose:
*        Close can port by handle 
*		
*
*    Arguments:
*        hDevice          - handle of device
*
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acCanClose(HANDLE hDevice)
{
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		if(!CloseHandle(hDevice))
			return OPERATION_ERROR;
		
	}
	return SUCCESS;
}

/*****************************************************************************
*
*    acEnterResetMode
*
*    Purpose:
*        Enter reset mode.
*		
*
*    Arguments:
*        hDevice            - handle of device
*
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acEnterResetMode(HANDLE hDevice)            
{
	Command_par_t cmd;
	BOOL flag;
	ULONG ulOutLen;

        cmd.cmd = CMD_STOP;	
        flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_COMMAND,
		&cmd,
		sizeof(Command_par_t),
      NULL,
      0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}
	
	return SUCCESS;
}

/*****************************************************************************
*
*    acEnterWorkMode
*
*    Purpose:
*        Enter work mode 
*		
*
*    Arguments:
*        hDevice        - handle of device
*
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acEnterWorkMode(HANDLE hDevice)              
{
	Command_par_t cmd;
	BOOL flag;
	ULONG ulOutLen;

        cmd.cmd = CMD_START;	
	flag = DeviceIoControl (hDevice,
                (ULONG)CAN_IOCTL_COMMAND,
                &cmd,
                sizeof(Command_par_t),
                NULL,
                0,
                &ulOutLen,
                0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}
	
	return SUCCESS;
}

/*****************************************************************************
*
*    acClearRxFifo
*
*    Purpose:
*        Clear can port rx buffer by handle 
*		
*
*    Arguments:
*        hDevice        - handle of device
*
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acClearRxFifo(HANDLE hDevice)
{
	Command_par_t cmd;
	BOOL flag;
	ULONG ulOutLen;

        cmd.cmd = CMD_CLEARBUFFERS;	
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_COMMAND,
		&cmd,
		sizeof(Command_par_t),
                NULL,
                0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}
	
	return SUCCESS;
}

/*****************************************************************************
*
*    acSetBaud
*
*    Purpose:
*        Set baudrate of the CAN Controller.The two modes of configuring
*     baud rate are custom mode and standard mode.
*     -   Custom mode
*         If Baud Rate value is user defined, driver will write the first 8
*         bit of low 16 bit in BTR0 of SJA1000.
*         The lower order 8 bit of low 16 bit will be written in BTR1 of SJA1000.
*     -   Standard mode
*         Target value     BTR0      BTR1      Setting value 
*           10K            0x31      0x1c      10 
*           20K            0x18      0x1c      20 
*           50K            0x09      0x1c      50 
*          100K            0x04      0x1c      100 
*          125K            0x03      0x1c      125 
*          250K            0x01      0x1c      250 
*          500K            0x00      0x1c      500 
*          800K            0x00      0x16      800 
*         1000K            0x00      0x14      1000 
*		
*
*    Arguments:
*        hDevice        - handle of device
*        nBaud          - baudrate will be set
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetBaud(HANDLE hDevice, unsigned int nBaud)
{
	Config_par_t config;
	BOOL flag;
	ULONG  ulOutLen;
	
	config.target = CONF_TIMING;
	config.val1 = nBaud;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}

	return SUCCESS;
}
/*****************************************************************************
*
*    acSetBaudRegister
*
*    Purpose:
*        Configures baud rate by custom mode.
*		
*
*    Arguments:
*        hDevice        - handle of device
*        Btr0           - BTR0 register value.
*        Btr1           - BTR1 register value.
*    Returns:
*	 =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetBaudRegister(HANDLE hDevice, unsigned char Btr0, unsigned char Btr1)
{
   unsigned int BaudRateValue = Btr0 * 256;
	BaudRateValue += Btr1;
   return acSetBaud(hDevice, BaudRateValue);
}
/*****************************************************************************
*
*    acSetTimeOut
*
*    Purpose:
*        Set Timeout for read and write
*		
*
*    Arguments:
*        hDevice           - handle of device
*        ulReadTimeOut     - ms
*         ulWriteTimeOut   - ms
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetTimeOut(HANDLE hDevice, ULONG ulReadTimeOut, ULONG ulWriteTimeOut)
{
	Config_par_t config;
	BOOL flag;
	
	ULONG ulOutLen;
	config.target = CONF_TIMEOUT;
	config.val2 = ulReadTimeOut;
    config.val1 = ulWriteTimeOut;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}
	
	return SUCCESS;
}

/*****************************************************************************
*
*    acSetSelfReception
*
*    Purpose:
*        Set Self Reception mode
*		
*
*    Arguments:
*        hDevice        - handle of device
*        bSelfFlag      - TRUE, open self reception; FALSE close self reception
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetSelfReception(HANDLE hDevice, BOOL bSelfFlag)
{
	Config_par_t config;
	BOOL flag;
	ULONG ulOutLen;
	
   config.target = CONF_SELF_RECEPTION;
	if(bSelfFlag)
		config.val1 = 1;
	else
		config.val1 = 0;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}
	
	return SUCCESS;
}

/*****************************************************************************
*
*    acSetListenOnlyMode
*
*    Purpose:
*        Set listen only mode
*	
*
*    Arguments:
*        hDevice            - Handle of device
*        bListerOnlyFlag    - TRUE, open only listen mode; FALSE, close only listen mode
*    Returns:
*        =0 succeeded; or <0 Failed 
*
*****************************************************************************/
int acSetListenOnlyMode(
                                 HANDLE hDevice,
                                 BOOL   bListerOnlyFlag
                                 )
{
    Config_par_t config;
    BOOL flag;
    ULONG  ulOutLen;
	
   config.target = CONF_LISTEN_ONLY_MODE;
	if(bListerOnlyFlag)
		config.val1 = 1;
	else
		config.val1 = 0;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}

	return SUCCESS;   
}

/*****************************************************************************
*
*    acSetAcceptanceFilterMode
*
*    Purpose:
*        Set acceptance filter mode
*	
*
*    Arguments:
*        hDevice         - Handle of device
*        nFilterMode     - PELICAN_SINGLE_FILTER, single filter mode; PELICAN_DUAL_FILTER, dule filter mode
*    Returns:
*        =0 succeeded; or <0 Failed 
*
*****************************************************************************/
int acSetAcceptanceFilterMode(
                                 HANDLE hDevice,
                                 int   nFilterMode
                                 )
{
	Config_par_t config;
	BOOL flag;
	
	ULONG  ulOutLen;

	config.target = CONF_ACC_FILTER;
	config.val1 = nFilterMode;
	
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}

	return SUCCESS;   
}



/*****************************************************************************
*
*    acSetAcceptanceFilterCode
*
*    Purpose:
*        Set acceptance code
*		
*
*    Arguments:
*        hDevice              - handle of device
*        ulAcceptanceCode		- acceptance code
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetAcceptanceFilterCode(HANDLE hDevice, ULONG ulAcceptanceCode)
{
   Config_par_t config;
	BOOL flag;
	
	ULONG ulOutLen;
	
	config.target = CONF_ACCC;
	config.val1   = ulAcceptanceCode;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}

	return SUCCESS;
}

/*****************************************************************************
*
*    acSetAcceptanceFilterMask
*
*    Purpose:
*        Set acceptance mask
*		
*
*    Arguments:
*        hDevice              - handle of device
*        ulAcceptanceMask     - accept mask code
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetAcceptanceFilterMask(HANDLE hDevice, ULONG ulAcceptanceMask)
{
	Config_par_t config;
	BOOL flag;
	ULONG ulOutLen;
	
	

	config.target = CONF_ACCM;
	config.val1   = ulAcceptanceMask;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}

	return SUCCESS;
}

/*****************************************************************************
*
*    acSetAcceptanceFilter
*
*    Purpose:
*        Set acceptance code and Mask
*		
*
*    Arguments:
*        hDevice                 - handle of device
*        ulAcceptanceCode        - acceptance code
*        ulAcceptanceMask        - acceptance mask
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acSetAcceptanceFilter(HANDLE hDevice, ULONG ulAcceptanceCode, ULONG ulAcceptanceMask)
{
	Config_par_t config;
	BOOL flag;
	ULONG ulOutLen;

	config.target = CONF_ACC;
	config.val1   = ulAcceptanceMask;
	config.val2   = ulAcceptanceCode;
	flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_CONFIG,
		&config,
		sizeof(Config_par_t),
		NULL,
		0,
		&ulOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}

	return SUCCESS;
}

/*****************************************************************************
*
*    acCanWrite
*
*    Purpose:
*        Write can msg
*		
*
*    Arguments:
*        hDevice                      - handle of device
*        pbyData                      - data buffer
*        ulWriteCount                 - msgs number
*        pulNumberofWritten           - real msgs have written
*        ov                           - synchronization event
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acCanWrite(HANDLE hDevice, canmsg_t *pbyData, ULONG ulWriteCount, ULONG *pulNumberofWritten, OVERLAPPED *ov)
{
	ULONG ulErr = 0;

	*pulNumberofWritten = 0;
	BOOL flag = WriteFile(hDevice, pbyData, ulWriteCount, pulNumberofWritten, ov);
	if (flag)
	{
      if(ulWriteCount > *pulNumberofWritten)
         return TIME_OUT;
      return SUCCESS;	
	}
	else{
      ulErr = GetLastError();
      if ( ulErr == ERROR_IO_PENDING )
      {
         if( GetOverlappedResult( hDevice, ov, pulNumberofWritten, TRUE ) )         //retrieves the results of overlapped operation
         { 
            if( ulWriteCount > *pulNumberofWritten)
               return TIME_OUT;
            else
               return SUCCESS;
         }
         else
            return OPERATION_ERROR;
      }
      else
         return OPERATION_ERROR;
	}
}

/*****************************************************************************
*
*    acCanRead
*
*    Purpose:
*        Read can message
*		
*
*    Arguments:
*        hDevice           - handle of device
*        pbyData           - data buffer
*        ulReadCount       - msg number of data buffer size
*        pulNumberofRead   - real msgs have read
*        ov                - synchronization event
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acCanRead(HANDLE hDevice, canmsg_t *pbyData, ULONG ulReadCount, ULONG *pulNumberofRead, OVERLAPPED *ov)
{
	ULONG ulErr = 0;

	*pulNumberofRead = 0;
	BOOL flag = ReadFile(hDevice, pbyData, ulReadCount, pulNumberofRead, ov);
	if (flag)
	{
      if(*pulNumberofRead == 0)
          return TIME_OUT;
      return SUCCESS;	
	}
	else{
		ulErr = GetLastError();
      if ( ulErr == ERROR_IO_PENDING )
      {
         if ( GetOverlappedResult( hDevice, ov, pulNumberofRead, TRUE ) )    //retrieves the results of overlapped operation
         {
           if(*pulNumberofRead == 0)
		     {
               return TIME_OUT;
            }
            else
            {
               return SUCCESS;
            }
         }
         else
            return OPERATION_ERROR;
      }
      else return OPERATION_ERROR;
	}
}
/*****************************************************************************
*
*    acGetStatus
*
*    Purpose:
*        Get driver current status
*		
*
*    Arguments:
*        hDevice           - handle of device
*        CanStatus         - status buffer
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acGetStatus(HANDLE hDevice, CanStatusPar_t * CanStatus)
{
	ULONG			dwOutLen;


	int flag = DeviceIoControl (hDevice,
		(ULONG)CAN_IOCTL_STATUS,
		NULL,
		0,
		CanStatus,
		sizeof(CanStatusPar_t),
		&dwOutLen,
		0
		);
	if(!flag)
	{
		return OPERATION_ERROR;
	}
	return SUCCESS;
}
/*****************************************************************************
*
*    acClearCommError
*
*    Purpose:
*        Execute ClearCommError of AdvCan.
*		
*
*    Arguments:
*        hDevice           - handle of device
*        ErrorCode         - error code if the CAN Controller occur error
* 
* 
*    Returns:
*        TRUE SUCCESS; or FALSE failure 
*
*****************************************************************************/
BOOL acClearCommError(HANDLE hDevice, ULONG *ulErrorCode)
{
   COMSTAT State;
   return ClearCommError(hDevice, ulErrorCode, &State);
}

/*****************************************************************************
*
*    acSetCommMask
*
*    Purpose:
*        Execute SetCommMask.
*		
*
*    Arguments:
*        hDevice           - handle of device
*        EvtMask           - event type
* 
* 
*    Returns:
*        TRUE SUCCESS; or FALSE failure 
*
*****************************************************************************/
BOOL acSetCommMask(HANDLE hDevice, ULONG ulEvtMask)
{
   return SetCommMask(hDevice, ulEvtMask);
}

/*****************************************************************************
*
*    acGetCommMask
*
*    Purpose:
*        Execute GetCommMask.
*		
*
*    Arguments:
*        hDevice         - handle of device
*        EvtMask         - event type
* 
* 
*    Returns:
*        TRUE SUCCESS; or FALSE failure 
*
*****************************************************************************/
BOOL acGetCommMask(HANDLE hDevice, ULONG *ulEvtMask)
{
   return GetCommMask(hDevice, ulEvtMask);
}
/*****************************************************************************
*
*    acWaitEvent
*
*    Purpose:
*        Wait can message or error of the CAN Controller.
*		
*
*    Arguments:
*        hDevice              - handle of device
*        pbyData              - buffer for read
*        nReadCount           - msgs number
*        pulNumberofRead      - real msgs have read
*        ErrorCode            - return error code when the CAN Controller has error
*        ov                   - synchronization event
* 
*    Returns:
*        =0 SUCCESS; or <0 failure 
*
*****************************************************************************/
int acWaitEvent(HANDLE hDevice, canmsg_t *pbyData, ULONG ulReadCount, ULONG *pulNumberofRead, ULONG *ErrorCode, OVERLAPPED *ov)
{
   ULONG Code = 0;
   int nRet = OPERATION_ERROR;

   if (WaitCommEvent(hDevice, &Code, ov) == TRUE)
   {
      if ((Code & EV_RXCHAR) != 0)
      {
         nRet = acCanRead(hDevice, pbyData, ulReadCount, pulNumberofRead, ov);
      }
      if ((Code & EV_ERR) != 0)
      {
         nRet = OPERATION_ERROR;
         acClearCommError(hDevice, ErrorCode);
      }
   }
   else
   {
      ULONG ulErr = GetLastError();
      if (ERROR_IO_PENDING == ulErr)
      {
         if (GetOverlappedResult(hDevice, ov, pulNumberofRead, TRUE))
         {
            if ((Code & EV_RXCHAR) != 0)
            {
               nRet = acCanRead(hDevice, pbyData, ulReadCount, pulNumberofRead, ov);
            }
            if ((Code & EV_ERR) != 0)
            {
               nRet = OPERATION_ERROR;
               acClearCommError(hDevice, ErrorCode);
            }
         }
      }
   }

   return nRet;
}

