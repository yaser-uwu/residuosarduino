import { useCallback, useEffect, useState } from 'react'
import { supabase, supabaseConfigError } from '../lib/supabase'
import type { RegistroResiduo } from '../types'

/** Polling continuo: la feria se actualiza aunque Realtime falle en el celular. */
const INTERVALO_POLL_MS = 250
const LIMITE_REGISTROS = 40

interface UseRegistrosResiduosResult {
  registros: RegistroResiduo[]
  totalRegistros: number
  loading: boolean
  error: string | null
  conectado: boolean
  modoRealtime: boolean
}

export function useRegistrosResiduos(): UseRegistrosResiduosResult {
  const [registros, setRegistros] = useState<RegistroResiduo[]>([])
  const [totalRegistros, setTotalRegistros] = useState(0)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)
  const [conectado, setConectado] = useState(false)
  const [modoRealtime, setModoRealtime] = useState(false)

  const ordenarRegistros = useCallback((data: RegistroResiduo[]) => {
    return [...data].sort(
      (a, b) =>
        new Date(b.fecha_hora).getTime() - new Date(a.fecha_hora).getTime(),
    )
  }, [])

  useEffect(() => {
    if (supabaseConfigError) {
      setLoading(false)
      return
    }

    let activo = true
    let realtimeConectado = false

    const aplicarRegistros = (data: RegistroResiduo[]) => {
      setRegistros(ordenarRegistros(data))
    }

    async function cargarRegistros(silencioso = false) {
      const [lista, conteo] = await Promise.all([
        supabase
          .from('registros_residuos')
          .select('*')
          .order('fecha_hora', { ascending: false })
          .limit(LIMITE_REGISTROS),
        supabase
          .from('registros_residuos')
          .select('*', { count: 'exact', head: true }),
      ])

      if (!activo) return

      if (lista.error) {
        setError(lista.error.message)
        setLoading(false)
        return
      }

      aplicarRegistros(lista.data ?? [])
      if (!conteo.error) setTotalRegistros(conteo.count ?? 0)
      if (!silencioso) setLoading(false)
    }

    const agregarOActualizar = (registro: RegistroResiduo) => {
      setRegistros((prev) => {
        const existe = prev.find((r) => r.id === registro.id)
        const siguiente = existe
          ? prev.map((r) => (r.id === registro.id ? registro : r))
          : [registro, ...prev]
        return ordenarRegistros(siguiente).slice(0, LIMITE_REGISTROS)
      })
    }

    cargarRegistros()

    const intervaloPoll = setInterval(() => {
      cargarRegistros(true)
    }, INTERVALO_POLL_MS)

    const channel = supabase
      .channel('registros_residuos_live', {
        config: { broadcast: { self: false } },
      })
      .on(
        'postgres_changes',
        {
          event: '*',
          schema: 'public',
          table: 'registros_residuos',
        },
        (payload) => {
          if (payload.eventType === 'INSERT') {
            agregarOActualizar(payload.new as RegistroResiduo)
            setTotalRegistros((n) => n + 1)
          } else if (payload.eventType === 'UPDATE') {
            agregarOActualizar(payload.new as RegistroResiduo)
          } else if (payload.eventType === 'DELETE') {
            const eliminado = payload.old as RegistroResiduo
            setRegistros((prev) => prev.filter((r) => r.id !== eliminado.id))
            setTotalRegistros((n) => Math.max(0, n - 1))
          }
        },
      )
      .subscribe((status, err) => {
        if (!activo) return

        if (status === 'SUBSCRIBED') {
          realtimeConectado = true
          setConectado(true)
          setModoRealtime(true)
          setError(null)
        } else if (status === 'CHANNEL_ERROR') {
          setConectado(false)
          setModoRealtime(false)
          console.error('Realtime error:', err)
        } else if (status === 'CLOSED') {
          setConectado(false)
          setModoRealtime(false)
        }
      })

    const esperaRealtime = setTimeout(() => {
      if (activo && !realtimeConectado) {
        setConectado(true)
      }
    }, 2500)

    const onVisible = () => {
      if (document.visibilityState === 'visible') {
        cargarRegistros(true)
      }
    }
    document.addEventListener('visibilitychange', onVisible)

    return () => {
      activo = false
      clearTimeout(esperaRealtime)
      clearInterval(intervaloPoll)
      document.removeEventListener('visibilitychange', onVisible)
      supabase.removeChannel(channel)
    }
  }, [ordenarRegistros])

  return { registros, totalRegistros, loading, error, conectado, modoRealtime }
}
