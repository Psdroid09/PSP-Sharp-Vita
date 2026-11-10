# Detección automática PSP / PS1 — avance experimental

## Objetivo estricto

PSP-Sharp-Vita debe filtrar **solo juegos PSP** lanzados con el
PSPemu oficial en PS Vita 1000 3.60 + Ensō. No debe modificar PS1/POPS,
Adrenaline, LiveArea ni aplicaciones nativas de Vita. No se requiere introducir
ningún Title ID ni mantener una lista por juego.

## Hallazgo: categoría del contenido

El campo `CATEGORY` en el `PARAM.SFO` del contenido PSP puede indicar:

| CATEGORY | Interpretación | Política |
| --- | --- | --- |
| `EG` | Juego PSP digital / minis | Candidato PSP, todavía no autorizado |
| `MG` | Juego/aplicación de Memory Stick PSP | Candidato PSP, todavía no autorizado |
| `UG` | Juego PSP UMD | Candidato PSP, todavía no autorizado |
| `ME` | PS1 Classic / POPS | Excluido obligatoriamente |
| Cualquier otro / sin lectura | No verificado | Desactivado |

Referencias: https://www.psdevwiki.com/ps3/PBP y https://docs.vitasdk.org/group__SceAppMgrUser.html.

**Importante:** `sceAppMgrAppParamGetString(0, SCE_APPMGR_APP_PARAM_CATEGORY, ...)`
lee la metadata del **proceso actual**. Como PSP y PS1 comparten ScePspemu,
esa categoría podría describir NPXS10028 y **no** el EBOOT.PBP realmente
lanzado. La lectura por sí sola NO constituye un detector PSP/PS1 fiable.

## Prototipo agregado

- `src/common/psp_scope.h`: clasificador sin dependencias ni acceso a archivos.
- `tests/psp_scope_test.c`: pruebas de regresión, incluidos PS1 y errores.
- El probe PSPemu agrega la lectura del campo CATEGORY de AppMgr al
  informe de diagnóstico, **solo para observar** qué valor ofrece el sistema.
- El probe sigue siendo de solo lectura. No hay filtrado, cambios de flags,
  acceso a píxeles ni cambios de memoria de juegos.
- El permiso de aplicar un futuro filtro **siempre** permanece denegado mientras
  no se verifique independientemente que la categoría corresponde al medio PSP
  activo. No usamos nombres de Title ID para dar ese permiso.

## Bloqueo pendiente

Confirmar en PSPemu stock cómo acceder a la metadata del **EBOOT.PBP de la
sesión activa** sin interceptar globalmente llamadas de archivo ni introducir
conflictos con otros plugins. Después contrastar observaciones de **PSP y PS1**
en hardware de prueba. Si AppMgr solo devuelve la categoría del proceso
NPXS10028, esta ruta no servirá para autorizar el filtro.

El workflow de GitHub Actions comprueba compilación de PSPSharpProbe.suprx y
las pruebas host del clasificador. Un build verde **no** valida el detector en
hardware ni significa que exista un filtro listo para instalar.

## Seguridad

No instalar el SUPRX experimental en la Vita principal ni editar
`ur0:tai/config.txt` por ahora. No se publicará como filtro funcional hasta
tener pruebas de identidad de juego, lectura segura del framebuffer,
sincronización, rendimiento y arranque/salida sin regresiones.
