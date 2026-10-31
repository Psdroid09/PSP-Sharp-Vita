# Auditoría de APIs SceCompat relacionadas con framebuffer

**Fecha:** 31 de octubre de 2026  
**Objetivo:** comprobar si las APIs públicas de VitaSDK ofrecen acceso directo y seguro al framebuffer PSP utilizado por PSPemu stock (burbujas Chovy-Sign).

## Conclusión

VitaSDK expone varias funciones del subsistema de compatibilidad (SceCompat) relacionadas con memoria y presentación del entorno PSP. Sin embargo, la documentación pública consultada **no ofrece una función que devuelva el puntero, pitch/stride, formato y dimensiones del framebuffer PSP activo**.

Por tanto, estas APIs son pistas para investigación, no una ruta de implementación validada. No justifican crear todavía un plugin ni modificar ur0:tai/config.txt.

## APIs encontradas en psp2/compat.h

Referencia principal: https://github.com/vitasdk/vita-headers/blob/master/include/psp2/compat.h

| API | Qué documenta | Qué no permite concluir |
|---|---|---|
| sceCompatFrameBufferInit(void *framebuffer) | Inicializa el framebuffer usando un puntero proporcionado | No devuelve el framebuffer activo ni documenta cómo obtenerlo |
| sceCompatAllocCdramWithHole(SceCompatCdram *cdram) | Asigna CDRAM y devuelve direcciones cached/uncached | No demuestra que esa memoria sea el framebuffer PSP presentado |
| sceCompatCache(mode, addr, size) | Ejecuta operaciones de caché (invalidate/writeback) sobre un rango | No concede acceso al buffer ni resuelve sincronización con GPU/DMA |
| sceCompatLCDCSync(void) | Expone una operación denominada sincronización LCDC | La documentación no especifica una secuencia segura para leer/modificar frames |
| sceCompatInitEx, sceCompatStart, sceCompatStop | Controlan el ciclo de vida del subsistema compat | No son APIs de captura o postprocesado |

Los nombres de las funciones son indicativos, pero no prueban que un plugin de usuario pueda interceptar el frame de un juego PSP.

## Diferencia con las APIs SceDisplay de Vita

Referencia: https://docs.vitasdk.org/group__SceDisplayUser.html

- sceDisplayGetFrameBuf devuelve los parámetros del framebuffer actualmente configurado para la pantalla Vita.
- sceDisplaySetFrameBuf configura o actualiza los parámetros del framebuffer de pantalla.
- Los callbacks VBlank notifican el ciclo de refresco de la pantalla.

Estas funciones describen el framebuffer de salida de Vita. No documentan que el puntero obtenido sea el buffer PSP previo al escalado ni que pueda modificarse sin interferir con PSPemu, el compositor o el scanout.

## Relación con el mapa de memoria PSPemu

La wiki de investigación de Vita documenta una región PSP de VRAM/framebuffer en 0x04000000 y registros asociados a SceCompatLCDDMA:

https://www.psdevwiki.com/vita/PSP_Emulator

Esos datos ayudan a comprender la arquitectura, pero no bastan para que un plugin Vita lea o escriba esa región de forma segura. Falta demostrar el mapeo válido desde el proceso elegido, el formato real, el stride, la propiedad del buffer y el momento en que deja de estar en uso.

## Evaluación de rutas

1. **Leer el framebuffer Vita con SceDisplay:** no satisface por sí solo el objetivo. Podría observar una superficie de salida ya escalada o administrada por el sistema; no se ha demostrado que sea modificable sin efectos secundarios.
2. **Usar SceCompat:** las APIs disponibles documentan inicialización, asignación de CDRAM y operaciones de caché, pero no una consulta del framebuffer PSP activo.
3. **Interceptar el lado PSP con sceDisplaySetFrameBuf:** hay precedentes en PSP CFW, pero dependen de infraestructura de CFW PSP y no demuestran compatibilidad con PSPemu stock/Chovy-Sign.
4. **Hook global de pantalla:** descartado como primer experimento en la Vita principal por el riesgo de afectar LiveArea y aplicaciones nativas.

## Próximas comprobaciones técnicas

- Revisar implementaciones públicas que llamen a sceCompatFrameBufferInit, sceCompatAllocCdramWithHole, sceCompatCache y sceCompatLCDCSync; determinar qué proceso las invoca y con qué buffers.
- Identificar si PSPemu expone un módulo/export o callback de presentación accesible desde un plugin limitado al proceso correcto.
- No asumir que las direcciones PSP documentadas son directamente desreferenciables desde un plugin Vita.
- Antes de cualquier filtro, probar solo lectura en un entorno aislado y verificar formato, stride, coherencia de caché y sincronización.
- Si no aparece una interfaz documentada y acotada al PSPemu, mantener el proyecto en laboratorio offline.

## Veredicto

**Sin cambio en la decisión de seguridad:** todavía no hay un punto de integración probado para filtrar los frames de juegos Chovy-Sign en PSPemu stock. No compilar ni instalar un prototipo en la PS Vita principal.