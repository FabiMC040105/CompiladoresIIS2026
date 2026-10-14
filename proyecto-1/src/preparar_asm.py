
import re
from pathlib import Path

ENTRADA = Path("salida_resuelta.asm")
SALIDA = Path("salida_para_binario.asm")

# Identifica etiquetas simbólicas como inicio_while_2:
PATRON_ETIQUETA = re.compile(r"^[a-zA-Z_][a-zA-Z0-9_]*:$")

# Identifica los cuatro tipos de unidad funcional.
PATRON_UNIDAD = re.compile(r"^(ALU|LSU|BRU|FEIST)\s*:", re.I)


def preparar_asm():
    if not ENTRADA.exists():
        raise FileNotFoundError(
            "No existe salida_resuelta.asm. Primero ejecuta make run."
        )

    instrucciones = []

    for numero, linea in enumerate(ENTRADA.read_text().splitlines(), 1):
        # Quitar comentarios y espacios.
        texto = linea.split("#", 1)[0].split("//", 1)[0].strip()

        if not texto:
            continue

        # Las etiquetas ya no son necesarias después de resolverlas.
        if PATRON_ETIQUETA.fullmatch(texto):
            continue

        # Conservar únicamente instrucciones de unidades funcionales.
        if not PATRON_UNIDAD.match(texto):
            raise ValueError(
                f"Línea {numero} no reconocida: {texto}"
            )

        instrucciones.append(texto)

    SALIDA.write_text(
        "\n".join(instrucciones) + "\n",
        encoding="utf-8"
    )

    print("Ensamblador preparado correctamente.")
    print(f"Bundles conservados: {len(instrucciones)}")
    print(f"Archivo generado: {SALIDA}")


if __name__ == "__main__":
    preparar_asm()
