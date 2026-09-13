#include <stdint.h>
#include <stdio.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up

uint16_t distance[10][32]; //create a 2D array which is used to store the diatance data measured



void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//initialize Port H
void PortH_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R7) == 0){}

    GPIO_PORTH_DIR_R |= 0x0F;
    GPIO_PORTH_AFSEL_R &= ~0x0F;
    GPIO_PORTH_DEN_R |= 0x0F;
    GPIO_PORTH_AMSEL_R &= ~0x0F;
    GPIO_PORTH_DATA_R &= ~0x0F;
}

//initialize Port N
void PortN_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;			
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	
	GPIO_PORTN_DIR_R |= 0x03;        							
  GPIO_PORTN_AFSEL_R &= ~0x03;     							
  GPIO_PORTN_DEN_R |= 0x03;        								
																									
  GPIO_PORTN_AMSEL_R &= ~0x03;     							
	return;
}

//initialize Port M
void PortM_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;			
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};	
	GPIO_PORTM_DIR_R |= 0x01;        							
  GPIO_PORTM_AFSEL_R &= ~0x01;     							
  GPIO_PORTM_DEN_R |= 0x01;        								
																									
  GPIO_PORTM_AMSEL_R &= ~0x01;     							
	return;
}

//initialize Port J
void PortJ_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;			
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};	
	GPIO_PORTJ_DIR_R &= ~0x03;        							
  GPIO_PORTJ_AFSEL_R &= ~0x03;     								
  GPIO_PORTJ_DEN_R |= 0x03;   		
	GPIO_PORTJ_PUR_R |= 0x03;
																								
  GPIO_PORTJ_AMSEL_R &= ~0x03;     								
	return;
}

//initialize Port F
void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;				
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};	
	GPIO_PORTF_DIR_R |= 0x11;        								
  GPIO_PORTF_AFSEL_R &= ~0x11;     								
  GPIO_PORTF_DEN_R |= 0x11;        								
																									
  GPIO_PORTF_AMSEL_R &= ~0x11;     								
	return;
}

//flash the on-board LED D4 when a measurement take place
void Measurement_status_LED_Flash(void)
{
	GPIO_PORTF_DATA_R |= 0x01; //turn on LED D4 by set PF0 to 1
  SysTick_Wait10ms(1); //have a 10ms delay to simulate a flash
  GPIO_PORTF_DATA_R &= ~0x01; //turn off the LED D4 by set PF0 to 0
}

//flash the on-board LED D3 when a UART transmission take place
void UART_LED_Flash(void)
{
	GPIO_PORTF_DATA_R |= 0x10; //turn on LED D3 by set PF4 to 1
  SysTick_Wait10ms(1); //have a 10ms delay to simulate a flash
  GPIO_PORTF_DATA_R &= ~0x10; //turn off the LED D3 by set PF4 to 0
}

//function that turn on the on-board LED D1
void Additional_status_LED_on(void)
{
	GPIO_PORTN_DATA_R |= 0x02; //turn on LED D1 by set PN1 to 1.
}

//function that turn off the on-board LED D1
void Additional_status_LED_off(void)
{
	GPIO_PORTN_DATA_R &= ~0x02; //turn off LED D1 by set PN1 to 0
}

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}

//function that contains a sequence of 4 steps that will lead to clockwise rotation
void stepper_cw()
{
	GPIO_PORTH_DATA_R = 0b00000011; //set PH3 - PH0 to 0011
	SysTick_Wait10ms(1);	//have a 10ms delay between each step										
	GPIO_PORTH_DATA_R = 0b00000110;	//set PH3 - PH0 to 0110											
	SysTick_Wait10ms(1);  //have a 10ms delay between each step		
	GPIO_PORTH_DATA_R = 0b00001100;	//set PH3 - PH0 to 1100												
	SysTick_Wait10ms(1);  //have a 10ms delay between each step		
	GPIO_PORTH_DATA_R = 0b00001001;	//set PH3 - PH0 to 1001												
	SysTick_Wait10ms(1);  //have a 10ms delay between each step		
}

//function that contains a sequence of 4 steps that will lead to counterclockwise rotation
void stepper_ccw()
{
	GPIO_PORTH_DATA_R = 0b00001001;  //set PH3 - PH0 to 1001	
	SysTick_Wait10ms(1);	//have a 10ms delay between each step												
	GPIO_PORTH_DATA_R = 0b00001100;	 //set PH3 - PH0 to 1100											
	SysTick_Wait10ms(1);  //have a 10ms delay between each step		
	GPIO_PORTH_DATA_R = 0b00000110;  //set PH3 - PH0 to 0110													
	SysTick_Wait10ms(1);  //have a 10ms delay between each step		
	GPIO_PORTH_DATA_R = 0b00000011;	 //set PH3 - PH0 to 0011													
	SysTick_Wait10ms(1);  //have a 10ms delay between each step		
}

uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

uint16_t Get_Distance(void)
{
		uint8_t dataReady = 0; //variable to check whether new data is ready
    uint8_t RangeStatus = 0; //variable to check whether the data is valid
    uint16_t Distance = 0; //variable to get the measured distance
    int timeout = 0; //variable of a time count

    while((dataReady == 0) && (timeout < 200)) //keep checking whether the new data is ready when tries is less than 200 times
    {
        status = VL53L1X_CheckForDataReady(dev, &dataReady);
        VL53L1_WaitMs(dev, 5);
        timeout++;
    }

    if(dataReady == 0)
    {
        UART_printf("Sensor timeout\r\n"); //UART transmit the sensor timeout if the data is not ready
        return 0;
    }

    status = VL53L1X_GetRangeStatus(dev, &RangeStatus); //read the range status
    status = VL53L1X_GetDistance(dev, &Distance); //read the distance data
    status = VL53L1X_ClearInterrupt(dev); //clear the interrupt
		
		if(RangeStatus != 0)
    {
        UART_printf("Out of range\r\n"); //UART transmit out of range if the data is not valid
        return 4000; //return the maximum distance the ToF sensor can measure
    }

    if(Distance > 4000)
    {
        return 4000; //return 4000 if the measured distance is bigger than the maximum capable measuring distance
    }

    return Distance;
}

//function that will rortate the stepper motor in counterclockwise for certain steps in order to move the ToF sensor back to the original point
void homemode(){
	
	Additional_status_LED_on(); //turn on additional LED D1 when the motor is working
	for(int k = 0; k < 31; k++) //since we rotate 11.25 degree 31 times in one scan so i use a for loop to repeat it 31 times
  {
     for(int i = 0; i < 16; i++) //the steps needs to rotate 11.25 degrees is 64 so i repeat it 16 times since 16 * 4 is 63
     {
        stepper_ccw(); //call the counterclockwise rotation function which includes 4 steps
     }
  }
	Additional_status_LED_off(); //turn off the the additional LED D1 when the motor stop working
	
}

//function that initialize the sensor to ready for distance measurement
void Initialize_Sensor(void)
{
	uint8_t sensorState = 0;
	uint8_t byteData = 0;
	uint16_t wordData = 0;
	
	UART_printf("Program Begins\r\n"); //send message to the PC to tell the user the state of the program
	
	// reset ToF sensor using XSHUT
	VL53L1X_XSHUT();
	SysTick_Wait10ms(10);
	
	// wait for sensor boot
	while(sensorState == 0)
	{
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
	}

	UART_printf("ToF Chip Booted!\r\n");

	// clear interrupt and initialize sensor
	status = VL53L1X_ClearInterrupt(dev);
	status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

	// optional settings
	status = VL53L1X_SetDistanceMode(dev, 2);          // 1=short, 2=long
	status = VL53L1X_SetTimingBudgetInMs(dev, 100);   // 20, 50, 100, 200, 500
	status = VL53L1X_SetInterMeasurementInMs(dev, 200);

	// start ranging
	status = VL53L1X_StartRanging(dev);
}

//function that will take one 360 degree scan which include 32 measurements
void Scan_Onetime(int layer)
{
	int k; //intialize the local variable
	
	for(k = 0; k < 32; k++) //create a for loop that in order to take 32 measurements
	{
		distance[layer][k] = Get_Distance(); //call the Get_Distance function to collect the distacne data and store into the 2D array
		Measurement_status_LED_Flash(); // flash the LED D4 when a measurement take place

		sprintf(printf_buffer, "%d,%.2f,%u\r\n", layer + 1, k * 11.25, distance[layer][k]);
		UART_printf(printf_buffer); //send a live UART transmission line to let the user indicate the distance data measured

		if (k < 31) //if the number of measurements is less than 32
		{
			Additional_status_LED_on(); //turn on the additional LED when the motor stsrt working
			for (int i = 0; i < 16; i++) //repeat 16 times to achieve 64 steps which will rotate 11.25 degree
			{
				stepper_cw(); //call the counterclockwise rotation function
			}
			Additional_status_LED_off(); //turn off the additional LED when the motor stop working
		}
		SysTick_Wait10ms(20); //have a 200 ms delay after each 11.25 rotation
	}
	
	homemode();// call the homemode() function to rotate the ToF sensor back to the start position
}
//function that will transfer all the data to the PC after all the scans are finished
void Transfer_data(void)
{
	int a, b; //initialize local variables

  UART_LED_Flash(); //flash the on-board LED D3 when the UART data transmission is taking place
	UART_printf("scan,x_axis_pos,angle_deg,distance_mm\r\n"); //use UART to send a guide to the user that each column represent which data

  for(a = 0; a < 10; a++) //use a for loop to transmit the data per scan
	{
    for(b = 0; b < 32; b++) //transmit the 32 measurements data in one scan
		{
      sprintf(printf_buffer, "%d,%d,%.2f,%u\r\n", a + 1, 1000 * a, b * 11.25, distance[a][b]); //send the layer, x displacement, angle data and distance data
      UART_printf(printf_buffer); // use UART to tranmit the data to the PC
    }
  }

    UART_printf("scan_done\r\n");// send a closing message after all the data is transmitted 
}

//the main function
int main(void)
{
	PLL_Init(); //intiaize the PLL
	SysTick_Init(); //initialize the SysTick
	onboardLEDs_Init(); 
	I2C_Init(); //initialize the I2C
	UART_Init(); //initialize the UART
	PortG_Init(); //initialize Port G
	PortH_Init(); //initialize Port H
	PortJ_Init(); //initialize Port J
  PortN_Init(); //initialize Port N
  PortF_Init(); //initialize Port F
	PortM_Init(); //initialize Port M

	Initialize_Sensor(); //initialize the ToF sensor

	while(1) //create a infinity loop
	{
		if((GPIO_PORTJ_DATA_R & 0x01) == 0) //use polling method to check whether PJ0 is pressed
		{
      while((GPIO_PORTJ_DATA_R & 0x01) == 0){} //wait fot PJ0 to be released

			UART_printf("Start 10 scans\r\n"); //send a message to the PC to remind the user scan started

      for(int a = 0; a < 10; a++) //run 10 scans
			{
        sprintf(printf_buffer, "Starting scan %d\r\n", a + 1); //sens a message to user to remind which scan currently in
        UART_printf(printf_buffer);

        Scan_Onetime(a); //call the function to run one complete scan
        SysTick_Wait10ms(20); //have a 200ms delay after each scan
      }

      Transfer_data(); //call the function to transfer all the data after all the scans are finished
      UART_printf("Finished\r\n"); //send a message to user to rimind everything is finished
    }
		
		//the two lines of the code below is used for bus speed checking, uncomment the two lines of code below to enable bus speed checking
		//it just toggle the PM0 every 50000Hz
		//GPIO_PORTM_DATA_R ^= 0x01;  
    //SysTick_Wait(50000); 
	}
}



