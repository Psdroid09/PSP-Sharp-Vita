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
## Hallazgo adicional: implementación interna de SceCompat

Se localizó una implementación de referencia de SceCompat en https://github.com/shoui520/psp2/tree/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/kernel/src/compat.

El código de compat.c permite precisar el comportamiento de dos funciones:

- compatFrameBufferInit recibe un SceUID de memblock, lo convierte a GUID, retiene una referencia al objeto y lo guarda en gCompatFrameBuffer. No devuelve una dirección de píxeles al llamador.
- compatEnterRunningState obtiene la base virtual de ese memblock, la traduce a dirección física y programa los registros SceCompatLCDDMA con 480×272, stride 2048 y pixelFormat 2 cuando existe un framebuffer registrado.
- compatLCDCSync espera a un punto de barrido horizontal (hcount) según el modo de pantalla; no copia ni devuelve el framebuffer.

Esto aclara que SceCompatFrameBufferInit registra un bloque de memoria para el subsistema de salida LCDDMA. No es una API genérica de captura ni un getter del framebuffer interno del PSPemu. La ruta puede estar relacionada con la salida de compatibilidad/TV, pero la fuente revisada no basta para afirmar que sea el frame usado por una burbuja Chovy-Sign en la pantalla integrada.

**Impacto en la decisión:** la implementación interna refuerza que SceCompat no ofrece el acceso simple que necesitamos. No usar esta API como atajo para inyectar un filtro; primero habría que identificar quién registra el memblock, en qué escenarios se activa LCDDMA y si ese bloque coincide con la superficie real que queremos procesar.

## Hallazgo de mayor precisión: SceDisplaySetFrameBufForCompat

Se inspeccionó el wrapper de firmware 3.60 y su export table. La función `sceDisplaySetFrameBufForCompat` está exportada por SceDisplayUser y el código fuente del firmware indica que solo está disponible para un proceso emulador PSP.

Su firma de wrapper tiene seis argumentos. El tercer argumento es un bloque de opciones `SceDisplaySetFrameBufForCompatOpt` (0x18 bytes). Ese bloque contiene un puntero `frame_buf` a `SceDisplayFrameBufForCompat` (0x1C bytes), que describe el framebuffer de origen con base, pitch en píxeles, formato, ancho, alto y flags. Un campo distinto `capture_frame_buf` describe un destino opcional de captura.

Esto es más específico que `sceDisplayGetFrameBuf`, que consulta la salida Vita, y que las APIs SceCompat de asignación/sincronización. La función compat está documentada en el código del firmware como disponible únicamente para procesos emuladores PSP, por lo que su import en ScePspemu es un candidato acotado para observar el frame enviado al subsistema de presentación.

Referencias:
- Firmware wrapper: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/usermode/src/driver_us/display.c
- Export table y restricción de proceso: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/usermode/src/driver_us/exports.yml
- Estructuras SDK: https://github.com/vitasdk/vita-headers/blob/e66ebe90b73d1fa4cce005a5b2072cec27322544/include/psp2/display.h

**Corrección importante:** el último argumento observado por el hook de Adrenaline no es el framebuffer de origen; corresponde a uno de los parámetros opcionales de captura. El origen está anidado en el tercer argumento. La documentación de `research/PSPEMU_PLUGIN_SCOPE.md` contiene la auditoría completa y el plan de prueba actualizado.

**Estado:** punto de observación prometedor, no todavía una ruta de modificación segura. No leer ni escribir píxeles hasta verificar que el descriptor es válido, que la memoria es accesible, que PSPemu terminó de renderizar y que el buffer no está siendo leído por DMA/scanout.


## Critical correction: wrapper argument mapping

A source-level audit of the firmware wrapper and kernel syscall corrected the earlier assumption that the third hook argument points to an options block.

The hooked SceDisplayUser import at library NID 0x4FAACD11 / function NID 0x8C36B628 is the six-argument user wrapper:
- arg1: head
- arg2: index
- arg3: compatibility crop-width parameter (do not dereference it as an options structure)
- arg4: sync
- arg5: optional source framebuffer pointer (the wrapper passes this as the kernel syscall's pParam)
- arg6: optional conversion/capture destination pointer (the wrapper passes this as convDst)

The wrapper reads the size field from arguments 5 and 6 and forwards both pointers and sizes through its syscall argument block. The kernel implementation then copies arg5 into a local source framebuffer descriptor and arg6 into a separate conversion destination descriptor. The source descriptor is therefore the fifth argument, not the third or sixth.

Primary source:
- User wrapper: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/usermode/src/driver_us/display.c
- Kernel syscall copy/argument mapping: https://github.com/shoui520/psp2/blob/0261b5803d8108135d6cd8dc8660834d865b4e24/sys/internal/driver/src/display/display_syscall.c
- VitaSDK compat descriptor: https://github.com/vitasdk/vita-headers/blob/e66ebe90b73d1fa4cce005a5b2072cec27322544/include/psp2/display.h

Adrenaline's six-argument hook is consistent with the wrapper ABI. Its final argument is the optional conversion/capture destination; replacing that argument when NULL is a POPS-specific path. This does not mean the sixth argument is the PSP source framebuffer.

The diagnostic prototype has been corrected to observe arg5 as the source descriptor and arg6 only as the optional conversion destination. It does not dereference arg3. It remains uncompiled and untested.
