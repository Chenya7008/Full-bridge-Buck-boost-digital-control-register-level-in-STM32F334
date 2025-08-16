
#include "OLED_SSD1306.h"
#include "Reg_i2c.h"
//static void OLED_Write_Byte(uint8_t WData, uint8_t cmd_or_data);
static void OLED_SetPos(uint8_t x, uint8_t y);
extern uint16_t temp;

 /*
 set the position of display begin
 according to the datasheet 
 following the page addressing mode	
 */
static void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_Write_Byte(0xb0 + y, CmdReg);               //page start address
    OLED_Write_Byte((x & 0xf0) >> 4 | 0x10, CmdReg); //upper start column 1 0000 - 1 1111
    OLED_Write_Byte((x & 0x0f) | 0x00, CmdReg);      //lower start column 0-1111
}

 //let the screen display the same 
	
void OLED_Fill(uint8_t Fill_Data)
{
    uint8_t i, j;
    for (i = 0; i < 8; i++) //8 pages
    {
        OLED_Write_Byte(0xb0 + i, CmdReg); 
        OLED_Write_Byte(0x10, CmdReg);     
        OLED_Write_Byte(0x00, CmdReg);    
        for (j = 0; j < OLED_Width; j++)   
            OLED_Write_Byte(Fill_Data, DataReg);
    }
}

//init function 
void OLED_Init(void)
{

    OLED_Delay_ms(200);
    OLED_Write_Byte(0xae, CmdReg); //display off
		//temp ++;
    OLED_Write_Byte(0x20, CmdReg); //Set Memory Addressing Mode
		//temp ++;
    OLED_Write_Byte(0x10, CmdReg); //10 page addressing
    OLED_Write_Byte(0x00, CmdReg); //set low column address
    OLED_Write_Byte(0x10, CmdReg); //set high column address
    OLED_Write_Byte(0x40, CmdReg); //set start line address
    OLED_Write_Byte(0xb0, CmdReg); // set page address
    OLED_Write_Byte(0x81, CmdReg); // set Contrast 
    OLED_Write_Byte(0xff, CmdReg); //set Contrast
		//normal display direction
    OLED_Write_Byte(0xc0, CmdReg); //Com scan direction
    OLED_Write_Byte(0xa0, CmdReg); //set segment remap
	
    OLED_Write_Byte(0xa6, CmdReg); //normal display
    OLED_Write_Byte(0xa8, CmdReg); //set multiplex ratio(1 to 64)
    OLED_Write_Byte(0x3f, CmdReg); 
    OLED_Write_Byte(0xd3, CmdReg); //set display offset
    OLED_Write_Byte(0x00, CmdReg); //
    OLED_Write_Byte(0xd5, CmdReg); // set osc division
    OLED_Write_Byte(0x80, CmdReg); //
    OLED_Write_Byte(0xd8, CmdReg); //set area color mode off
    OLED_Write_Byte(0x05, CmdReg); //
    OLED_Write_Byte(0xd9, CmdReg); //Set Pre-Charge Period
    OLED_Write_Byte(0xf1, CmdReg); //
    OLED_Write_Byte(0xda, CmdReg); //set com pin configuartion
    OLED_Write_Byte(0x12, CmdReg); //
    OLED_Write_Byte(0xdb, CmdReg); //vcomh set Vcomh
    OLED_Write_Byte(0x30, CmdReg); //
    OLED_Write_Byte(0x8d, CmdReg); //set charge pump enable
    OLED_Write_Byte(0x14, CmdReg); //
    OLED_Write_Byte(0xa4, CmdReg); //bit0，1开启，0关闭(白屏/黑屏)

    OLED_Write_Byte(0xa6, CmdReg); //normal display

    OLED_Write_Byte(0xaf, CmdReg); // turn on oled panel
    OLED_Fill(Dark);               //clear the screen
    OLED_Delay_ms(100);
		//temp ++;
}

//show char in certain position
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t DisplayChar, uint8_t FontSize, uint8_t Color_Turn)
{
    uint8_t c = 0, i;
    c = DisplayChar - 32; // char index in front library 
    if (x > 128 - 1)
    {
        x = 0;
        y += 2;
    }
    switch (FontSize)
    {
    case 1:
        OLED_SetPos(x, y); 
        for (i = 0; i < 6; i++)
        {
            if (Color_Turn)
                OLED_Write_Byte(~F6X8[c * 6 + i], DataReg); //show 6x8 char page align i-> col
            else
                OLED_Write_Byte(F6X8[c * 6 + i], DataReg); //show 6x8 char
        }
        break;
    case 2:
        OLED_SetPos(x, y);     
        for (i = 0; i < 8; i++) //upper part of char
        {
            if (Color_Turn)
                OLED_Write_Byte(~F8X16[c * 16 + i], DataReg); //show the first 8 data of char
            else
                OLED_Write_Byte(F8X16[c * 16 + i], DataReg); //show the first 8 data of char
        }
        OLED_SetPos(x, y + 1);  //switch to next page
        for (i = 0; i < 8; i++) //lower part of char
        {
            if (Color_Turn)
                OLED_Write_Byte(~F8X16[c * 16 + i + 8], DataReg); //show the next 8 data of char
            else
                OLED_Write_Byte(F8X16[c * 16 + i + 8], DataReg); //show the next 8 data of char
        }
        break;
    }
}

// show string in cartain postion 
void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t *DisplayStr, uint8_t FontSize, uint8_t Color_Turn)
{
    uint8_t j = 0;
    while (DisplayStr[j] != '\0') //whether string display complete
    {
        OLED_ShowChar(x, y, DisplayStr[j], FontSize, Color_Turn);
        if (FontSize == 1) //6X8 char to show next char
            x += 6;
        else if (FontSize == 2) 
            x += 8;

        if (x > 122 && FontSize == 1)  //switch to next page
        {
            x = 0;
            y++;
        }
        if (x > 120 && FontSize == 2)  
        {
            x = 0;
            y++;
        }
        j++;
    }
}

uint32_t oled_pow(uint8_t m,uint8_t n)
{
	uint32_t result = 1;	 
	while(n--) result *= m;    
	return result;
}	


void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;	//using to switch state					   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1)) 
		{
			if(temp==0)//find the zero in front of number
			{
				OLED_ShowChar(x+(size/2)*t,y,' ', FontSize6x8, 0);//display space
				continue;
			}else enshow=1;  //meet the none zero ,start display
		 	 
		}
	 	OLED_ShowChar(x+(size/2)*t,y,temp+'0', FontSize6x8, 0); 
	}
	//without enshow THE mid zero in number will be replace as space
} 
//reserve the zero in front of number
void OLED_ShowNum_0(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size/2)*t,y,'0', FontSize6x8, 0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size/2)*t,y,temp+'0', FontSize6x8, 0); 
	}
} 
