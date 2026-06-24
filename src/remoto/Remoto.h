/*
 * Módulo: Remoto
 * Atiende comandos remotos por UART1 (Virtual Terminal) o UART0 (bridge del Monitor Serie).
 * RADIO (sonido lógico), HORNO y MERCADO. Horno: 1 min lógico = 5 s reales en demo.
 * La lista de mercado se persiste en EEPROM y también se gestiona desde la UI.
 */
#ifndef REMOTO_H
#define REMOTO_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void Remoto_Init(void);
void Remoto_Task(uint32_t now_ms);

/* ---- Market list API ---- */
uint8_t  Remoto_MarketAdd(uint8_t product_id, uint8_t quantity);
void     Remoto_MarketClear(void);
uint8_t  Remoto_MarketGetCount(void);
uint8_t  Remoto_MarketGetItem(uint8_t index, market_item_t *out);
const char* Remoto_MarketGetProductName(uint8_t product_id);
uint8_t  Remoto_MarketGetProductCount(void);

#ifdef __cplusplus
}
#endif

#endif /* REMOTO_H */
