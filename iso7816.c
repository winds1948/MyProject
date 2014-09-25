/**************************************************************************
Filename : iso7816.c
Language : C source file
Description : The contents of the base functions defined in iso7816.h  and the data of atr
Author(s) :	liuhm   
Company  : AisinoChip Ltd.
version  : 1.0
Change Log : 2010-6-22 13:29, first edition
******************************************************************************/

/**********************************************************
*	include files
**********************************************************/
#include "common.h"
#include "iso7816.h"
/**********************************************************
*	structure
**********************************************************/

/**********************************************************
*	global variables
**********************************************************/
UINT8 SCI_rx_Buff[128];
UINT8 SCI_rx_Len = 0;
int SCIM_rx_flag = 0;
int SCIM_tx_flag = 0;
extern UINT8 IC_CARD_POW_STA;

/**********************************************************
*	functions
**********************************************************/

/**********************************************************
* ISO7816 MASTER	functions
**********************************************************/

void SCIM_IO_FALL_IRQHandler(void) __irq
{
//	uart_printf("sci7816 SCIM_IO_FALL_IRQHandler!\n");
	REG_7816M_INTIO1 |= 0x01;     //clear the IO PAD1 falling edge interrupt flag 
    while (((REG_7816M_INTIO1) & 0x01) != 0);                   
}

void SCIM_rx_IRQHandler(void) __irq
{
	//uart_printf("sci7816 SCIM_rx_IRQHandler!\n"); 
	REG_7816M_INTIO1 |= 0x02;                  //clear the receive interrupt flag 
	while ((REG_7816M_INTIO1 &0x02) != 0x0);
	SCI_rx_Buff[SCI_rx_Len++] = REG_7816M_BUFHW;             //read the received data from register SCIBUFHW
	SCIM_rx_flag = 1;
	//uart_printf("sci7816 receive data is %x!\n",BUFFER); 
	                
}

void SCIM_tx_IRQHandler(void) __irq
{
 	//SCIBUFHW = BUFFER;             //more byte data to be transmitted into register SCIBUFHW
	SCIM_tx_flag = 1;
	REG_7816M_INTIO1 |= 0x04;                  //clear the transmit interrupt flag 
	while ((REG_7816M_INTIO1 &0x04) != 0x0);	
}

void SCIM_rst_IRQHandler(void) __irq
{
	//uart_printf("sci7816 SCIM_rst_IRQHandler!\n");
	REG_7816M_INTRST = 0x01;                  //clear the reset interrupt flag 
	while (((REG_7816M_INTRST) & 0x01) != 0);
}

/*******************************************************************************
* Function Name  : SCIM_t0_tx_single
* Description    : Transmits signle data through the SCI7816M.
* Input          : - tx_data: the data to transmit.
* Output         : None
* Return         : None
*******************************************************************************/
void SCIM_t0_tx_single(UINT8 tx_data)
{
	//check tx fifo is full or not
	while (REG_7816M_STAT & 0x10)
	{
		if(IC_CARD_POW_STA  == 0) return ;
		//uart_printf("1");
	}
	//write data to SCIBUFHW
	REG_7816M_BUFHW = tx_data;
	//check retry error flag
	while (REG_7816M_STAT & 0x20)
	{
		if(IC_CARD_POW_STA  == 0) return ;
		//uart_printf("2");

	}
}

/*******************************************************************************
* Function Name  : SCIM_t0_tx_poll
* Description    : Transmits data through the SCI7816M.
* Input          : - tx_data: the data to transmit.
*                  - length: the length of data
* Output         : None
* Return         : None
*******************************************************************************/
void SCIM_t0_tx_poll(UINT8 tx_data[], UINT8 length)
{
	UINT8 i;

	for (i=0; i<length; i++)
	{
		SCIM_t0_tx_single(tx_data[i]);	
	}	
	while (!(REG_7816M_INTIO1&0x04))
	{
		if(IC_CARD_POW_STA  == 0) return ;
	};
	while ((REG_7816M_INTIO1&0x04))
	{
		REG_7816M_INTIO1 |= 0x04;
		if(IC_CARD_POW_STA  == 0) return ;
	}	
}

/*******************************************************************************
* Function Name  : SCIM_t0_rx_single
* Description    : Receive signle data through the SCI7816M.
* Input          : None
* Output         : None
* Return         : the received data
*******************************************************************************/
UINT8 SCIM_t0_rx_single(void)
{
	UINT8 temp = 0;

	while (!(REG_7816M_STAT & 0x01))
	{
		if( REG_7816M_STAT & 0x80)
			return 0;
		if(IC_CARD_POW_STA  == 0) return 0;
		

	}
	temp = REG_7816M_BUFHW; 
	return temp;	
}

/*******************************************************************************
* Function Name  : SCIM_t0_rx_poll
* Description    : Receives data through the SCI7816M.
* Input          : - rx_data: the address of received data.
*                  - length: the length of data
* Output         : None
* Return         : None
*******************************************************************************/
UINT8 SCIM_t0_rx_poll(UINT8 *rx_data, UINT8 length)
{
	UINT8 i;
	for (i=0; i<length; i++)
	{
		rx_data[i] = SCIM_t0_rx_single();
	}
	return i;	
}

/*******************************************************************************
* Function Name  : SCIM_reader_initial
* Description    : initial sci7816 master module.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SCIM_reader_initial(void)
{


#if	T==1
	   REG_7816M_MODHW = 0x69;
#else
  	   REG_7816M_MODHW = 0x70;
#endif
	
	//configure SCICTRL register,select PAD1 as H/W IO
	REG_7816M_CTRL = 0x0a;
	//set baud rate  
	REG_7816M_ETUDATA = 0x174;
	//enable CWT
	REG_7816M_WTCTRL = 0x02;
	//clear interrupt
	while ((REG_7816M_INTRST) & 0x01)
	{
		REG_7816M_INTRST = 0x01;
       }
	while ((REG_7816M_INTIO1) & 0x07)
	{
		REG_7816M_INTIO1 = 0x07;
       }

}
//7816 soft reset
void SCIM_rst(void)
{
	 //7816 master give a reset signal
	 REG_SYS_RCR |= 0x20000000;	
	 delay(10); 	 	 
	 REG_SYS_RCR &= (~0x20000000);
	 	 
	//clear tx and rx fifo
	 if(REG_7816M_STAT & 0x09)
	 {
		REG_7816M_CTRL |= 0x30;
		REG_7816M_CTRL &= 0xcf;	
     }
} 
void SCIM_clk(void)
{
	//set SCIM 7816clk 4M to slave
	REG_SYS_DIV = 0x800b0000;//sysclk = 48M
	delay(10);
}
void SCIM_disable_clk(void)
{
	//set SCIM 7816clk 4M to slave
	REG_SYS_DIV &= 0x7fffffff;//sysclk = 48M
	delay(10);
}
/*******************************************************************************
* Function Name  : SCIM_atr_rec
* Description    : receive the data of ATR from card,and set the CONV bit according to the TS byte
* Input          : - data_rec: the pointer of data buffer to store receive data.
* Output         : None
* Return         : return atr length if success, else return 0
*******************************************************************************/
//receive the data of ATR from TS and store in the parameter_1
//parameter_2 is the buffer length which is enough to store the ATR
//return atr length if success, else return 0
//meantime we set the CONV bit in UCICR1 according to the TS byte here
UINT8 SCIM_atr_rec(UINT8 * data_rec)
{
	UINT8 td,t0;
	UINT8 i = 0;
	UINT8 flag = 1;
	UINT8 l;
	UINT8 tck_flag = 0;

	//set the cwt_en bit 
	REG_7816M_WTCTRL |=0x02;	
	data_rec[i++] = SCIM_t0_rx_single();
	while(flag)
	{
		td = SCIM_t0_rx_single();
		//if only T=0 is indicated, TCK shall not be sent
		if((i != 1) && ((td & 0x0f) != 0x00))
			tck_flag = 1;
			
		data_rec[i] = td;
		i++;

		if((td & 0x10) == 0x10)
		{
			data_rec[i] = SCIM_t0_rx_single();
			i++;
		}

		if((td & 0x20) == 0x20)
		{
			data_rec[i] = SCIM_t0_rx_single();
			i++;
		}

		if((td & 0x40) == 0x40)
		{
			data_rec[i] = SCIM_t0_rx_single();
			i++;
		}

		if((td & 0x80) == 0x80)
			flag = 1;
		else
			flag = 0;
	}

   	t0 = data_rec[1];
	l = (t0 & 0x0f) + i;
	for(;i<l;i++)
	data_rec[i] = SCIM_t0_rx_single();//story bytes
	
	if(tck_flag == 1)   // TCK exist
		data_rec[i++] = SCIM_t0_rx_single();//TCK bytes

	//clear the WTEN bit 
	REG_7816M_WTCTRL &=0x01;
	
	//if((SCI_wto_flag == 1) || (SCI_pf_flag == 1))
	//	return 0;
		
	if( data_rec[0] == 0x3b)//direct convention
		REG_7816M_MODHW &= (~SCI_CONV);

	else if( data_rec[0] == 0x3f)//inverse convention
		REG_7816M_MODHW |= SCI_CONV;

	else 	return 0;

	return i;
}

/*******************************************************************************
* Function Name  : SCIM_change_baud_rate
* Description    : change baud rate according to para of PPS
* Input          : - para: the parameter of change baud rate
* Output         : None
* Return         : None
*******************************************************************************/
void SCIM_change_baud_rate(UINT8 para)
{
	switch (para)
	{
		case 0x11: 				//	3.57M  9600bps
		{
			REG_7816M_ETUDATA =0x174; 
			break;			
		}
		case 0x12: 				//	3.57M  19200bps
		{
			REG_7816M_ETUDATA =0xBA;
			break;			
		}
		case 0x13: 				//	3.57M  38400bps
		{
			REG_7816M_ETUDATA =0x5D;
			break;			
		}
		case 0x18: 				//	3.57M  115200bps
		{
			REG_7816M_ETUDATA =0x1f;
			break;			
		}

		default:
		{
			REG_7816M_ETUDATA =0x174; 
			break;
		}
	}
}


UINT16 SCIM_tpdu_data_sent_T0(UINT8 CLA, UINT8 INS, UINT8 P1, UINT8 P2, UINT8 P3,UINT8 data[])
{
	UINT8 temp_s[1],temp_r[1];
	UINT8 cmd[5];
	UINT8 sw[2];
	UINT8 data_temp[300];
	UINT8 i = 0;
	UINT8 j;
	UINT8 lc = P3;

	cmd[0] = CLA;
	cmd[1] = INS;
	cmd[2] = P1;
	cmd[3] = P2;
	cmd[4] = P3;

	SCIM_t0_tx_poll(cmd,5);
	
	if(lc == 0)
		goto L2;
L1:	
	SCIM_t0_rx_poll(temp_r,1);
	if(temp_r[0] == 0x60)
		goto L1;
	if(((temp_r[0] & 0xF0) == 0x60)||((temp_r[0] & 0xF0) == 0x90))
	{
		sw[0] = temp_r[0];
		SCIM_t0_rx_poll(temp_r,1);
		sw[1] = temp_r[0];
		goto L3;
	}
	if(((temp_r[0] ^ INS) == 0xff)||((temp_r[0] ^ INS) == 0xfe))
	{
		temp_s[0] = data[i];
		i++;		
		SCIM_t0_tx_poll(temp_s,1);
		if(i == lc)
			goto L2;
		else
			goto L1;		
	}
	if(((temp_r[0] ^ INS) == 0x00)||((temp_r[0] ^ INS) == 0x01))
	{
		for(j=0;j<lc-i;j++)
			data_temp[j] = data[i+j];
		delay(500);
		SCIM_t0_tx_poll(data_temp,j);

L4:		
		SCIM_t0_rx_poll(temp_r,1);
		if(temp_r[0] == 0x60)
			goto L4;

		sw[0] = temp_r[0];
		SCIM_t0_rx_poll(temp_r,1);
		sw[1] = temp_r[0];
		goto L3;
			
	}
		
L2:
	SCIM_t0_rx_poll(sw,2);
L3:	return sw[0]* 256 + sw[1];
}

UINT16 SCIM_tpdu_data_rec_T0(UINT8 CLA, UINT8 INS, UINT8 P1, UINT8 P2, UINT8 P3,UINT8 data[])
{
	UINT8 temp_r[1];
	UINT8 cmd[5];
	UINT8 sw[2];
	UINT8 data_temp[300];
	UINT8 i = 0;
	UINT8 j;
	UINT8 le = (P3 == 0)? 256:P3;

	cmd[0] = CLA;
	cmd[1] = INS;
	cmd[2] = P1;
	cmd[3] = P2;
	cmd[4] = P3;

	SCIM_t0_tx_poll(cmd,5);
	if(le == 0)
		goto L2;
L1:	
	SCIM_t0_rx_poll(temp_r,1);
	if(temp_r[0] == 0x60)
		goto L1;
	if(((temp_r[0] & 0xF0) == 0x60)||((temp_r[0] & 0xF0) == 0x90))
	{
		sw[0] = temp_r[0];
		SCIM_t0_rx_poll(temp_r,1);
		sw[1] = temp_r[0];
		
		goto L3;
	}
	if(((temp_r[0] ^ INS) == 0xff)||((temp_r[0] ^ INS) == 0xfe))
	{				
		SCIM_t0_rx_poll(temp_r,1);
		data[i] = temp_r[0];
		i++;
		if(i == le)
			goto L2;
		else
			goto L1;		
	}
	if(((temp_r[0] ^ INS) == 0x00)||((temp_r[0] ^ INS) == 0x01))
	{		
		SCIM_t0_rx_poll(data_temp,le-i);
		for(j=i;j<le;j++)
			data[j] = data_temp[j-i];
			
		/*SCIM_t0_rx_poll(data_temp,le);
		for(j=0;j<le;j++)
			data[j] = data_temp[j];*/
		
	}


		
L2:
	SCIM_t0_rx_poll(sw,2);
L3:	return sw[0]* 256 + sw[1];

}

