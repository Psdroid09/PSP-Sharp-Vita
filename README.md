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

<badge>Investigación inicial</badge>

Este repositorio todavía no contiene un plugin instalable ni una compilación funcional. No hay evidencia suficiente para afirmar que un filtro personalizado pueda inyectarse de forma segura en el emulador oficial de PSP.

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
