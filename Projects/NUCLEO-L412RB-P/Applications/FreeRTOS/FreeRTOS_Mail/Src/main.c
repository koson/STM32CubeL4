/**
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_Mail/Src/main.c
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct 
{ /* Mail object structure */
  uint32_t var1; /* var1 is a uint32_t */
  uint32_t var2; /* var2 is a uint32_t */
  uint8_t var3; /* var3 is a uint8_t */
} Amail_TypeDef;

/* Private define ------------------------------------------------------------*/
#define blckqSTACK_SIZE   configMINIMAL_STACK_SIZE
#define MAIL_SIZE        (uint32_t) 1

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
osMailQId mailId;

uint32_t ProducerValue1 = 0, ProducerValue2 = 0;
uint8_t ProducerValue3 = 0;
__IO uint8_t ProducerErrors = 0;
uint32_t ConsumerValue1 = 0, ConsumerValue2 = 0;
uint8_t ConsumerValue3 = 0;
__IO uint8_t ConsumerErrors = 0;

/* Private function prototypes -----------------------------------------------*/

/* Thread function that creates a mail and posts it on a mail queue. */
static void MailQueueProducer (const void *argument);

/* Thread function that receives mail , remove it  from a mail queue and checks that
it is the expected mail. */
static void MailQueueConsumer (const void *argument);

static void SystemClock_Config(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32L4xx HAL library initialization:
       - Configure the Flash prefetch and Buffer caches
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();  
  
  /* Configure the System clock to 80 MHz */
  SystemClock_Config();
  
  /* Initialize LED4 */
  BSP_LED_Init(LED4);
  
  /* Create the mail queue used by the two tasks to pass the struct Amail_TypeDef */
  osMailQDef(mail, MAIL_SIZE, Amail_TypeDef); /* Define mail queue */
  
  mailId = osMailCreate(osMailQ(mail), NULL); /* create mail queue */
  
  /* Note the producer has a lower priority than the consumer when the tasks are
     spawned. */
  osThreadDef(QCons, MailQueueConsumer, osPriorityBelowNormal, 0, blckqSTACK_SIZE);
  osThreadCreate(osThread(QCons), NULL);
  
  osThreadDef(QProd, MailQueueProducer, osPriorityBelowNormal, 0, blckqSTACK_SIZE);
  osThreadCreate(osThread(QProd), NULL);
  
  /* Start scheduler */
  osKernelStart ();

  /* We should never get here as control is now taken by the scheduler */
  for(;;);
}

/**
  * @brief  Mail Producer Thread.
  * @param  argument: Not used
  * @retval None
  */
static void MailQueueProducer(const void *argument)
{
  Amail_TypeDef *pTMail;
  
  for(;;)
  {		

    pTMail = osMailAlloc(mailId, osWaitForever); /* Allocate memory */
    pTMail->var1 = ProducerValue1; /* Set the mail content */
    pTMail->var2 = ProducerValue2;
    pTMail->var3 = ProducerValue3;
    
    if(osMailPut(mailId, pTMail) != osOK) /* Send Mail */  
    {      
      ++ProducerErrors;
      
      /* Turn on LED4 to indicate error */
      BSP_LED_On(LED4);
    }
    else
    {
      /* Increment the variables we are going to post next time round.  The
      consumer will expect the numbers to follow in numerical order. */
      ++ProducerValue1;
      ProducerValue2 += 2;
      ProducerValue3 += 3;
      
      /* Toggle LED4 to indicate a correct number received  */
      if( (ProducerErrors == 0) && (ConsumerErrors == 0) )
        BSP_LED_Toggle(LED4);

      osDelay(250);
    }
  }
}

/**
  * @brief  Mail Consumer Thread.
  * @param  argument: Not used
  * @retval None
  */
static void MailQueueConsumer (const void *argument)
{
  osEvent event;
  Amail_TypeDef *pRMail;
  
  for(;;)
  {
    /* Get the message from the queue */
    event = osMailGet(mailId, osWaitForever); /* wait for mail */
    
    if(event.status == osEventMail)
    {
      pRMail = event.value.p;
      
      if((pRMail->var1 != ConsumerValue1) || (pRMail->var2 != ConsumerValue2) || (pRMail->var3 != ConsumerValue3))
      {
        /* Catch-up. */
        ConsumerValue1 = pRMail->var1;
        ConsumerValue2 = pRMail->var2;
        ConsumerValue3 = pRMail->var3;
        
        ++ConsumerErrors;
        
        /* Turn on LED4 to indicate error */
        BSP_LED_On(LED4);
      }
      else
      {  
        /* Calculate values we expect to remove from the mail queue next time
        round. */
        ++ConsumerValue1;
        ConsumerValue2 += 2;
        ConsumerValue3 += 3;        
      }

      osMailFree(mailId, pRMail); /* free memory allocated for mail */       
    }		
  }
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

