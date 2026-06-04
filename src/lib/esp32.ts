import type { RegistroResiduo } from '../types'

/** Sin datos del ESP32 en este tiempo → stand apagado o sin Wi‑Fi (heartbeat ~2 s). */
export const ESP32_INACTIVO_MS = 10_000

export function ultimoEnvioEsp32(registros: RegistroResiduo[]): number | null {
  let ultimo: number | null = null

  for (const registro of registros) {
    if (registro.usuario?.toLowerCase() !== 'esp32') continue
    const t = new Date(registro.fecha_hora).getTime()
    if (Number.isNaN(t)) continue
    if (ultimo === null || t > ultimo) ultimo = t
  }

  return ultimo
}

export function esp32Activo(
  registros: RegistroResiduo[],
  ahora = Date.now(),
): boolean {
  const ultimo = ultimoEnvioEsp32(registros)
  if (ultimo === null) return false
  return ahora - ultimo <= ESP32_INACTIVO_MS
}
