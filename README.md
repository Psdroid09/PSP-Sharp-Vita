# PSP-Sharp-Vita

Proyecto de investigación para estudiar si es posible mejorar la nitidez de los juegos de PSP ejecutados en PS Vita mediante el emulador oficial de PSP.

## Objetivo

Investigar una solución de nitidez que conserve los detalles de la imagen y que, si resulta técnicamente viable, pueda utilizarse con juegos PSP instalados como burbujas mediante **Chovy-Sign**.

## Prioridades

1. Seguridad y estabilidad en PS Vita.
2. No modificar ni reemplazar archivos del emulador oficial.
3. No requerir cambios innecesarios en `ur0:tai/config.txt`.
4. Evitar procesos en segundo plano, registros y escrituras innecesarias.
5. Documentar claramente qué está comprobado y qué sigue siendo experimental.

## Alcance inicial

- Investigar cómo se escala y presenta la imagen de PSP en el entorno oficial de Vita.
- Revisar las interfaces disponibles para plugins de Vita y sus límites en el proceso PSP.
- Evaluar por separado las opciones existentes de escalado y filtrado.
- Diseñar pruebas reversibles antes de considerar cualquier instalación en una consola real.

## Estado

**Investigación técnica en curso.**

La auditoría inicial encontró que las soluciones existentes más cercanas trabajan en Adrenaline o cambian el escalado global; ninguna demuestra todavía un filtro personalizado seguro para burbujas Chovy-Sign en el PSPemu oficial. Consulta la evaluación de viabilidad en docs/TECHNICAL_FEASIBILITY.md y la auditoría de referencias en research/REFERENCE_AUDIT.md.

## Seguridad

**No instales archivos de este repositorio en tu PS Vita mientras no exista una versión publicada, revisada y probada.** Las pruebas iniciales deben realizarse fuera de la consola siempre que sea posible. Cualquier prueba en hardware deberá incluir instrucciones de instalación, desinstalación y recuperación.

## Compatibilidad objetivo

- PS Vita 1000 y 2000.
- Firmware 3.60 con Ensō como entorno de referencia inicial.
- Juegos PSP ejecutados mediante el emulador oficial, incluidos los instalados con Chovy-Sign.

La compatibilidad real se determinará mediante pruebas; estos puntos son objetivos, no garantías.

## Estructura prevista

```
docs/       Investigación técnica y notas de compatibilidad
research/   Referencias y experimentos reproducibles
src/        Código, solo cuando exista un enfoque técnicamente validado
tests/      Pruebas y herramientas de verificación
```

## Idioma

La documentación principal de este proyecto se mantiene en español.


## Investigación del framebuffer

Se añadió una auditoría de las APIs SceCompat y SceDisplay para determinar si existe una vía segura de observar el framebuffer PSP en PSPemu stock: [research/COMPAT_FRAMEBUFFER_API.md](research/COMPAT_FRAMEBUFFER_API.md). La conclusión sigue siendo que no hay un punto de integración demostrado para Chovy-Sign; no hay plugin ni VPK instalable.

## Laboratorio de filtros (PC)

Ya está disponible una herramienta offline para comparar nearest-neighbour, bilinear, una aproximación sharp-bilinear y unsharp mask con la misma imagen:

- [Guía del laboratorio](tools/README.md)
- [Script de comparación](tools/filter_lab.py)

Prueba rápida: `python -m pip install Pillow` y después `python tools/filter_lab.py --synthetic --output comparison.png`.

El laboratorio solo procesa imágenes en PC. No es un plugin y no accede al PSPemu ni a la PS Vita.


## Investigación del PSPemu stock

Nueva auditoría de carga de plugins por proceso, identidad `NPXS10028`/`ScePspemu`, patrón `*ALL` + `taiGetModuleInfo` de NoPspEmuDrm y advertencias de compatibilidad de remaPSV: [research/PSPEMU_PLUGIN_SCOPE.md](research/PSPEMU_PLUGIN_SCOPE.md). Se confirmó una ruta de detección del proceso PSPemu y se identificó que el tercer argumento del import `SceDisplaySetFrameBufForCompat` contiene el descriptor del framebuffer de origen. Es el candidato principal para una prueba de solo lectura; aún falta validar acceso, sincronización y rendimiento. No hay filtro ni VPK instalable. No instalar plugins ni editar `ur0:tai/config.txt` por ahora.

