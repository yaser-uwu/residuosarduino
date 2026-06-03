import { createClient } from '@supabase/supabase-js'

const supabaseUrl = import.meta.env.VITE_SUPABASE_URL as string | undefined
const supabaseAnonKey = import.meta.env.VITE_SUPABASE_ANON_KEY as string | undefined

export const supabaseConfigError = (() => {
  if (!supabaseUrl || !supabaseAnonKey) {
    return 'Faltan variables de entorno. Copia .env.example a .env y configura VITE_SUPABASE_URL y VITE_SUPABASE_ANON_KEY.'
  }

  if (
    supabaseAnonKey === 'tu_clave_aqui' ||
    supabaseUrl.includes('TU_PROYECTO')
  ) {
    return 'Configura tus credenciales de Supabase en el archivo .env antes de continuar.'
  }

  return null
})()

export const supabase = createClient(
  supabaseUrl ?? '',
  supabaseAnonKey ?? '',
)
