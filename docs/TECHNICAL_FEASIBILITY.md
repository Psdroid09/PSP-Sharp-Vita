# Evaluación de viabilidad técnica

**Fecha de revisión:** 30 de octubre de 2026  
**Objetivo de hardware:** PS Vita 1000, firmware 3.60 con Ensō  
**Objetivo de ejecución:** juegos PSP firmados como burbujas mediante Chovy-Sign, ejecutados en el PSPemu oficial.

## Conclusión ejecutiva

La nitidez adicional es una meta técnicamente plausible, pero todavía **no existe una ruta de implementación demostrada para las burbujas Chovy-Sign**. No se debe compilar ni instalar un plugin experimental en la Vita principal en esta etapa.

La distinción fundamental es el entorno de ejecución:

- Chovy-Sign convierte imágenes PSP a contenido firmado para ejecutarse en el firmware sin modificar, usando el PSPemu oficial.
- Adrenaline modifica el PSPemu oficial para ejecutar un entorno PSP 6.61 personalizado y habilitar la inyección de código y plugins PSP.
- Un plugin PSP que funciona dentro de Adrenaline no queda automáticamente disponible en el PSPemu oficial que lanza una burbuja Chovy-Sign.

Fuentes:
- Chovy-Sign: https://github.com/LiEnby/chovy-sign
- Adrenaline: https://github.com/TheOfficialFloW/Adrenaline
- Descripción de PSPemu/ePSP y vPSP: https://adrenaline.sarcasticat.com/docs/XX-Glossary.html

## Hallazgos

### 1. Chovy-Sign no es Adrenaline

El proyecto Chovy-Sign describe su función como convertir ISO PSP/PS1 para ejecutarlas en una Vita sin modificar. El resultado es contenido firmado, no un entorno CFW PSP con soporte general de plugins.

Adrenaline, por el contrario, modifica PSPemu mediante taiHEN e inyecta código en el IPL. Su sistema de plugins PSP (game.txt, vsh.txt) pertenece al entorno modificado de Adrenaline.

**Implicación:** no se debe dar por hecho que un PRX PSP de GePatch pueda cargarse desde una burbuja Chovy-Sign. Primero hay que demostrar un punto de carga válido dentro del PSPemu stock.

### 2. GePatch: útil como referencia, no como solución directa

GePatch de TheFloW modifica listas de comandos del Graphics Engine (GE) para renderizar determinados juegos a resolución nativa. Su documentación exige Adrenaline 7 o superior, y advierte de incompatibilidades como pantallas negras, escenas que no aparecen, texturas ausentes y problemas de clipping/culling.

No es un filtro universal de nitidez: altera la ruta de renderizado y requiere compatibilidad por juego.

Fuente: https://github.com/TheOfficialFloW/GePatch

### 3. Auditoría puntual de GePatchUltra

Repositorio revisado: https://github.com/alangrewco/GePatchUltra

La inspección de los archivos disponibles encontró una discrepancia importante entre el README y el código:

- pspemu_plugin/main.c contiene las funciones de parcheo GE, pero las búsquedas de módulos (FindProc) y las llamadas que conectan los hooks (sctrlHENPatchSyscall) están comentadas.
- module_start() escribe un ACK y devuelve éxito; eso solo demuestra que el módulo puede alcanzar su función de inicio, no que el parche GE esté activo.
- vita_renderer/main.c solo contiene module_start() y module_stop() vacíos. No inicializa GXM, no recibe comandos GE y no dibuja una imagen.
- El README muestra hitos de transporte y renderizado todavía pendientes.

**Resultado:** GePatchUltra es una referencia de arquitectura/prototipo, no una solución funcional lista para instalar. No se copiarán sus binarios ni se presentarán sus hitos como funciones terminadas.

### 4. reRescaler y Sharpscale

- reRescaler ofrece métodos de escalado y filtrado, pero su propio registro de incidencias incluye un problema de cierre de juegos PSP al abrir los ajustes desde el menú ImPOSE. Su documentación también advierte que algunos juegos pueden fallar.
- Sharpscale modifica cómo se escala el framebuffer hacia la pantalla (por ejemplo, Point o Bilinear). Eso puede cambiar la apariencia del escalado, pero no equivale a ejecutar un filtro de nitidez personalizado sobre cada frame PSP.

Fuentes:
- https://github.com/Rinnegatamante/reRescaler/issues
- https://github.com/Electry/Sharpscale

**Decisión:** no recomendar instalar ninguno de estos plugins para este experimento en la consola principal.

## Rutas técnicas consideradas

| Ruta | Ventaja | Riesgo / bloqueo | Decisión |
|---|---|---|---|
| Plugin PSP que intercepta GE | Puede modificar geometría y renderizado antes de dibujar | Requiere un punto de carga PSP válido; GePatch apunta a Adrenaline | Investigar, no implementar aún |
| Postproceso del framebuffer en Vita | Podría aplicar un filtro de nitidez al frame final sin modificar cada juego | Sincronización GPU, acceso al framebuffer de PSPemu, coste por frame y compatibilidad no demostrados | Investigar interfaces y ownership del framebuffer |
| Cambiar solo el escalado de pantalla | Menor complejidad; ya existen plugins | No garantiza nitidez real ni control por juego | No satisface por sí solo el objetivo |
| Parche individual de cada juego | Puede evitar hooks globales | Trabajo por juego y riesgo de artefactos | Fuera del primer prototipo |

## Plan de trabajo seguro

1. Identificar cómo PSPemu stock presenta el framebuffer en Vita y qué APIs/exports intervienen.
2. Determinar si existe una interfaz documentada y segura para observar el framebuffer sin modificarlo.
3. Diseñar una prueba offline con imágenes sintéticas para comparar nearest-neighbour, sharp-bilinear y unsharp mask, midiendo halos y pérdida de detalle.
4. Solo si se identifica un punto de integración viable, preparar un prototipo que pueda desactivarse sin tocar datos de juegos ni sustituir módulos del sistema.
5. Validar primero en un entorno de pruebas y con una Vita secundaria; no usar la consola principal como banco de pruebas.

## Criterios de aceptación

- No modificar archivos de PSPemu ni archivos de juegos.
- No escribir logs continuamente ni crear archivos en cada frame.
- Poder desactivar el prototipo y volver al estado anterior sin restaurar la base de datos.
- No causar GPU Crash, congelamientos ni regresiones en LiveArea.
- Comparar capturas idénticas y medir nitidez, halos, latencia y rendimiento.
- Documentar por separado compatibilidad con PSPemu stock y Adrenaline.

## Estado actual

**Investigación y auditoría documental completadas parcialmente.** No existe todavía un método confirmado para inyectar un filtro de imagen en los juegos Chovy-Sign del PSPemu stock. No hay VPK, SUPRX, SKPRX ni binario instalable en este repositorio.


## Anexo de investigación: hook de framebuffer

Se encontró una referencia adicional: OASIS OS intercepta `sceDisplaySetFrameBuf` (NID `0x289D82FE`) desde un plugin PSP para CFW, recibe el puntero del framebuffer y dibuja antes de llamar a la función original.

Código inspeccionado:
- https://github.com/AndrewAltimit/oasis-os/blob/6512f72df3602da6a487c0e01c7a0ecbaca13c40/crates/oasis-plugin-psp/src/hook.rs
- https://github.com/AndrewAltimit/rust-psp/blob/e4cc06f6fa50fa45dc3ae8b332e8a2b67625f588/psp/src/hook.rs

La implementación depende de `sctrlHENFindFunction` y `sctrlHENPatchSyscall`, es decir, de infraestructura CFW PSP. Por eso aporta una referencia de cómo observar un framebuffer en un PSP CFW, pero **no demuestra compatibilidad con el PSPemu stock de Chovy-Sign**. Tampoco prueba que el buffer pueda procesarse sin problemas de caché, sincronización o rendimiento.

La documentación detallada está en [research/FRAMEBUFFER_PIPELINE.md](../research/FRAMEBUFFER_PIPELINE.md).

**Decisión actual:** mantener abierta la investigación, pero no compilar ni instalar un hook. El siguiente bloqueo a resolver es encontrar una interfaz de observación compatible con PSPemu stock, o demostrar que existe un mecanismo seguro del lado Vita para observar su buffer sin interferir con otras aplicaciones.
