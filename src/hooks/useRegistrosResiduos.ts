import { useCallback, useEffect, useState } from 'react'
import { supabase, supabaseConfigError } from '../lib/supabase'
import type { RegistroResiduo } from '../types'

/** Solo si Realtime no conecta (respaldo rápido para la feria). */
const INTERVALO_RESPALDO_MS = 1000

interface UseRegistrosResiduosResult {
  registros: RegistroResiduo[]
  loading: boolean
  error: string | null
  conectado: boolean
  modoRealtime: boolean
}

export function useRegistrosResiduos(): UseRegistrosResiduosResult {
  const [registros, setRegistros] = useState<RegistroResiduo[]>([])
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
    let intervaloRespaldo: ReturnType<typeof setInterval> | undefined

    const aplicarRegistros = (data: RegistroResiduo[]) => {
      setRegistros(ordenarRegistros(data))
    }

    async function cargarRegistros(silencioso = false) {
      const { data, error: fetchError } = await supabase
        .from('registros_residuos')
        .select('*')
        .order('fecha_hora', { ascending: false })

      if (!activo) return

      if (fetchError) {
        setError(fetchError.message)
        setLoading(false)
        return
      }

      aplicarRegistros(data ?? [])
      if (!silencioso) setLoading(false)
    }

    const agregarOActualizar = (registro: RegistroResiduo) => {
      setRegistros((prev) => {
        const existe = prev.find((r) => r.id === registro.id)
        const siguiente = existe
          ? prev.map((r) => (r.id === registro.id ? registro : r))
          : [registro, ...prev]
        return ordenarRegistros(siguiente)
      })
    }

    const iniciarRespaldo = () => {
      if (intervaloRespaldo) return
      intervaloRespaldo = setInterval(() => {
        cargarRegistros(true)
      }, INTERVALO_RESPALDO_MS)
    }

    const detenerRespaldo = () => {
      if (intervaloRespaldo) {
        clearInterval(intervaloRespaldo)
        intervaloRespaldo = undefined
      }
    }

    cargarRegistros()

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
          } else if (payload.eventType === 'UPDATE') {
            agregarOActualizar(payload.new as RegistroResiduo)
          } else if (payload.eventType === 'DELETE') {
            const eliminado = payload.old as RegistroResiduo
            setRegistros((prev) => prev.filter((r) => r.id !== eliminado.id))
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
          detenerRespaldo()
        } else if (status === 'CHANNEL_ERROR') {
          setConectado(false)
          setModoRealtime(false)
          iniciarRespaldo()
          console.error('Realtime error:', err)
        } else if (status === 'CLOSED') {
          setConectado(false)
          setModoRealtime(false)
          iniciarRespaldo()
        }
      })

    const esperaRealtime = setTimeout(() => {
      if (activo && !realtimeConectado) {
        iniciarRespaldo()
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
      detenerRespaldo()
      document.removeEventListener('visibilitychange', onVisible)
      supabase.removeChannel(channel)
    }
  }, [ordenarRegistros])

  return { registros, loading, error, conectado, modoRealtime }
}
