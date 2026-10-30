# Investigación del framebuffer PSPemu

**Fecha:** 30 de octubre de 2026  
**Objetivo:** determinar si se puede observar o procesar la imagen de los juegos Chovy-Sign sin modificar el emulador oficial.

## Resumen

La investigación encontró un punto técnico conocido en el entorno PSP: `sceDisplaySetFrameBuf` (NID `0x289D82FE`). Un proyecto de CFW PSP llamado OASIS OS intercepta esa función y recibe el puntero del framebuffer, el stride, el formato de píxel y el modo de sincronización.

Esto demuestra que, dentro de un entorno PSP con CFW y soporte de hooks de kernel, es posible observar el buffer que se envía a pantalla. **No demuestra que el mismo método funcione en el PSPemu stock usado por Chovy-Sign.**

## 1. Qué se conoce del lado PSP

La documentación de investigación de Vita describe el mapa de memoria PSP y señala una región de VRAM de 2 o 4 MB en `0x04000000`, utilizada para VRAM/framebuffer. También documenta registros de `SceCompatLCDDMA`, incluyendo dimensiones y dirección DMA.

Referencia:
- https://www.psdevwiki.com/vita/PSP_Emulator

La función `sceDisplaySetFrameBuf` es un candidato relevante porque recibe los datos necesarios para identificar el buffer presentado. Su NID `0x289D82FE` aparece en implementaciones PSP de hooks.

## 2. Referencia de implementación: OASIS OS

Código inspeccionado:
- https://github.com/AndrewAltimit/oasis-os/blob/6512f72df3602da6a487c0e01c7a0ecbaca13c40/crates/oasis-plugin-psp/src/hook.rs
- https://github.com/AndrewAltimit/rust-psp/blob/e4cc06f6fa50fa45dc3ae8b332e8a2b67625f588/psp/src/hook.rs

El hook de OASIS:
- Intercepta `sceDisplaySetFrameBuf`.
- Recibe `top_addr`, `buffer_width`, `pixel_format` y `sync`.
- Accede al framebuffer y dibuja un overlay antes de llamar a la función original.
- Utiliza un mecanismo de hook de CFW basado en `sctrlHENFindFunction` y `sctrlHENPatchSyscall`, con alternativa de parcheo inline.
- La implementación mantiene un trampoline para llamar a la función original.

### Qué demuestra

- Hay una vía conocida para interceptar la presentación del framebuffer en un PSP CFW.
- El callback obtiene un puntero al buffer y su stride.
- Un procesamiento de píxeles podría estudiarse en ese punto, antes de presentar el buffer.

### Qué NO demuestra

- No demuestra que el PSPemu stock de Vita permita cargar ese PRX.
- No demuestra que el NID, la biblioteca o el módulo sean iguales en el PSPemu stock.
- No demuestra que la dirección recibida sea directamente accesible con las mismas reglas de caché y memoria en todos los entornos.
- No demuestra que un filtro completo que lea y escriba cada píxel quepa en el presupuesto de tiempo de cada frame.
- El código de OASIS es una referencia técnica, no un plugin que debamos instalar en la Vita del usuario.

## 3. Bloqueo de compatibilidad con Chovy-Sign

Chovy-Sign prepara contenido PSP firmado para el emulador oficial. No convierte el PSPemu stock en un entorno PSP CFW ni proporciona por sí solo el sistema de carga de plugins de Adrenaline.

La implementación de OASIS depende explícitamente de símbolos de CFW como `sctrlHENFindFunction` y `sctrlHENPatchSyscall`. Por tanto, no se puede trasladar directamente al PSPemu stock sin encontrar otro mecanismo de integración.

Referencias:
- https://github.com/LiEnby/chovy-sign
- https://github.com/TheOfficialFloW/Adrenaline
- https://github.com/AndrewAltimit/oasis-os

**Conclusión:** `sceDisplaySetFrameBuf` es un candidato técnico para una futura ruta PSP CFW, pero no constituye actualmente un punto de hook validado para Chovy-Sign.

## 4. ¿Podría hacerse el filtro en el lado Vita?

Una segunda posibilidad sería procesar el buffer desde el lado Vita, sin cargar un PRX PSP. Sin embargo, aún falta identificar una interfaz pública y segura que permita:

1. Detectar que el PSPemu está presentando un frame.
2. Obtener el buffer correcto y su formato real.
3. Saber cuándo el PSPemu terminó de escribirlo.
4. Procesarlo sin competir con el DMA/GPU ni modificar un buffer que todavía se está usando.
5. Entregar el resultado sin romper sincronización, latencia o presentación.

No se encontró evidencia suficiente para afirmar que taiHEN ofrezca directamente un callback seguro y universal para el framebuffer del PSPemu stock. Un hook global de pantalla sería especialmente delicado porque podría afectar otras aplicaciones Vita.

## 5. Diseño de filtro que conviene evaluar offline

Antes de tocar el framebuffer real, comparar con capturas de prueba:

- **Nearest-neighbour:** conserva píxeles originales, pero puede producir bordes escalonados.
- **Bilinear:** suaviza la imagen y puede perder detalle fino.
- **Sharp-bilinear:** intenta conservar una apariencia más definida al escalar.
- **Unsharp mask:** realza bordes después del escalado; debe limitarse para evitar halos y ruido.

Un filtro de nitidez no puede recuperar detalle que el juego nunca renderizó. La aceptación debe basarse en comparaciones lado a lado con la misma captura, no solo en que la imagen parezca más contrastada.

## 6. Riesgos de rendimiento

Un filtro por frame implica leer y escribir píxeles repetidamente. En una pantalla PSP de 480×272, una pasada completa procesa 130,560 píxeles; un filtro de vecindad requiere lecturas adicionales por píxel. El coste real dependerá del formato, la memoria, el caché y la frecuencia de presentación.

No se debe asumir que el procesador Vita puede hacer el trabajo sin afectar FPS, consumo o temperatura. La primera prueba debe medir el coste fuera de la consola.

## 7. Próximos pasos y decisión

1. Buscar documentación y código de PSPemu stock que identifique módulos, bibliotecas y exports de presentación.
2. Verificar si hay una interfaz de observación de framebuffer que no dependa de CFW PSP.
3. Preparar un banco offline de imágenes sintéticas y capturas PSP legítimas para comparar filtros y medir halos.
4. Solo si aparece una interfaz de integración compatible con PSPemu stock, diseñar un prototipo desactivable.
5. No instalar hooks globales ni modificar `ur0:tai/config.txt` en la Vita principal.

## Estado

**Hallazgo parcial:** existe un hook de framebuffer en un PSP CFW (OASIS OS).  
**Bloqueo vigente:** no se ha demostrado un punto de carga o hook equivalente en el PSPemu stock de Chovy-Sign.  
**Entregable instalable:** ninguno. No hay VPK, SUPRX, SKPRX ni binario para instalar.
