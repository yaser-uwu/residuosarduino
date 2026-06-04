import { normalizarCategoria } from './categorias'
import type { CategoriaTacho, RegistroResiduo } from '../types'

/** Misma precisión que el LCD del ESP32 (3 decimales, ej. 144 g → 0.144 kg). */
export function formatKg(kg: number, conUnidad = true): string {
  const valor = kg.toFixed(3)
  return conUnidad ? `${valor} kg` : valor
}

/** Último peso reportado por el ESP32 por categoría (mismo valor que muestra el LCD). */
export function pesoActualPorCategoria(
  registros: RegistroResiduo[],
): Record<CategoriaTacho, number> {
  const pesos: Record<CategoriaTacho, number> = {
    Vidrio: 0,
    Plástico: 0,
    Papel: 0,
  }
  const yaVisto = new Set<CategoriaTacho>()

  for (const registro of registros) {
    const categoria = normalizarCategoria(registro.categoria)
    if (categoria && !yaVisto.has(categoria)) {
      pesos[categoria] = Number(registro.peso) || 0
      yaVisto.add(categoria)
    }
  }

  return pesos
}

export function totalPesoActual(
  pesos: Record<CategoriaTacho, number>,
): number {
  return Object.values(pesos).reduce((sum, p) => sum + p, 0)
}
