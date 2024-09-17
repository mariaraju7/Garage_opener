/*
 * ECE 153B
 *
 * Name(s): Janani Ganesh, Maria Raju
 * Section: Wed 7PM
 * Project
 */


#include "UART.h"
#include "DMA.h"
#include <string.h>
#include <stdio.h>

static volatile DMA_Channel_TypeDef * tx;
static volatile char inputs[IO_SIZE];
static volatile uint8_t data_t_0[IO_SIZE];
static volatile uint8_t data_t_1[IO_SIZE];
static volatile uint8_t input_size = 0;
static volatile uint8_t pending_size = 0;
static volatile uint8_t * active = data_t_0;
static volatile uint8_t * pending = data_t_1;
static volatile uint8_t dmafinish =0; 

#define SEL_0 1
#define BUF_0_EMPTY 2
#define BUF_1_EMPTY 4
#define BUF_0_PENDING 8
#define BUF_1_PENDING 16

void transfer_data(char ch);
void on_complete_transfer(void);

void UART1_Init(void) {

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN   ; //enable clck
	RCC->CCIPR &= ~ RCC_CCIPR_USART1SEL; //clear bits
	RCC->CCIPR |= RCC_CCIPR_USART1SEL_0; //01, : System clock (SYSCLK) selected as USART2 clock
	
}

void UART2_Init(void) {

	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; //enable clck
	RCC->CCIPR &= ~ RCC_CCIPR_USART2SEL; //clear bits
	RCC->CCIPR |= RCC_CCIPR_USART2SEL_0; //01, : System clock (SYSCLK) selected as USART2 clock
	
}

void UART1_GPIO_Init(void) {

	// PB6 and PB7 to operate as UART transmitters and receivers.
	
	//initializing PB6
	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN; //enable clock
	
	//Set PB6 to very high output speed
	GPIOB->OSPEEDR  &= ~(GPIO_OSPEEDER_OSPEEDR6);
	GPIOB->OSPEEDR  |= (GPIO_OSPEEDER_OSPEEDR6); // sets both bits high, for high speed
	
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT6); // sets to push/pull output type
	
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6); // Resets to no pull-up/pull-down
  GPIOB->PUPDR |= GPIO_PUPDR_PUPD6_0; // 01, sets it to pull-up
	
		// Alternative Function
	GPIOB->MODER &= ~(GPIO_MODER_MODE6_0); //sets bit 2 to high
	
	// Selects the alternative function for PB7
	GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL6;
	GPIOB->AFR[0] |= (GPIO_AFRL_AFSEL6_2| GPIO_AFRL_AFSEL6_1|GPIO_AFRL_AFSEL6_0);
	
	
	// initializing PB7
	
	GPIOB->OSPEEDR  &= ~(GPIO_OSPEEDER_OSPEEDR7);
	GPIOB->OSPEEDR  |= (GPIO_OSPEEDER_OSPEEDR7); // sets both bits high, for high speed
	
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT7); // sets to push/pull output type
	
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD7); // Resets to no pull-up/pull-down
  GPIOB->PUPDR |= GPIO_PUPDR_PUPD7_0; // 01, sets it to pull-up
	
	// Alternative Function
	GPIOB->MODER &= ~ (GPIO_MODER_MODE7_0); //sets bit 2 to high
	
	// Selects the alternative function for PB7
	GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL7;
	GPIOB->AFR[0] |= (GPIO_AFRL_AFSEL7_2| GPIO_AFRL_AFSEL7_1 | GPIO_AFRL_AFSEL7_0);

	
}

void UART2_GPIO_Init(void) {

	
	// PA2 and PA3 to operate as UART transmitters and receivers.
	
	//initializing PA2
	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; //enable clock
	
	//Set PA2 to very high output speed
	GPIOA->OSPEEDR  &= ~(GPIO_OSPEEDER_OSPEEDR2);
	GPIOA->OSPEEDR  |= (GPIO_OSPEEDER_OSPEEDR2); // sets both bits high, for high speed
	
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT2); // sets to push/pull output type
	
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD2); // Resets to no pull-up/pull-down
  GPIOA->PUPDR |= GPIO_PUPDR_PUPD2_0; // 01, sets it to pull-up
	
		// Alternative Function
	GPIOA->MODER &= ~(GPIO_MODER_MODE2_0); //sets bit 2 to high
	
	// Selects the alternative function for PA5
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2;
	GPIOA->AFR[0] |= (GPIO_AFRL_AFSEL2_2| GPIO_AFRL_AFSEL2_1|GPIO_AFRL_AFSEL2_0);
	
	
	// initializing PA3
	
	GPIOA->OSPEEDR  &= ~(GPIO_OSPEEDER_OSPEEDR3);
	GPIOA->OSPEEDR  |= (GPIO_OSPEEDER_OSPEEDR3); // sets both bits high, for high speed
	
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT3); // sets to push/pull output type
	
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD3); // Resets to no pull-up/pull-down
  GPIOA->PUPDR |= GPIO_PUPDR_PUPD3_0; // 01, sets it to pull-up
	
	// Alternative Function
	GPIOA->MODER &= ~ (GPIO_MODER_MODE3_0); //sets bit 2 to high
	
	// Selects the alternative function for PA5
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3;
	GPIOA->AFR[0] |= (GPIO_AFRL_AFSEL3_2| GPIO_AFRL_AFSEL3_1 | GPIO_AFRL_AFSEL3_0);
}

void USART_Init(USART_TypeDef * USARTx) {

	USARTx->CR1 &= ~ USART_CR1_UE; //usart is disabled in order to modify registers
	
	if(USARTx == USART1){
		//initialize UART1
		UART1_Init(); 
		UART1_GPIO_Init();
		
		tx = DMA1_Channel4; // channel corresponds to USART1_TX
		
		NVIC_SetPriority(USART1_IRQn,2); //Set interrupt priority to 2 in NVIC
		NVIC_EnableIRQ(USART1_IRQn); //Enable interrupt in NVIC
	}
	else if(USARTx == USART2){
		//initialize UART1
		UART2_Init(); 
		UART2_GPIO_Init();
		
		tx = DMA1_Channel7; // channel corresponds to USART2_TX
		
		NVIC_SetPriority(USART2_IRQn,2); //Set interrupt priority to 2 in NVIC
		NVIC_EnableIRQ(USART2_IRQn); //Enable interrupt in NVIC
		
	}
	
	DMA_Init_UARTx(tx, USARTx); //initiliazing dma
	
	tx->CMAR = (uint32_t)active; //set data source to active
	tx->CPAR = (uint32_t) &USARTx->TDR; //set desitination = transmit data register
	
	USARTx->CR1  &= ~ USART_CR1_M; //set word length to 8 bits
	USARTx->CR1  &= ~ USART_CR1_OVER8 ; // oversampling mode to oversample by 16 
	USARTx->CR2 &= ~USART_CR2_STOP; // number of stop bits to 1
	
	USARTx->BRR = 8333; // f_clk = 80Mhz /USART_DIV = 9600 
	
	USARTx->CR1 |= USART_CR1_RE; //enables reciever
	USARTx->CR1 |= USART_CR1_TE; //enables transmitter
	
	USARTx->CR3 |= USART_CR3_DMAT; //enable DMA transmitter
	
	USARTx->CR3 |= USART_CR3_OVRDIS; //stops overrun from breaking uart
	
	USARTx-> CR1 |= USART_CR1_RXNEIE; //enable reciver not empty interrupt
	
	USARTx->CR1 |= USART_CR1_UE; //usart is now enabled

}



/**
 * This function accepts a string that should be sent through UART
*/
void UART_print(char* data) {
	
	//transfer_data(*data);
	if((tx->CCR & DMA_CCR_EN) == 0){ //data can be transferred only if channel is disabled
		input_size = strlen(data); //Transfer char array to buffer
		sprintf(active, data); //takes all of data and transfer it to active buffer
		tx->CNDTR = input_size; //number of data to transfer
		dmafinish = 0;
		tx->CCR |= DMA_CCR_EN; //enable DMA to read only mode after writing data in
		while (!dmafinish) {} //wait till dma finsihes 
		
			//resets active buffer
		for (int i=0; i<input_size; i++){
			active[i] =0; 
		}
		input_size =0; 
		
	}
	
	else{
			sprintf(pending, data); //If DMA is not ready, put the data aside into pending buffer
			pending_size = strlen(data);
	}
}


void UART_input(void){
  if((USART1->ISR & USART_ISR_RXNE) !=0){
    transfer_data(USART1->RDR);
  }
}

/**
 * This function should be invoked when a character is accepted through UART
*/
void transfer_data(char ch) {
	// Append character to input buffer.
	inputs[input_size++] = ch; //assigns ch to input_size, then increments input_size
	
	// If the character is end-of-line, invoke UART_onInput
	if(ch == '\n'){
		UART_onInput(inputs, input_size);
		for(int i=0; i<input_size; i++){
			inputs[i] =0;
		}
		input_size=0;
	}
	
}

/**
 * This function should be invoked when DMA transaction is completed
*/
void on_complete_transfer(void) {
	// If there are pending data to send, switch active and pending buffer, and send data
	
	if(pending_size!=0){ //function first checks if any data to transmit
		
		uint8_t *var = active; // need a temporary variable to swap buffer
		active = pending; // swap occurs
		pending = var; //swap continues
		
		tx->CNDTR = pending_size;
		dmafinish = 0; //dma not finished
		
		tx->CCR |= DMA_CCR_EN; //enable dma = read-only mode
		
		while(dmafinish == 0){} //wait till dma finishes
		tx->CCR &= ~(DMA_CCR_EN); //disable dma to write into it
		
		//reset pending buffer
		for (int i=0; i<pending_size; i++){
			pending[i] =0; 
		}
		pending_size = 0; // indicates no more data to be trasnmitted 
		
	}
}

void USART1_IRQHandler(void){
	//TODO
	tx =DMA1_Channel4;
	NVIC_ClearPendingIRQ(USART1_IRQn);
	
	// When receive a character, invoke transfer_data
	if (USART1-> ISR & USART_ISR_RXNE )
	{
		USART1-> RQR |= USART_RQR_RXFRQ; //flush request checks all data is currently transmitting
		char ch = USART1-> RDR;			transfer_data(ch);
	}
	
	if (USART1->ISR & USART_ISR_TC) {
	// When complete sending data, invoke on_complete_transfer
		
		USART1->ICR |= USART_ICR_TCCF; //Transmission Complete Clear Flag
		on_complete_transfer();
	}
		
}

void USART2_IRQHandler(void){
	//TODO
	tx =DMA1_Channel7;
	NVIC_ClearPendingIRQ(USART2_IRQn);
	
	// When receive a character, invoke transfer_data
	if (USART2-> ISR & USART_ISR_RXNE ){		
		USART2-> RQR |= USART_RQR_RXFRQ; //flush request checks all data is currently transmitting
		char ch = USART2->RDR;
		transfer_data(ch);
	}
	
	if (USART2->ISR & USART_ISR_TC) {
	// When complete sending data, invoke on_complete_transfer		
		USART2->ICR |= USART_ICR_TCCF; //Transmission Complete Clear Flag
		on_complete_transfer();
	}
	
}


void DMA1_Channel7_IRQHandler(void)
{
	NVIC_ClearPendingIRQ(DMA1_Channel7_IRQn); //clear NVIC interrupt flag

	//Check Transfer Complete interrupt flag. If it occurs, clear the flag and mark computationas completed by calling computationComplete
	if((DMA1->ISR & DMA_ISR_TCIF7) !=0){
		DMA1->IFCR |= DMA_IFCR_CTCIF7; //clear transfer complete
		dmafinish = 1;
		tx->CCR &= ~DMA_CCR_EN; //disables dma
		on_complete_transfer(); 
	}
	DMA1->IFCR |= DMA_IFCR_CGIF7;	//Clear global DMA interrupt flag.
}


void DMA1_Channel4_IRQHandler(void)
{
	NVIC_ClearPendingIRQ(DMA1_Channel4_IRQn); //clear NVIC interrupt flag

	//Check Transfer Complete interrupt flag. If it occurs, clear the flag and mark computationas completed by calling computationComplete
	if((DMA1->ISR & DMA_ISR_TCIF4) !=0){
		DMA1->IFCR |= DMA_IFCR_CTCIF4; //clear transfer complete
		dmafinish = 1;
		tx->CCR &= ~DMA_CCR_EN; //disables dma
		on_complete_transfer(); 
	}	
	DMA1->IFCR |= DMA_IFCR_CGIF4;	//Clear global DMA interrupt flag.
}


