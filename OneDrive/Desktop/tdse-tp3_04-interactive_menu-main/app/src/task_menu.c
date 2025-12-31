/*
 * Copyright (c) 2023 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file   : task_menu.c
 * @date   : Set 26, 2023
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "task_menu_attribute.h"
#include "task_menu_interface.h"
#include "display.h"

/********************** macros and definitions *******************************/
#define G_TASK_MEN_CNT_INI			0ul
#define G_TASK_MEN_TICK_CNT_INI		0ul

#define DEL_MEN_XX_MIN				0ul
#define DEL_MEN_XX_MED				50ul
#define DEL_MEN_XX_MAX				500ul

#define MAX_MOTORS					2 -1
#define MAX_VAR						3 -1
#define MAX_POWER					1
#define MAX_SPEED					9
#define MAX_SPIN					1


/********************** internal data declaration ****************************/
task_menu_dta_t task_menu_dta =
	{DEL_MEN_XX_MIN, ST_ACT_MENU_0, EV_MEN_ENT_IDLE, false, 0, 0, 0};

motor_info_t motor_info_list[] = {
	{false, 0, false},
	{false, 0, false}
};

//int motor_info_list[MAX_MOTORS][3] = {{0, 0, 0}, {0, 0, 0}};


const int MAX_VAL[] = {MAX_POWER, MAX_SPEED, MAX_SPIN};
const char* var_names[] = {"Power", "Speed", "Spin"};
const char* val_names[][10] = {{"OFF", "ON"}, {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"}, {"L", "R"}};

#define MENU_DTA_QTY	(sizeof(task_menu_dta)/sizeof(task_menu_dta_t))

/********************** internal functions declaration ***********************/
void task_menu_statechart(void);
void LCD_show(const char * first_row, const char * second_row);

/********************** internal data definition *****************************/
const char *p_task_menu 		= "Task Menu (Interactive Menu)";
const char *p_task_menu_ 		= "Non-Blocking & Update By Time Code";


/********************** external data declaration ****************************/
uint32_t g_task_menu_cnt;
volatile uint32_t g_task_menu_tick_cnt;

/********************** external functions definition ************************/

void LCD_show(const char * first_row, const char * second_row){
	displayCharPositionWrite(0, 0);
	displayStringWrite("                    ");
	displayCharPositionWrite(0, 1);
	displayStringWrite("                    ");
	displayCharPositionWrite(0, 0);
	displayStringWrite(first_row);
	displayCharPositionWrite(0, 1);
	displayStringWrite(second_row);
}

void task_menu_init(void *parameters)
{
	task_menu_dta_t *p_task_menu_dta;
	task_menu_st_t	state;
	task_menu_ev_t	event;
	bool b_event;

	char first_row[20];
	char second_row[20];
	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - %s", GET_NAME(task_menu_init), p_task_menu);
	LOGGER_INFO("  %s is a %s", GET_NAME(task_menu), p_task_menu_);

	/* Init & Print out: Task execution counter */
	g_task_menu_cnt = G_TASK_MEN_CNT_INI;
	LOGGER_INFO("   %s = %lu", GET_NAME(g_task_menu_cnt), g_task_menu_cnt);

	init_queue_event_task_menu();

	/* Update Task Actuator Configuration & Data Pointer */
	p_task_menu_dta = &task_menu_dta;

	/* Init & Print out: Task execution FSM */
	state = ST_ACT_MENU_0;
	p_task_menu_dta->state = state;

	event = EV_MEN_ENT_IDLE;
	p_task_menu_dta->event = event;

	b_event = false;
	p_task_menu_dta->flag = b_event;

	LOGGER_INFO(" ");
	LOGGER_INFO("   %s = %lu   %s = %lu   %s = %s",
				 GET_NAME(state), (uint32_t)state,
				 GET_NAME(event), (uint32_t)event,
				 GET_NAME(b_event), (b_event ? "true" : "false"));

	/* Init & Print out: LCD Display */
	displayInit( DISPLAY_CONNECTION_GPIO_4BITS );
	snprintf(first_row, sizeof(first_row), "Motor 0: %s, %s, %s",
			val_names[0][motor_info_list[0].power],
			val_names[1][motor_info_list[0].speed],
			val_names[2][motor_info_list[0].spin]);
	snprintf(second_row, sizeof(second_row), "Motor 1: %s, %s, %s",
			val_names[0][motor_info_list[1].power],
			val_names[1][motor_info_list[1].speed],
			val_names[2][motor_info_list[1].spin]);
	LCD_show(first_row, second_row);
}

void task_menu_update(void *parameters)
{
	bool b_time_update_required = false;

	/* Protect shared resource */
	__asm("CPSID i");	/* disable interrupts */
    if (G_TASK_MEN_TICK_CNT_INI < g_task_menu_tick_cnt)
    {
		/* Update Tick Counter */
    	g_task_menu_tick_cnt--;
    	b_time_update_required = true;
    }
    __asm("CPSIE i");	/* enable interrupts */

    while (b_time_update_required)
    {
		/* Update Task Counter */
		g_task_menu_cnt++;

		/* Run Task Menu Statechart */
    	task_menu_statechart();

    	/* Protect shared resource */
		__asm("CPSID i");	/* disable interrupts */
		if (G_TASK_MEN_TICK_CNT_INI < g_task_menu_tick_cnt)
		{
			/* Update Tick Counter */
			g_task_menu_tick_cnt--;
			b_time_update_required = true;
		}
		else
		{
			b_time_update_required = false;
		}
		__asm("CPSIE i");	/* enable interrupts */
	}
}

void task_menu_statechart(void)
{
	task_menu_dta_t *p_task_menu_dta;
	char first_row[20];
	char second_row[20];

    /* Update Task Menu Data Pointer */
	p_task_menu_dta = &task_menu_dta;

	if (true == any_event_task_menu())
	{
		p_task_menu_dta->flag = true;
		p_task_menu_dta->event = get_event_task_menu();
	}

	switch (p_task_menu_dta->state)
	{
		case ST_ACT_MENU_0:

			if ((true == p_task_menu_dta->flag) && (EV_MEN_ENT_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_1;
				snprintf(second_row, sizeof(second_row), "> %lu", p_task_menu_dta->curr_motor);
				LCD_show("Select Motor:", second_row);
			}
			break;

		case ST_ACT_MENU_1:
			if ((true == p_task_menu_dta->flag) && (EV_MEN_ENT_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_2;
				snprintf(first_row, sizeof(first_row), "Config Motor: %lu", p_task_menu_dta->curr_motor);
				snprintf(second_row, sizeof(second_row), "> %s", var_names[p_task_menu_dta->curr_var]);
				LCD_show(first_row, second_row);

			}
			else if ((true == p_task_menu_dta->flag) && (EV_MEN_NEX_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				if(p_task_menu_dta->curr_motor < MAX_MOTORS){p_task_menu_dta->curr_motor++;}
				else if (p_task_menu_dta->curr_motor == MAX_MOTORS){p_task_menu_dta->curr_motor=0;}
				snprintf(second_row, sizeof(second_row), "> %lu", p_task_menu_dta->curr_motor);
				LCD_show("Select Motor:", second_row);

			}
			else if ((true == p_task_menu_dta->flag) && (EV_MEN_ESC_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_0;
				snprintf(first_row, sizeof(first_row), "Motor 0: %s, %s, %s",
						val_names[0][motor_info_list[0].power],
						val_names[1][motor_info_list[0].speed],
						val_names[2][motor_info_list[0].spin]);
				snprintf(second_row, sizeof(second_row), "Motor 1: %s, %s, %s",
						val_names[0][motor_info_list[1].power],
						val_names[1][motor_info_list[1].speed],
						val_names[2][motor_info_list[1].spin]);
				LCD_show(first_row, second_row);
			}

			break;

		case ST_ACT_MENU_2:
			if ((true == p_task_menu_dta->flag) && (EV_MEN_ENT_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_3;

				if (0 == p_task_menu_dta->curr_var){p_task_menu_dta->curr_val = motor_info_list[p_task_menu_dta->curr_motor].power;}
				if (1 == p_task_menu_dta->curr_var){p_task_menu_dta->curr_val = motor_info_list[p_task_menu_dta->curr_motor].speed;}
				if (2 == p_task_menu_dta->curr_var){p_task_menu_dta->curr_val = motor_info_list[p_task_menu_dta->curr_motor].spin;}

				snprintf(first_row, sizeof(first_row), "Set %s", var_names[p_task_menu_dta->curr_var]);
				snprintf(second_row, sizeof(second_row), "> %s", val_names[p_task_menu_dta->curr_var][p_task_menu_dta->curr_val]);
				LCD_show(first_row, second_row);

			}
			else if ((true == p_task_menu_dta->flag) && (EV_MEN_NEX_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				if(p_task_menu_dta->curr_var < MAX_VAR){p_task_menu_dta->curr_var++;}
				else if (p_task_menu_dta->curr_var == MAX_VAR){p_task_menu_dta->curr_var=0;}
				snprintf(second_row, sizeof(second_row), "> %s", var_names[p_task_menu_dta->curr_var]);
				LCD_show("Select Variable:", second_row);
			}
			else if ((true == p_task_menu_dta->flag) && (EV_MEN_ESC_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_1;
				snprintf(second_row, sizeof(second_row), "> %lu", p_task_menu_dta->curr_motor);
				LCD_show("Select Motor:", second_row);
			}
			break;
		case ST_ACT_MENU_3:
			if ((true == p_task_menu_dta->flag) && (EV_MEN_ENT_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_2;

				if (0 == p_task_menu_dta->curr_var){motor_info_list[p_task_menu_dta->curr_motor].power = p_task_menu_dta->curr_val;}
				if (1 == p_task_menu_dta->curr_var){motor_info_list[p_task_menu_dta->curr_motor].speed = p_task_menu_dta->curr_val;}
				if (2 == p_task_menu_dta->curr_var){motor_info_list[p_task_menu_dta->curr_motor].spin = p_task_menu_dta->curr_val;}

				snprintf(second_row, sizeof(second_row), "> %s", var_names[p_task_menu_dta->curr_var]);
				LCD_show("Select Variable:", second_row);

			}
			else if ((true == p_task_menu_dta->flag) && (EV_MEN_NEX_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				if(p_task_menu_dta->curr_val < MAX_VAL[p_task_menu_dta->curr_var]){p_task_menu_dta->curr_val++;}
				else if (p_task_menu_dta->curr_val == MAX_VAL[p_task_menu_dta->curr_var]){p_task_menu_dta->curr_val=0;}
				snprintf(first_row, sizeof(first_row), "Set %s", var_names[p_task_menu_dta->curr_var]);
				snprintf(second_row, sizeof(second_row), "> %s", val_names[p_task_menu_dta->curr_var][p_task_menu_dta->curr_val]);
				LCD_show(first_row, second_row);
			}
			else if ((true == p_task_menu_dta->flag) && (EV_MEN_ESC_ACTIVE == p_task_menu_dta->event))
			{
				p_task_menu_dta->flag = false;
				p_task_menu_dta->state = ST_ACT_MENU_2;
				snprintf(second_row, sizeof(second_row), "> %s", var_names[p_task_menu_dta->curr_var]);
				LCD_show("Select Variable:", second_row);
			}
			break;

		default:
			break;
	}
}

/********************** end of file ******************************************/
