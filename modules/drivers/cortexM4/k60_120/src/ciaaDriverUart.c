/* Copyright 2014, 2015 Mariano Cerdeiro, Esteban Volentini
 * All rights reserved.
 *
 * This file is part of CIAA Firmware.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** \brief CIAA UART Driver for K60_120
 **
 ** Implements the UART Driver for K60_120
 **
 **/

/** \addtogroup CIAA_Firmware CIAA Firmware
 ** @{ */
/** \addtogroup Drivers CIAA Drivers
 ** @{ */
/** \addtogroup UART UART Drivers
 ** @{ */

/*
 * Initials     Name
 * ---------------------------
 * MaCe         Mariano Cerdeiro
 * EV			Esteban Volentini
 */

/*
 * modification history (new versions first)
 * -----------------------------------------------------------
 * 20150403 v0.0.2 EV   first operational version
 * 20140913 v0.0.1 MaCe first stub version
 */

/*==================[inclusions]=============================================*/
#include "ciaaDriverUart_Internal.h"
#include "ciaaDriverUart.h"
#include "ciaaPOSIX_stdlib.h"
#include "ciaaPOSIX_stdio.h"
#include "os.h"

#include "fsl_device_registers.h"
#include "fsl_clock_manager.h"
#include "fsl_mcg_hal.h"


/*==================[macros and definitions]=================================*/
/** \brief Pointer to Devices */
typedef struct  {
   ciaaDevices_deviceType * const * const devices;
   uint8_t countOfDevices;
} ciaaDriverConstType;

#define UART_RX_FIFO_SIZE       (1)

typedef struct {
   uint8_t hwbuf[UART_RX_FIFO_SIZE];
   uint8_t rxcnt;
} ciaaDriverUartControl;

/*==================[internal data declaration]==============================*/
/** \brief Uart Port 0 */
static ciaaDriverUart_portType const ciaaDriverUart_port0;

/** \brief Uart Port 1 */
static ciaaDriverUart_portType const ciaaDriverUart_port1;

/** \brief Uart Port 2 */
static ciaaDriverUart_portType const ciaaDriverUart_port2;

/** \brief Buffers */
ciaaDriverUartControl uartControl[3];

static ciaaDevices_deviceType ciaaDriverUart_device0 = {
   "uart/0",                        /** <= driver name */
   ciaaDriverUart_open,             /** <= open function */
   ciaaDriverUart_close,            /** <= close function */
   ciaaDriverUart_read,             /** <= read function */
   ciaaDriverUart_write,            /** <= write function */
   ciaaDriverUart_ioctl,            /** <= ioctl function */
   NULL,                            /** <= seek function is not provided */
   NULL,                            /** <= upper layer */
   (void*)&ciaaDriverUart_uart0,    /** <= layer */
   (void*)&ciaaDriverUart_port0     /** <= NULL no lower layer */
};

/** \brief Device for UART 1 */
static ciaaDevices_deviceType ciaaDriverUart_device1 = {
   "uart/1",                        /** <= driver name */
   ciaaDriverUart_open,             /** <= open function */
   ciaaDriverUart_close,            /** <= close function */
   ciaaDriverUart_read,             /** <= read function */
   ciaaDriverUart_write,            /** <= write function */
   ciaaDriverUart_ioctl,            /** <= ioctl function */
   NULL,                            /** <= seek function is not provided */
   NULL,                            /** <= upper layer */
   (void*)&ciaaDriverUart_uart1,    /** <= layer */
   (void*)&ciaaDriverUart_port1     /** <= NULL no lower layer */
};

/** \brief Device for UART 2 */
static ciaaDevices_deviceType ciaaDriverUart_device2 = {
   "uart/2",                        /** <= driver name */
   ciaaDriverUart_open,             /** <= open function */
   ciaaDriverUart_close,            /** <= close function */
   ciaaDriverUart_read,             /** <= read function */
   ciaaDriverUart_write,            /** <= write function */
   ciaaDriverUart_ioctl,            /** <= ioctl function */
   NULL,                            /** <= seek function is not provided */
   NULL,                            /** <= upper layer */
   (void*)&ciaaDriverUart_uart2,    /** <= layer */
   (void*)&ciaaDriverUart_port2     /** <= NULL no lower layer */
};

static ciaaDevices_deviceType * const ciaaUartDevices[] = {
   &ciaaDriverUart_device0,
   &ciaaDriverUart_device1,
   &ciaaDriverUart_device2
};

static ciaaDriverConstType const ciaaDriverUartConst = {
   ciaaUartDevices,
   3
};

/*==================[internal functions declaration]=========================*/
/** \brief Uart 0 */
ciaaDriverUart_uartType ciaaDriverUart_uart0;

/** \brief Uart 1 */
ciaaDriverUart_uartType ciaaDriverUart_uart1;

/** \brief Uart 2 */
ciaaDriverUart_uartType ciaaDriverUart_uart2;


/*==================[internal data definition]===============================*/
static ciaaDriverUart_portType const ciaaDriverUart_port0 = {
   UART0, 0, UART0_RX_TX_IRQn, kSimClockGateUart0,
   { PORTD, 6u, kPortMuxAlt3, kSimClockGatePortD },
   { PORTD, 7u, kPortMuxAlt3, kSimClockGatePortD }
};

static ciaaDriverUart_portType const ciaaDriverUart_port1 = {
   UART5, 5, UART5_RX_TX_IRQn, kSimClockGateUart5,
   { PORTE, 9u, kPortMuxAlt3, kSimClockGatePortE },
   { PORTE, 8u, kPortMuxAlt3, kSimClockGatePortE }
};

static ciaaDriverUart_portType const ciaaDriverUart_port2 = {
   UART4, 4, UART4_RX_TX_IRQn, kSimClockGateUart4,
   { PORTC, 14u, kPortMuxAlt3, kSimClockGatePortC },
   { PORTC, 15u, kPortMuxAlt3, kSimClockGatePortC }
};

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/
static void ciaaDriverUart_rxIndication(ciaaDevices_deviceType const * const device, uint32_t const nbyte)
{
   /* receive the data and forward to upper layer */
   ciaaSerialDevices_rxIndication(device->upLayer, nbyte);
}

static void ciaaDriverUart_txConfirmation(ciaaDevices_deviceType const * const device)
{
   /* receive the data and forward to upper layer */
   ciaaSerialDevices_txConfirmation(device->upLayer, 1 );
}

/** \brief Initialize configuration variables and hardware port  */
void ciaaDriverUart_configInit(ciaaDevices_deviceType * device, uint8_t index)
{
   ciaaDriverUart_uartType * uart = device->layer;

	uart->instance = index;

   uart->config.baudRate = 115200;
   uart->config.bitCountPerChar = kUart8BitsPerChar;
   uart->config.parityMode = kUartParityDisabled;
#if FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT
   uart->config.stopBitCount = kUartOneStopBit;
#endif

}
/*==================[external functions definition]==========================*/
extern ciaaDevices_deviceType * ciaaDriverUart_open(char const * path, ciaaDevices_deviceType * device, uint8_t const oflag)
{
   ciaaDriverUart_uartType * uart = device->layer;
   ciaaDriverUart_portType const * port = device->loLayer;

   uint32_t uartSourceClock;

   /* Enable clock for PORTs */
   SIM_HAL_EnableClock(SIM, port->rx.gate);
   SIM_HAL_EnableClock(SIM, port->tx.gate);
   /* Configure port pins functions to UART */
   PORT_HAL_SetMuxMode(port->rx.port, port->rx.pin, port->rx.mux);
   PORT_HAL_SetMuxMode(port->tx.port, port->tx.pin, port->tx.mux);

   SIM_HAL_EnableClock(SIM, port->gate);
   /* Initialize HAL UART to known state */
   UART_HAL_Init(port->base);

   /* UART clock source is either system or bus clock depending on instance */
   uartSourceClock = CLOCK_SYS_GetUartFreq(port->instance);

  /* Initialize UART baud rate, bit count, parity and stop bit. */
   UART_HAL_SetBaudRate(port->base, uartSourceClock, uart->config.baudRate);
   UART_HAL_SetBitCountPerChar(port->base, uart->config.bitCountPerChar);
   UART_HAL_SetParityMode(port->base, uart->config.parityMode);
#if FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT
   UART_HAL_SetStopBitCount(port->base, uart->config.stopBitCount);
#endif
   /* Enable UART transmitter and receiver modules */
   UART_HAL_EnableTransmitter(port->base);
   UART_HAL_EnableReceiver(port->base);

   /* Enable interrupts from UART module */
   NVIC_EnableIRQ(port->irq);

   return device;
}

extern int32_t ciaaDriverUart_close(ciaaDevices_deviceType const * const device)
{
   ciaaDriverUart_portType const * port = device->loLayer;
   int32_t ret = -1;

   UART_HAL_DisableTransmitter(port->base);
   UART_HAL_DisableReceiver(port->base);
   NVIC_EnableIRQ(port->irq);

   ret = 0;

   return ret;
}

extern int32_t ciaaDriverUart_ioctl(ciaaDevices_deviceType const * const device, int32_t const request, void * param)
{
   ciaaDriverUart_uartType * uart = device->layer;
   int32_t ret = -1;

   if((device == ciaaDriverUartConst.devices[0]) ||
      (device == ciaaDriverUartConst.devices[1]) ||
      (device == ciaaDriverUartConst.devices[2]))
   {
      switch(request)
      {
         /* signal to start transmition */
         case ciaaPOSIX_IOCTL_STARTTX:
            ciaaDriverUart_txConfirmation(device);
         break;

         /* set serial port baudrate */
         case ciaaPOSIX_IOCTL_SET_BAUDRATE:
            uart->config.baudRate = (uint32_t)(param);
            /** @todo Cambiar velocidad en el puerto fisico */
            break;
      }
   }
   return ret;
}

extern ssize_t ciaaDriverUart_read(ciaaDevices_deviceType const * const device, uint8_t * const buffer, size_t const size)
{
  // ciaaDriverUart_uartType * uart = device->layer;
   ssize_t ret = -1;

   return ret;
}

extern ssize_t ciaaDriverUart_write(ciaaDevices_deviceType const * const device, uint8_t const * const buffer, size_t const size)
{
   ciaaDriverUart_portType const * port = device->loLayer;
   ssize_t ret = 0;

   if((device == ciaaDriverUartConst.devices[0]) ||
      (device == ciaaDriverUartConst.devices[1]) ||
      (device == ciaaDriverUartConst.devices[2]) )
   {
      while(UART_HAL_GetStatusFlag(port->base, kUartTxDataRegEmpty) && (ret < size))
      {
         /* send first byte */
         UART_HAL_Putchar(port->base, buffer[ret]);
         /* bytes written */
         ret++;
      }
      UART_HAL_SetIntMode(port->base, kUartIntTxDataRegEmpty, true);
   }
   return ret;
}

void ciaaDriverUart_init(void)
{
   uint8_t loopi;

   /* add uart driver to the list of devices */
   for(loopi = 0; loopi < ciaaDriverUartConst.countOfDevices; loopi++) {
      /* initialize configuration variables and port */
      ciaaDriverUart_configInit(ciaaDriverUartConst.devices[loopi], loopi);

      /* add each device */
      ciaaSerialDevices_addDriver(ciaaDriverUartConst.devices[loopi]);
   }
}


ISR(UART0_IRQHandler)
{

}


ISR(UART2_IRQHandler)
{
   ciaaDriverUart_portType const * port = ciaaUartDevices[1]->loLayer;

   if (UART_HAL_GetStatusFlag(port->base, kUartRxDataRegFull) && UART_HAL_GetIntMode(port->base, kUartRxDataRegFull))
   {
      do
      {
         UART_HAL_Getchar(port->base, &uartControl[1].hwbuf[uartControl[1].rxcnt]);
         uartControl[1].rxcnt++;
      } while((UART_HAL_GetStatusFlag(port->base, kUartRxDataRegFull)) &&
             (uartControl[1].rxcnt < UART_RX_FIFO_SIZE));

      ciaaDriverUart_rxIndication(&ciaaDriverUart_device1, uartControl[1].rxcnt);
   }
   if (UART_HAL_GetStatusFlag(port->base, kUartTxDataRegEmpty) && UART_HAL_GetIntMode(port->base, kUartIntTxDataRegEmpty))
   {
      /* tx confirmation, 1 byte sent */
      ciaaDriverUart_txConfirmation(&ciaaDriverUart_device1);

      if (UART_HAL_GetStatusFlag(port->base, kUartTxDataRegEmpty))
      {  /* There is not more bytes to send, disable THRE irq */
         UART_HAL_SetIntMode(port->base, kUartIntTxDataRegEmpty, false);
      }
   }

}

ISR(UART3_IRQHandler)
{

}

/*==================[interrupt handlers]=====================================*/
/** @} doxygen end group definition */
/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/

