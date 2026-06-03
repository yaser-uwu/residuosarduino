import { createClient } from '@supabase/supabase-js'

function normalizarUrl(raw: string): string {
  let url = raw.trim()
  if (!url.startsWith('http://') && !url.startsWith('https://')) {
    url = `https://${url}`
  }
  url = url.replace(/\/rest\/v1\/?$/i, '')
  url = url.replace(/\/+$/, '')
  return url
}

const urlCruda = import.meta.env.VITE_SUPABASE_URL as string | undefined
const keyCruda = import.meta.env.VITE_SUPABASE_ANON_KEY as string | undefined

const supabaseUrl = urlCruda ? normalizarUrl(urlCruda) : undefined
const supabaseAnonKey = keyCruda?.trim()

/** Qué quedó grabado en el build (Vite no lee .env en runtime en producción). */
export const supabaseBuildInfo = {
  tieneUrl: Boolean(urlCruda?.trim()),
  tieneKey: Boolean(supabaseAnonKey),
  host: (() => {
    try {
      return supabaseUrl ? new URL(supabaseUrl).hostname : null
    } catch {
      return null
    }
  })(),
  keyOk: Boolean(
    supabaseAnonKey?.startsWith('sb_publishable_') ||
      supabaseAnonKey?.startsWith('eyJ'),
  ),
}

export const supabaseConfigError = (() => {
  if (!urlCruda?.trim() || !supabaseAnonKey) {
    return 'Las variables no entraron en el build de Vercel. Agregalas en Environment Variables y hacé Redeploy (con limpiar caché).'
  }

  if (
    supabaseAnonKey === 'tu_clave_aqui' ||
    urlCruda.includes('TU_PROYECTO')
  ) {
    return 'Configura las credenciales reales de Supabase (no los valores de ejemplo).'
  }

  if (
    urlCruda.startsWith('sb_') ||
    urlCruda.startsWith('eyJ') ||
    urlCruda.length > 120
  ) {
    return 'VITE_SUPABASE_URL debe ser la URL del proyecto (https://xxx.supabase.co), no la API key.'
  }

  if (supabaseAnonKey.includes('supabase.co')) {
    return 'VITE_SUPABASE_ANON_KEY debe ser la clave publishable (sb_publishable_...), no la URL.'
  }

  try {
    const parsed = new URL(supabaseUrl!)
    if (!parsed.hostname.endsWith('supabase.co')) {
      return `VITE_SUPABASE_URL incorrecta en el build: ${parsed.hostname}`
    }
  } catch {
    return 'VITE_SUPABASE_URL no es una URL válida.'
  }

  if (!supabaseBuildInfo.keyOk) {
    return 'VITE_SUPABASE_ANON_KEY no parece una clave válida (debe empezar con sb_publishable_ o eyJ).'
  }

  return null
})()

export const supabase = createClient(
  supabaseUrl ?? '',
  supabaseAnonKey ?? '',
)
