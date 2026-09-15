#include "uart.h"
#include<string.h>

#if (UART_PORT == UART_PORT_USB)
#include "tusb.h"
#include "device/usbd_pvt.h"
#elif (UART_PORT == UART_PORT_D0_D1)
#include "bsp_api.h"
#include "r_ioport.h"
#include "r_sci_uart.h"
#else
#error "uart.h: UART_PORT must be UART_PORT_USB or UART_PORT_D0_D1"
#endif

static volatile char g_rx[UART_BUFF_LEN];
static volatile uint8_t g_rxIndex = 0;
static volatile bool g_eof = false;
static volatile uint8_t g_rxLen = 0;
static volatile bool rxBusy = false;

static char* pRxFramebuffer;  // external buffer
static uint8_t rxFrameBufferLen; // external buffer length
static bool g_open = false;

static uint8_t g_tx[UART_BUFF_LEN];
static volatile bool txBusy = false;

//--------------------------------------------------------------------------------
// Called from interrupt context for each received char
static void rxChar(char c)
{
  rxBusy = true;

  if(g_rxIndex >= UART_BUFF_LEN)
  {
    // Overflow: keep overwriting the last char so the '\n' still ends the frame
    g_rxIndex = UART_BUFF_LEN - 1;
  }
  g_rx[g_rxIndex++] = c;

  if('\n' == c)
  {
    g_rxLen = g_rxIndex;

    memset(pRxFramebuffer, 0, rxFrameBufferLen); // clear external buffer

    uint8_t n = (g_rxLen < rxFrameBufferLen) ? g_rxLen : rxFrameBufferLen;
    for(uint8_t i=0; i<n; i++)
    {
      pRxFramebuffer[i] = g_rx[i];
    }

    g_rxIndex = 0; // reset rx buffer index
    g_eof = true;
  }
}

#if (UART_PORT == UART_PORT_USB)
//================================================================================
// usb-uart (TinyUSB CDC). USB is started by the core before setup(), and the
// TinyUSB callbacks below run from the USB interrupt.
//================================================================================

//--------------------------------------------------------------------------------
void tud_cdc_rx_cb(uint8_t itf)
{
  (void)itf;

  if(!g_open)
  {
    return; // leave the data in the CDC fifo
  }

  uint8_t c;
  while(tud_cdc_read(&c, 1))
  {
    rxChar((char)c);
  }
}
//--------------------------------------------------------------------------------
void tud_cdc_tx_complete_cb(uint8_t itf)
{
  (void)itf;

  if(tud_cdc_write_available() >= CFG_TUD_CDC_TX_BUFSIZE)
  {
    // tx fifo empty: all chars have been written
    txBusy = false;
  }
}
//--------------------------------------------------------------------------------
static bool portInit()
{
  return true;
}
//--------------------------------------------------------------------------------
static uint8_t portWrite(uint8_t len)
{
  if(!tud_cdc_connected())
  {
    return UART_STATUS_ERROR;
  }

  if(tud_cdc_write_available() < len)
  {
    return UART_STATUS_BUSY;
  }

  txBusy = true;
  usbd_int_set(false);
  tud_cdc_write(g_tx, len);
  usbd_int_set(true);
  tud_cdc_write_flush();

  return UART_STATUS_OK;
}
//--------------------------------------------------------------------------------
static void portCheckTxBusy()
{
  if(!tud_cdc_connected())
  {
    txBusy = false; // terminal gone, the tx complete callback may never come
  }
}

#elif (UART_PORT == UART_PORT_D0_D1)
//================================================================================
// D0/D1 uart (SCI2)
//================================================================================

#define UART_SCI_CHANNEL 2
#define UART_RX_PIN BSP_IO_PORT_03_PIN_01 // D0
#define UART_TX_PIN BSP_IO_PORT_03_PIN_02 // D1

// Defined in uart_irq.cpp
bool uart_irq_setup(uart_cfg_t* p_cfg);

static sci_uart_instance_ctrl_t g_ctrl;
static uart_cfg_t g_cfg;
static baud_setting_t g_baud;
static sci_uart_extended_cfg_t g_cfgExtend;

//--------------------------------------------------------------------------------
// Called from the SCI interrupts
static void uartCallback(uart_callback_args_t* p_args)
{
  switch(p_args->event)
  {
    case UART_EVENT_RX_CHAR:
      rxChar((char)p_args->data);
      break;

    case UART_EVENT_TX_COMPLETE:
      // all chars have been written
      txBusy = false;
      break;

    default:
      // Frame / parity / overflow errors: char is discarded
      break;
  }
}
//--------------------------------------------------------------------------------
// Init uart for UART_BAUD_RATE 8N1, rx & tx
static bool portInit()
{
  //-------------------------------
  // Pins
  R_BSP_PinAccessEnable();
  R_BSP_PinCfg(UART_RX_PIN, (uint32_t)(IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_SCI0_2_4_6_8));
  R_BSP_PinCfg(UART_TX_PIN, (uint32_t)(IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_SCI0_2_4_6_8));
  R_BSP_PinAccessDisable();

  //-------------------------------
  // Initialise uart hardware
  memset(&g_baud, 0, sizeof(g_baud));
  if(FSP_SUCCESS != R_SCI_UART_BaudCalculate(UART_BAUD_RATE, true, 3000, &g_baud)) // 3% max error
  {
    return false;
  }

  g_cfgExtend.clock = SCI_UART_CLOCK_INT;
  g_cfgExtend.rx_edge_start = SCI_UART_START_BIT_FALLING_EDGE;
  g_cfgExtend.noise_cancel = SCI_UART_NOISE_CANCELLATION_DISABLE;
  g_cfgExtend.rx_fifo_trigger = SCI_UART_RX_FIFO_TRIGGER_MAX;
  g_cfgExtend.p_baud_setting = &g_baud;
  g_cfgExtend.flow_control = SCI_UART_FLOW_CONTROL_RTS;
  g_cfgExtend.flow_control_pin = (bsp_io_port_pin_t)UINT16_MAX;
  g_cfgExtend.rs485_setting.enable = SCI_UART_RS485_DISABLE;
  g_cfgExtend.rs485_setting.polarity = SCI_UART_RS485_DE_POLARITY_HIGH;
  g_cfgExtend.rs485_setting.de_control_pin = (bsp_io_port_pin_t)UINT16_MAX;

  g_cfg.channel = UART_SCI_CHANNEL;
  g_cfg.data_bits = UART_DATA_BITS_8;
  g_cfg.parity = UART_PARITY_OFF;
  g_cfg.stop_bits = UART_STOP_BITS_1;
  g_cfg.p_callback = uartCallback;
  g_cfg.p_context = NULL;
  g_cfg.p_extend = &g_cfgExtend;
  g_cfg.p_transfer_tx = NULL;
  g_cfg.p_transfer_rx = NULL;
  g_cfg.txi_irq = FSP_INVALID_VECTOR;
  g_cfg.tei_irq = FSP_INVALID_VECTOR;
  g_cfg.rxi_irq = FSP_INVALID_VECTOR;
  g_cfg.eri_irq = FSP_INVALID_VECTOR;

  if(!uart_irq_setup(&g_cfg))
  {
    return false;
  }

  return (FSP_SUCCESS == R_SCI_UART_Open(&g_ctrl, &g_cfg));
}
//--------------------------------------------------------------------------------
static uint8_t portWrite(uint8_t len)
{
  txBusy = true;
  if(FSP_SUCCESS != R_SCI_UART_Write(&g_ctrl, g_tx, len))
  {
    txBusy = false;
    return UART_STATUS_ERROR;
  }

  return UART_STATUS_OK;
}
//--------------------------------------------------------------------------------
static void portCheckTxBusy()
{
}

#endif

//================================================================================
// Common
//================================================================================

//--------------------------------------------------------------------------------
bool uart_init(char* pRxBuffer, uint8_t rxBufferLen)
{
  pRxFramebuffer = pRxBuffer;
  rxFrameBufferLen = rxBufferLen;

  g_open = portInit();
  return g_open;
}
//--------------------------------------------------------------------------------
bool uart_checkFrameRx()
{
  bool b = g_eof;
  g_eof = false;
  return b;
}
//--------------------------------------------------------------------------------
uint8_t uart_getRxLenAndReset()
{
  rxBusy = false;
  return g_rxLen;
}
//--------------------------------------------------------------------------------
uint8_t uart_write(const char* buffer)
{
  if(!g_open)
  {
    return UART_STATUS_ERROR;
  }

  if(rxBusy || uart_getTxBusy())
  {
    return UART_STATUS_BUSY;
  }

  // copy incoming buffer to tx buffer, up to and including '\n'
  uint8_t len = 0;
  while(len < UART_BUFF_LEN)
  {
    g_tx[len] = (uint8_t)buffer[len];
    len++;
    if(buffer[len - 1] == '\n')
    {
      break;
    }
  }
  g_tx[len - 1] = '\n'; // no-op unless truncated

  return portWrite(len);
}
//--------------------------------------------------------------------------------
bool uart_getTxBusy()
{
  portCheckTxBusy();
  return txBusy;
}
//--------------------------------------------------------------------------------
