# PSP-Sharp-Vita

Proyecto de investigación para estudiar si es posible mejorar la nitidez de los juegos de PSP ejecutados en PS Vita mediante el emulador oficial de PSP.

## Objetivo

Investigar una solución de nitidez que conserve los detalles de la imagen y que, si resulta técnicamente viable, pueda utilizarse con juegos PSP instalados como burbujas mediante **Chovy-Sign**, sin depender de Adrenaline.

## Prioridades

1. Seguridad y estabilidad en PS Vita.
2. No modificar ni reemplazar archivos del emulador oficial.
3. No requerir cambios innecesarios en `ur0:tai/config.txt`.
4. Evitar procesos en segundo plano, registros y escrituras innecesarias.
5. Documentar claramente qué está comprobado y qué sigue siendo experimental.

## Estado

**Investigación técnica en curso.** Ya existe un prototipo de diagnóstico nativo que compila con VitaSDK en GitHub Actions. El prototipo solo observa metadatos de una llamada de presentación de PSPemu; no aplica filtros y no ha sido probado en hardware.

La auditoría de referencias sigue sin encontrar una solución que demuestre un filtro personalizado seguro para burbujas Chovy-Sign. Consulta `research/TECHNICAL_FEASIBILITY.md`, `research/REFERENCE_AUDIT.md` y `research/PSPEMU_PLUGIN_SCOPE.md`.

## Seguridad

**No instales archivos de este repositorio en tu PS Vita todavía.** La compilación de CI no equivale a una prueba de funcionamiento ni de seguridad en una consola real. Cualquier prueba futura deberá incluir instrucciones de instalación, desinstalación y recuperación.

No se ha modificado `ur0:tai/config.txt`, los plugins instalados ni el contenido de SD2Vita.

## Compatibilidad objetivo

- PS Vita 1000 y 2000 como objetivos de investigación.
- Firmware 3.60 con Ensō como entorno de referencia inicial.
- Juegos PSP ejecutados mediante el emulador oficial, incluidos los instalados con Chovy-Sign.

La compatibilidad real se determinará mediante pruebas; estos puntos son objetivos, no garantías.

## Herramientas

- `tools/filter_lab.py`: laboratorio offline para comparar nearest-neighbour, bilinear, sharp-bilinear aproximado y unsharp mask en PC.
- `src/diagnostic/`: prototipo de observación PSPemu-only, sin lectura ni escritura de píxeles.

Consulta los README de cada carpeta para instrucciones y limitaciones.

## Investigación técnica

- [Auditoría SceCompat/SceDisplay](research/COMPAT_FRAMEBUFFER_API.md)
- [Alcance del plugin PSPemu](research/PSPEMU_PLUGIN_SCOPE.md)
- [Auditoría de referencias](research/REFERENCE_AUDIT.md)
- [Prototipo de diagnóstico](src/diagnostic/README.md)
- [Plan de validación en hardware (solo lectura)](research/HARDWARE_VALIDATION_PLAN.md)

El import candidato es `SceDisplaySetFrameBufForCompat`. El wrapper de firmware mapea el framebuffer de origen al quinto argumento y el destino opcional de conversión al sexto. El prototipo llama primero a la función original y solo inspecciona metadatos del descriptor de origen tras una llamada exitosa. No lee el área de píxeles ni modifica argumentos.

**Bloqueadores pendientes:** demostrar que juegos PSP reales envían un descriptor útil, validar su memoria/propiedad/sincronización y medir el coste antes de implementar cualquier filtro.