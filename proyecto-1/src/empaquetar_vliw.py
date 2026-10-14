
import re
from pathlib import Path

ORIGINAL = Path("salida_resuelta.asm")
ENTRADA = Path("salida_para_binario.asm")
SALIDA = Path("salida_paralela.asm")

PATRON_LABEL = re.compile(r"^[A-Za-z_]\w*:$")
PATRON_SLOT = re.compile(r"^(ALU|LSU|BRU|FEIST)\s*:", re.I)

PATRON_LOAD = re.compile(
    r"^LSU:\s*LOAD\s+x(\d+),\s*"
    r"[-+]?(?:0[xX][0-9a-fA-F]+|\d+)"
    r"\s*\(x(\d+)\)$", re.I
)

PATRON_ADDI = re.compile(
    r"^ALU:\s*ADDI\s+x(\d+),\s*x0,\s*"
    r"[-+]?(?:0[xX][0-9a-fA-F]+|\d+)$", re.I
)


def leer_posiciones_etiquetas():
    """Conservar los destinos de los saltos originales."""
    destinos = set()
    indice = 0

    for linea in ORIGINAL.read_text(encoding="utf-8").splitlines():
        texto = linea.split("#", 1)[0].split("//", 1)[0].strip()

        if not texto:
            continue

        if PATRON_LABEL.fullmatch(texto):
            destinos.add(indice)
        elif PATRON_SLOT.match(texto):
            indice += 1
        else:
            raise ValueError(f"Linea ASM no reconocida: {texto}")

    return destinos, indice


def empaquetar():
    destinos, cantidad_original = leer_posiciones_etiquetas()

    instrucciones = [
        linea.strip()
        for linea in ENTRADA.read_text(encoding="utf-8").splitlines()
        if linea.strip()
    ]

    if len(instrucciones) != cantidad_original:
        raise ValueError("No coinciden las cantidades de bundles")

    fusiones = 0

    for i in range(len(instrucciones) - 1):
        load = PATRON_LOAD.fullmatch(instrucciones[i])
        addi = PATRON_ADDI.fullmatch(instrucciones[i + 1])

        if not load or not addi:
            continue

        reg_load = int(load.group(1))
        reg_base = int(load.group(2))
        reg_addi = int(addi.group(1))

        # Solo registros temporales ordinarios.
        if not 4 <= reg_addi <= 12:
            continue

        # Evitar dependencias dentro del mismo bundle.
        if reg_addi in (reg_load, reg_base):
            continue

        # No mover una instruccion que sea destino de salto.
        if (i + 1) in destinos:
            continue

        instrucciones[i] += " | " + instrucciones[i + 1]

        # Conservar el numero de bundles y las direcciones.
        instrucciones[i + 1] = "ALU: NOP"

        fusiones += 1

    SALIDA.write_text(
        "\n".join(instrucciones) + "\n",
        encoding="utf-8"
    )

    print(f"Bundles originales: {cantidad_original}")
    print(f"Bundles finales: {len(instrucciones)}")
    print(f"Bundles con ALU + LSU: {fusiones}")
    print(f"Archivo generado: {SALIDA}")


if __name__ == "__main__":
    empaquetar()
