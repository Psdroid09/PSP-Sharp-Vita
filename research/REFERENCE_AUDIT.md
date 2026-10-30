# Auditoría de referencias existentes

## GePatch (TheFloW)

- URL: https://github.com/TheOfficialFloW/GePatch
- Tipo: plugin PRX del entorno PSP.
- Requisito declarado: Adrenaline 7 o superior.
- Función: intercepta/parchea comandos del Graphics Engine para intentar renderizado a resolución nativa.
- Limitaciones declaradas: escenas/cutscenes ausentes, pantallas negras, texturas incompletas y clipping/culling.
- Uso para PSP-Sharp-Vita: referencia técnica del GE; no se asume compatibilidad con Chovy-Sign.

## GePatchUltra

- URL: https://github.com/alangrewco/GePatchUltra
- Fork de GePatch con una arquitectura de transferencia PSP→Vita descrita en su README.
- Auditoría de fuente:
  - Las llamadas de búsqueda y parcheo de syscalls GE en pspemu_plugin/main.c están comentadas.
  - module_start() únicamente escribe un archivo ACK y retorna.
  - vita_renderer/main.c solo contiene funciones de inicio/parada vacías.
  - La infraestructura de renderizado GXM y el transporte completo siguen pendientes según el propio roadmap.
- Veredicto: prototipo incompleto. No usar sus binarios como solución ni inferir funcionalidad de su README.

## reRescaler

- URL: https://github.com/Rinnegatamante/reRescaler
- Función: reemplaza métodos de escalado y filtrado.
- Incidencia: existe un reporte abierto sobre cierres de juegos PSP al abrir ajustes desde el menú ImPOSE.
- Veredicto: no instalar en la Vita principal para este proyecto.

## Sharpscale

- URL: https://github.com/Electry/Sharpscale
- Función: cambia el método de escalado del framebuffer hacia la pantalla.
- Veredicto: útil para entender el escalado de salida, pero no demuestra un filtro postproceso específico para Chovy-Sign.

## Chovy-Sign

- URL: https://github.com/LiEnby/chovy-sign
- Función: firma/construye contenido PSP/PS1 para ejecutarse como contenido en Vita sin modificar.
- Veredicto: el objetivo de compatibilidad es el PSPemu stock; no debe confundirse con el entorno CFW PSP de Adrenaline.

## Fuentes técnicas generales

- VitaSDK: https://vitasdk.org/
- PSP Emulator research wiki: https://www.psdevwiki.com/vita/PSP_Emulator
- Adrenaline overview: https://github.com/TheOfficialFloW/Adrenaline
