/*
 * Módulo: Test bootstrap
 * Solo para pruebas/demos: siembra un usuario demo en EEPROM al arrancar.
 * Se activa con DOMOTIC_HOME_ENABLE_DEMO_SEED; en build normal queda inactivo.
 */
#ifndef TEST_BOOTSTRAP_H
#define TEST_BOOTSTRAP_H

#ifdef __cplusplus
extern "C" {
#endif

void TestBootstrap_SeedDemoUser(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_BOOTSTRAP_H */
