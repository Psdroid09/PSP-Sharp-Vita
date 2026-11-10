# Observación automática del EBOOT.PBP abierto por PSPemu

**Estado:** implementación de diagnóstico, todavía no confirmación del juego activo.

## Cómo funciona

- ScePspemu abre EBOOT.PBP por la interfaz nativa de I/O en diversas rutas. NoPspEmuDrm usa este dato para su lógica de licencias; incluso allí se describe como una aproximación, no una identidad garantizada.
- Solo dentro del proceso ScePspemu, el probe intercepta la importación de sceIoOpen (biblioteca 0xCAE9ACE6, NID 0x6C60AC61). Llama primero a la función original y no altera argumentos ni retorno.
- Si la apertura tuvo éxito y el nombre termina exactamente en /EBOOT.PBP, analiza como máximo tres rutas distintas. El plugin realiza una apertura de lectura separada sin modificar el cursor del descriptor utilizado por el juego.
- Lee como máximo 16 KiB de cabecera y PARAM.SFO y valida formato PBP/SFO, offsets, tamaño, key CATEGORY y terminación NUL. El parser puro puede probarse sin VitaSDK.
- Registra candidato PSP (EG, MG, UG), PS1 excluido (ME) o desconocido en el informe de diagnóstico. No activa ningún filtro.

## Limitaciones que deben resolverse antes de habilitar nitidez

1. En algunas burbujas Chovy-Sign la ruta del EBOOT.PBP activo puede no observarse mediante este import.
2. Se puede abrir más de un PBP. La metadata de un archivo abierto no prueba que sea el medio que está en pantalla.
3. La lectura extra se hace solo durante la apertura del PBP; puede añadir latencia y entrar en conflicto con otros plugins de I/O (particularmente NoPspEmuDrm). Necesita validación con una consola de laboratorio.
4. El hook de compatibilidad actual observa descriptores de framebuffer, no posee el buffer ni conoce garantías de sincronización. No hay autorización para leer/escribir píxeles ni sustituir el escalado.
5. Los tests de CI no prueban comportamiento del emulador real.

## Siguiente validación en una PS Vita de laboratorio

Capturar un informe de una burbuja PSP y otro de una burbuja PS1/POPS; comparar rutas, categorías, dimensiones y flags. Comprobar arranque, suspensión, retorno al LiveArea, conflictos de hooks y recuperación. Solo después estudiar el acceso a un buffer **propio y sincronizado**, nunca escribir sobre la superficie del sistema sin demostrar que sea seguro.

**No instalar en la consola principal ni modificar ur0:tai/config.txt por ahora.**

Referencias: https://github.com/LiEnby/NoPspEmuDrm/blob/main/user/PspEmu.c ; https://www.psdevwiki.com/ps3/PBP ; https://www.psdevwiki.com/psp/PARAM.SFO
