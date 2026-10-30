# Laboratorio offline de PSP-Sharp-Vita

## Objetivo

Comparar métodos de escalado y nitidez sobre una misma imagen PSP sin instalar nada en la consola.

## Requisitos

- Python 3.10 o posterior
- Pillow

Instalación:

```bash
python -m pip install Pillow
```

## Prueba rápida

Desde la raíz del repositorio:

```bash
python tools/filter_lab.py --synthetic --output comparison.png
```

Esto genera una imagen sintética de referencia de 480×272 y una hoja comparativa 2×2 a 960×544.

También puedes usar una captura propia:

```bash
python tools/filter_lab.py ruta/a/tu/captura.png --output comparison.png
```

Para probar escalados 3× o 4×:

```bash
python tools/filter_lab.py captura.png --scale 3 --output comparison-3x.png
```

## Métodos comparados

1. **Nearest neighbour:** conserva valores de píxel, pero puede producir escalones.
2. **Bilinear:** suaviza transiciones y puede reducir detalle fino.
3. **Sharp-bilinear (aproximación):** bilinear con realce suave de bordes. No es una implementación exacta de un shader sharp-bilinear.
4. **Unsharp mask:** realce más fuerte para hacer visibles halos, ruido y contornos artificiales.

El script imprime un indicador sencillo de diferencias horizontales. No es una puntuación de calidad: un valor mayor puede significar más detalle o simplemente más aliasing/ruido.

## Cómo evaluar

- Usa exactamente la misma captura y escala para cada método.
- Observa texto pequeño, líneas diagonales, texturas y bordes de alto contraste.
- Rechaza resultados con halos claros, ringing, grano amplificado o pérdida de detalle.
- No elijas un método solo porque parezca más contrastado.
- El filtro no puede recuperar información que el juego no renderizó.

## Límites

Este laboratorio procesa archivos en PC. No lee el framebuffer de PSPemu, no se comunica con la Vita y no valida rendimiento en hardware real. El resultado no es un plugin ni un VPK.
