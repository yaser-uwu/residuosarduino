import { useEffect, useMemo, useState } from 'react'
import { esp32Activo } from '../lib/esp32'
import type { RegistroResiduo } from '../types'

/** Reevalúa cada 1 s si el ESP sigue enviando (para mostrar 0 kg si se apagó). */
export function useEsp32Activo(registros: RegistroResiduo[]) {
  const [ahora, setAhora] = useState(() => Date.now())

  useEffect(() => {
    const id = setInterval(() => setAhora(Date.now()), 1000)
    return () => clearInterval(id)
  }, [])

  return useMemo(() => esp32Activo(registros, ahora), [registros, ahora])
}
