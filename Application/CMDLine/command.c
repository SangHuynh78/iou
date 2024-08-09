/*
 * command.c
 *
 * Created: 5/19/2024 8:37:53 PM
 *  Author: Admin
 */ 
#include "scheduler.h"
#include "command.h"
#include "uart.h"
#include "temperature.h"
#include <stdlib.h>
#include "cmdline.h"
#include "ir_led.h"
#include "Accel_Gyro.h"
#include "Pressure.h"
#include "AD.h"
#include "CAN_Protocol.h"
#include <avr/pgmspace.h>

/* Private typedef -----------------------------------------------------------*/
typedef struct _Command_TaskContextTypedef_
{
	SCH_TASK_HANDLE               taskHandle;
	SCH_TaskPropertyTypedef       taskProperty;
} Command_TaskContextTypedef;


//typedef enum{OK = 0, CMDLINE_BAD_CMD, CMDLINE_TOO_MANY_ARGS, CMDLINE_TOO_FEW_ARGS, CMDLINE_INVALID_ARG} command_error_code_t;
const char  * ErrorCode[5] = {"OK\r\n", "CMDLINE_BAD_CMD\r\n", "CMDLINE_TOO_MANY_ARGS\r\n",
"CMDLINE_TOO_FEW_ARGS\r\n", "CMDLINE_INVALID_ARG\r\n" };

static	void	command_task_update(void);
tCmdLineEntry g_psCmdTable[] = {{"help", Cmd_help," | format: help" },
								{"set_temp", Cmd_set_temp," | format: set_temp channel setpoint (250 mean 25 Celcius)"},
								{"get_temp", Cmd_get_temp , " | format: get_temp NTC channel, get_temp onewire channel, get_temp bmp390"},
								{"get_temp_setpoint", Cmd_get_temp_setpoint , " | format: get_temp_setpoint  channel",},
								{"tec_ena", Cmd_TEC_enable, " | format: tec_ena channel"},
								{"tec_dis", Cmd_TEC_disable, " | format: tec_dis channel"},
								{"tec_ena_auto", Cmd_TEC_enable_auto_control, " | format: tec_ena_auto channel"},
								{"tec_dis_auto", Cmd_TEC_disable_auto_control, " | format: tec_dis_auto channel"},
								{"tec_set_output", Cmd_TEC_set_output, " | format: tec_set_output channel heat_cool(0:COOL, 1: HEAT) voltage(150 mean 1.5)"},
								{"tec_set_auto_voltage", Cmd_tec_set_auto_voltage, " | format: tec_set_auto_voltage channel voltage (150 mean 1.5)"},
								{"tec_get_status", Cmd_tec_get_status, " | format: tec_get_status"},
								{"tec_log_ena", Cmd_TEC_log_enable, " | format: tec_log_ena"},
								{"tec_log_dis", Cmd_TEC_log_disable, " | format: tec_log_ena"},
								{"ringled_set_rgbw", Cmd_ringled_set_rgbw, " | format: ringled_set_rgbw <R> <G> <B> <W>"},
								{"ringled_get_rgbw", Cmd_ringled_get_rgbw, " | format: ringled_get_rgbw"},
								{"irled_set_bri", Cmd_IRled_set_bright, " | format: ir_led_set_bright <duty>"},
								{"irled_get_bri", Cmd_IRled_get_bright, " | format: ir_led_get_bright"},
								{"get_accel_gyro", Cmd_get_acceleration_gyroscope, " | format: get_accel_gyro"},
								{"get_press", Cmd_get_pressure, " | format: get_press"},
// 								{"CAN_transmit", Cmd_can_transmit, " | format: CAN_transmit"},
// 								{"CAN_status", Cmd_can_status, " | format: CAN_status"},
								{"get_all", Cmd_get_all, " | format: get_all"},
								{0,0,0}
								};

volatile static	ringbuffer_t *p_CommandRingBuffer;
volatile static	char s_commandBuffer[COMMAND_MAX_LENGTH];
static uint8_t	s_commandBufferIndex = 0;

static Command_TaskContextTypedef           s_CommandTaskContext =
{
	SCH_INVALID_TASK_HANDLE,                 // Will be updated by Schedular
	{
		SCH_TASK_SYNC,                      // taskType;
		SCH_TASK_PRIO_0,                    // taskPriority;
		10,                                // taskPeriodInMS;
		command_task_update                // taskFunction;
	}
};

static	void	command_task_update(void)
{
	char rxData;
	int8_t	ret_val;
	while (! rbuffer_empty(p_CommandRingBuffer))
	{
		rxData = rbuffer_remove(p_CommandRingBuffer);				
		usart0_send_char(rxData);
		if ((rxData == '\r') || (rxData == '\n'))		//got a return or new line
		{
			if (s_commandBufferIndex > 0)				//if we got the CR or LF at the begining, discard	
			{
				s_commandBuffer[s_commandBufferIndex] = 0;
				s_commandBufferIndex++;
				ret_val = CmdLineProcess((char *)s_commandBuffer);		
				s_commandBufferIndex = 0;		
				usart0_send_string(ErrorCode[ret_val]);
				usart0_send_string("> ");
			}	
			else usart0_send_string("\r\n> ");
		}
		else if ((rxData == 8) || (rxData == 127))	
		{
			if (s_commandBufferIndex > 0) s_commandBufferIndex--;
		}
		else
		{
			s_commandBuffer[s_commandBufferIndex] = rxData;
			s_commandBufferIndex ++;
			if (s_commandBufferIndex >= COMMAND_MAX_LENGTH)
			{
				s_commandBufferIndex= 0;
			}
		}
	}	
}

void	command_init(void)
{
	usart0_init();
	p_CommandRingBuffer = uart_get_uart0_rx_buffer_address();
	memset((void *)s_commandBuffer, 0, sizeof(s_commandBuffer));
	s_commandBufferIndex = 0;
	usart0_send_string("IOU FIRMWARE V1.0.0 \r\n");
	usart0_send_string("> ");
//	command_send_splash();
}

void	command_create_task(void)
{
	SCH_TASK_CreateTask(&s_CommandTaskContext.taskHandle, &s_CommandTaskContext.taskProperty);
}

int Cmd_help(int argc, char *argv[]) {
	tCmdLineEntry *pEntry;

	usart0_send_string("\nAvailable commands\r\n");
	usart0_send_string("------------------\r\n");

	// Point at the beginning of the command table.
	pEntry = &g_psCmdTable[0];

	// Enter a loop to read each entry from the command table.  The
	// end of the table has been reached when the command name is NULL.
	while (pEntry->pcCmd) {
		// Print the command name and the brief description.
		usart0_send_string(pEntry->pcCmd);
		usart0_send_string(pEntry->pcHelp);
		usart0_send_string("\r\n");

		// Advance to the next entry in the table.
		pEntry++;

	}
	// Return success.
	return (CMDLINE_OK);
}
//*****************************************************************************
//
// Format: set_temp channel value
//
//*****************************************************************************
int
Cmd_set_temp(int argc, char *argv[]) 
{
	if (argc < 3) return CMDLINE_TOO_FEW_ARGS;
	if (argc >3) return CMDLINE_TOO_MANY_ARGS;
	uint8_t channel = atoi(argv[1]);
	if (channel > 3 || channel < 0) return CMDLINE_INVALID_ARG;
	int16_t setpoint = atoi(argv[2]);
	temperature_set_point(setpoint, channel);
	UARTprintf("Setpoint[%d]:%i \r\n",channel, setpoint);
	return CMDLINE_OK;
}

//*****************************************************************************
//
// Format: get_temp NTC	channel, get_temp onewire channel, get_temp bmp390 channel
// Get the temperature from NTC / onewire / bmp390
//
//*****************************************************************************
int
Cmd_get_temp(int argc, char *argv[])
{
	int16_t temp;
	if (!strcmp(argv[1], "bmp390"))
	{
		if (argc >2) return CMDLINE_TOO_MANY_ARGS;
		temp = temperature_get_bmp390();
		if (temp == 0x7FFF) usart0_send_string("BMP390 is fail\r\n");
		else UARTprintf("BMP390 temp: %i \r\n", temp);
		return CMDLINE_OK;
	}
	else
	{
		if (argc < 3) return CMDLINE_TOO_FEW_ARGS;
		if (argc >3) return CMDLINE_TOO_MANY_ARGS;
		uint8_t channel = atoi(argv[2]);
		if (channel > (MAX_CHANNEL -1)) return CMDLINE_INVALID_ARG;
		if (!strcmp(argv[1], "NTC"))
		{
			temp = temperature_get_NTC(channel);
			if (temp == 0x7FFF) UARTprintf("NTC[%d] is fail\r\n", channel);
			else UARTprintf("NTC[%d]: %i\r\n", channel, temp);
		}
		else if (!strcmp(argv[1], "onewire"))
		{
			if (channel > 1) return CMDLINE_INVALID_ARG;
			temp = temperature_get_onewire(channel);
			if (temp == 0x7FFF) UARTprintf("1-Wire[%d] is fail\r\n", channel);
			else UARTprintf("1-Wire[%d]: %i\r\n", channel, temp);
		}
		else return CMDLINE_INVALID_ARG;
		return CMDLINE_OK;
	}
}
//*****************************************************************************
//
// Format: tec_set_auto_voltage channel auto_voltage
//
//*****************************************************************************
int
Cmd_tec_set_auto_voltage(int argc, char *argv[]) 
{
	if (argc < 3) return CMDLINE_TOO_FEW_ARGS;
	if (argc >3) return CMDLINE_TOO_MANY_ARGS;
	uint8_t channel = atoi(argv[1]);
	if (channel > 3) return CMDLINE_INVALID_ARG;
	uint16_t voltage = atoi(argv[2]);
	temperature_set_auto_voltage(channel, voltage);
	UARTprintf("Channel %d auto vol: %d \r\n",channel, voltage);
	return CMDLINE_OK;
}
//*****************************************************************************
//
// Format: TEC_ena channel 
//
//*****************************************************************************
int
Cmd_TEC_enable(int argc, char *argv[])
{
	if (argc < 2) return CMDLINE_TOO_FEW_ARGS;
	if (argc >2) return CMDLINE_TOO_MANY_ARGS;
	if (!strcmp(argv[1], "a"))
	{
		for (uint8_t udx=0; udx < 4; udx++)	 temperature_enable_TEC(udx);
		UARTprintf("enabled all channel\r\n");
		return CMDLINE_OK;
	}
	uint8_t channel = atoi(argv[1]);
	if (channel > 3) return CMDLINE_INVALID_ARG;
	temperature_enable_TEC(channel);
	return CMDLINE_OK;
}
//*****************************************************************************
//
// Format: TEC_dis channel 
//
//*****************************************************************************
int
Cmd_TEC_disable(int argc, char *argv[])
{
	if (argc < 2) return CMDLINE_TOO_FEW_ARGS;
	if (argc >2) return CMDLINE_TOO_MANY_ARGS;
	if (!strcmp(argv[1], "a"))
	{
		for (uint8_t udx=0; udx < 4; udx++)	 temperature_disable_TEC(udx);
		UARTprintf("disabled all channel\r\n");
		return CMDLINE_OK;
	}
	uint8_t channel = atoi(argv[1]);
	if (channel > 3) return CMDLINE_INVALID_ARG;
	temperature_disable_TEC(channel);
	return CMDLINE_OK;
}

//*****************************************************************************
//
// Format: TEC_ena_control channel 
//
//*****************************************************************************
int
Cmd_TEC_enable_auto_control(int argc, char *argv[])
{
	if (argc < 2) return CMDLINE_TOO_FEW_ARGS;
	if (argc >2) return CMDLINE_TOO_MANY_ARGS;
	uint8_t channel = atoi(argv[1]);
	if (channel > 3)	return CMDLINE_INVALID_ARG;
	temperature_enable_auto_control_TEC(channel);
	return CMDLINE_OK;
}

//*****************************************************************************
//
// Format: TEC_dis_control channel 
//
//*****************************************************************************
int
Cmd_TEC_disable_auto_control(int argc, char *argv[])
{
	if (argc < 2) return CMDLINE_TOO_FEW_ARGS;
	if (argc >2) return CMDLINE_TOO_MANY_ARGS;
	uint8_t channel = atoi(argv[1]);
	if (channel > 3)	return CMDLINE_INVALID_ARG;
	temperature_disable_auto_control_TEC(channel);
	return CMDLINE_OK;
}

int
Cmd_tec_get_status(int argc, char *argv[])
{
	if (argc >1) return CMDLINE_TOO_MANY_ARGS;
	temperature_get_status();
	return CMDLINE_OK;
}

int
Cmd_TEC_log_enable(int argc, char *argv[])
{
	if (argc >1) return CMDLINE_TOO_MANY_ARGS;
	temperature_enable_log();
	return CMDLINE_OK;
}

int
Cmd_TEC_log_disable(int argc, char *argv[])
{
	if (argc >1) return CMDLINE_TOO_MANY_ARGS;
	temperature_disable_log();
	return CMDLINE_OK;
}


//*****************************************************************************
//
// Format: tec_set_output channel heat_cool(0:COOL, 1: HEAT) voltage(150 mean 1.5)
// set the TEC output manually
//
//*****************************************************************************

int
Cmd_TEC_set_output(int argc, char *argv[])
{
	if (argc < 4) return CMDLINE_TOO_FEW_ARGS;
	if (argc >4) return CMDLINE_TOO_MANY_ARGS;
	uint8_t _heatCool = atoi(argv[2]);
	uint8_t _channel = atoi(argv[1]);
	uint16_t	_voltage = atoi(argv[3]);
	if (_channel >=4 )	return CMDLINE_INVALID_ARG;
	temperature_set_TEC_output(_channel, _heatCool, _voltage);
	return CMDLINE_OK;
}

int
Cmd_get_temp_setpoint(int argc, char *argv[])
{
	if (argc < 2) return CMDLINE_TOO_FEW_ARGS;
	if (argc >2) return CMDLINE_TOO_MANY_ARGS;
	uint8_t _channel = atoi(argv[1]);
	if (_channel > 3)	return CMDLINE_INVALID_ARG;
	int16_t _setpoint = temperature_get_setpoint(_channel);
	UARTprintf("Setpoint[%d]:%i \r\n", _channel, _setpoint);
	return CMDLINE_OK;
}

int
Cmd_ringled_set_rgbw(int argc, char *argv[])
{
	if (argc < 5) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 5) return CMDLINE_TOO_MANY_ARGS;
	uint8_t _red = atoi(argv[1]);
	uint8_t _green = atoi(argv[2]);
	uint8_t _blue = atoi(argv[3]);
	uint8_t _white = atoi(argv[4]);
	ringled_set_RGBW(_red, _green, _blue, _white);
	return CMDLINE_OK;
}

int
Cmd_ringled_get_rgbw(int argc, char *argv[])
{
	if (argc > 1) return CMDLINE_TOO_MANY_ARGS;
	rgbw_color RGB = ringled_get_RGBW();
	UARTprintf("Ringled: R=%d, G=%d, B=%d, W=%d \r\n", RGB.red, RGB.green, RGB.blue, RGB.white);
	return CMDLINE_OK;
}

int
Cmd_IRled_set_bright(int argc, char *argv[])
{
	if (argc < 2) return CMDLINE_TOO_FEW_ARGS;
	if (argc > 2) return CMDLINE_TOO_MANY_ARGS;
	uint8_t _DutyCyclePercent = atoi(argv[1]);
	if (_DutyCyclePercent > 100) _DutyCyclePercent = 100;
	IR_led_set_DutyCyclesPercent(_DutyCyclePercent);
	UARTprintf("IR set %s %%\r\n",argv[1]);
	return CMDLINE_OK;
}

int
Cmd_IRled_get_bright(int argc, char *argv[])
{
	if (argc > 1) return CMDLINE_TOO_MANY_ARGS;
	uint8_t Current_DutyCyclePercent = IR_led_get_Current_DutyCyclesPercent();
	UARTprintf("IR: %d %%\r\n",Current_DutyCyclePercent);
	return CMDLINE_OK;
}

int
Cmd_get_acceleration_gyroscope(int argc, char *argv[])
{
	if (argc > 1) return CMDLINE_TOO_MANY_ARGS;
	Accel_Gyro_DataTypedef _accel_data = get_acceleration();
	Accel_Gyro_DataTypedef _gyro_data = get_gyroscope();
	if(_accel_data.x == 0x7FFF || _accel_data.y == 0x7FFF || _accel_data.z == 0x7FFF || _gyro_data.x == 0x7FFF || _gyro_data.y == 0x7FFF || _gyro_data.z == 0x7FFF)
	{
		usart0_send_string("LSM6DSOX is fail\r\n");
		return CMDLINE_OK;
	}
	UARTprintf("Accel %i %i %i\r\n", _accel_data.x, _accel_data.y, _accel_data.z);
	UARTprintf("Gyro %i %i %i\r\n", _gyro_data.x, _gyro_data.y, _gyro_data.z);
	return CMDLINE_OK;
}

int
Cmd_get_pressure(int argc, char *argv[])
{
	if (argc > 1) return CMDLINE_TOO_MANY_ARGS;
	int16_t press = get_pressure();
	if (press == 0x7FFF) usart0_send_string("BMP390 is fail\r\n");
	else UARTprintf("Press: %i (10hPa)\r\n", press);
	return CMDLINE_OK;
}

int
Cmd_get_all(int argc, char *argv[])
{
	temperature_get_status();
	rgbw_color RGBW = ringled_get_RGBW();
	UARTprintf("RING: R=%d, G=%d, B=%d, W=%d\r\n", RGBW.red, RGBW.green, RGBW.blue, RGBW.white);
	UARTprintf("IR: %d %%\r\n", IR_led_get_Current_DutyCyclesPercent());
	
	// Accel and Gyro
	Accel_Gyro_DataTypedef _accel_data = get_acceleration();
	Accel_Gyro_DataTypedef _gyro_data = get_gyroscope();
	if (_accel_data.x == 0x7FFF || _accel_data.y == 0x7FFF || _accel_data.z == 0x7FFF || _gyro_data.x == 0x7FFF || _gyro_data.y == 0x7FFF || _gyro_data.z == 0x7FFF)
		usart0_send_string("LSM6DSOX is fail\r\n");
	else
	{
		UARTprintf("Accel %i %i %i\r\n", _accel_data.x, _accel_data.y, _accel_data.z);
		UARTprintf("Gyro %i %i %i\r\n", _gyro_data.x, _gyro_data.y, _gyro_data.z);
	}
	// Pressure
	int16_t press = get_pressure();
	if (press == 0x7FFF) usart0_send_string("BMP390 is fail\r\n");
	else UARTprintf("Press: %i (10hPa)\r\n", press);
	
	return CMDLINE_OK;
}

const char SPLASH[][65] PROGMEM = {
	{".......................................................\r\n"},
	{".......................................................\r\n"},
	{"..                                                   ..\r\n"},
	{"..                                                   ..\r\n"},
	{"..              _____                                ..\r\n"},
	{"..             / ____|                               ..\r\n"},
	{"..            | (___  _ __   __ _  ___ ___           ..\r\n"},
	{"..             \\___ \\| '_ \\ / _` |/ __/ _ \\          ..\r\n"},
	{"..             ____) | |_) | (_| | (_|  __/          ..\r\n"},
	{"..            |_____/| .__/ \\__,_|\\___\\___|          ..\r\n"},
	{"..         _      _ _| | _______        _            ..\r\n"},
	{"..        | |    (_|_)_||__   __|      | |           ..\r\n"},
	{"..        | |     _ _ _ __ | | ___  ___| |__         ..\r\n"},
	{"..        | |    | | | '_ \\| |/ _ \\/ __| '_ \\        ..\r\n"},
	{"..        | |____| | | | | | |  __/ (__| | | |       ..\r\n"},
	{"..        |______|_|_|_| |_|_|\\___|\\___|_| |_|       ..\r\n"},
	{"..                                                   ..\r\n"},
	{"..                                                   ..\r\n"},
	{"..    _____ ____  _    _  __    ____   ___   ___     ..\r\n"},
	{"..   |_   _/ __ \\| |  | | \\ \\  / /_ | / _ \\ / _ \\    ..\r\n"},
	{"..     | || |  | | |  | |  \\ \\/ / | || | | | | | |   ..\r\n"},
	{"..    _| || |__| | |__| |   \\  /  | || |_| | |_| |   ..\r\n"},
	{"..   |_____\\____/ \\____/     \\/   |_(_)___(_)___/    ..\r\n"},
	{"..                                                   ..\r\n"},
	{"..                                                   ..\r\n"},
	{".......................................................\r\n"},
	{".......................................................\r\n"},
	{"\r\n"}
};

void	command_send_splash(void)
{
	for(uint8_t i = 0 ; i < 28 ; i++) 
		usart0_send_string_P(&SPLASH[i][0]);
	usart0_send_string("> ");
}