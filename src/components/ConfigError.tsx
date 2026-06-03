import { AlertCircle, KeyRound } from 'lucide-react'
import { supabaseBuildInfo } from '../lib/supabase'

interface ConfigErrorProps {
  error: string
}

export function ConfigError({ error }: ConfigErrorProps) {
  const esClaveInvalida =
    error.toLowerCase().includes('invalid api key') ||
    error.toLowerCase().includes('api key')

  const esUrlInvalida =
    error.toLowerCase().includes('invalid path') ||
    error.toLowerCase().includes('vite_supabase') ||
    error.toLowerCase().includes('url') ||
    error.toLowerCase().includes('vercel')

  return (
    <div className="mx-auto max-w-2xl rounded-2xl border border-red-200 bg-red-50 p-6 text-left shadow-sm">
      <div className="mb-4 flex items-start gap-3">
        <AlertCircle className="mt-0.5 h-6 w-6 shrink-0 text-red-600" />
        <div>
          <h2 className="text-lg font-semibold text-red-800">
            Error de conexión con Supabase
          </h2>
          <p className="mt-1 text-sm text-red-700">{error}</p>
        </div>
      </div>

      {(esClaveInvalida || esUrlInvalida) && (
        <div className="space-y-4 rounded-xl border border-red-100 bg-white p-5">
          <div className="flex items-center gap-2 text-slate-800">
            <KeyRound className="h-5 w-5 text-slate-500" />
            <p className="font-medium">
              {esUrlInvalida ? 'Variables en Vercel' : 'Cómo corregir la clave API'}
            </p>
          </div>

          <ol className="list-decimal space-y-2 pl-5 text-sm text-slate-600">
            <li>
              En Vercel: tu proyecto → <strong>Settings</strong> →{' '}
              <strong>Environment Variables</strong>.
            </li>
            <li>
              <strong>VITE_SUPABASE_URL</strong> = solo esto (sin barra al final):
              <br />
              <code className="mt-1 block rounded bg-slate-100 px-2 py-1 text-xs">
                https://fllgcqincjxqavpfmmbi.supabase.co
              </code>
            </li>
            <li>
              <strong>VITE_SUPABASE_ANON_KEY</strong> = clave publishable completa (
              <code className="rounded bg-slate-100 px-1">sb_publishable_...</code>
              ).
            </li>
            <li>
              Marcá las tres casillas: Production, Preview y Development.
            </li>
            <li>
              <strong>Deployments</strong> → los tres puntos del último deploy →{' '}
              <strong>Redeploy</strong> (obligatorio después de cambiar variables).
            </li>
          </ol>

          <p className="text-xs text-slate-500">
            No intercambies URL y clave. No pegues /rest/v1 en la URL.
          </p>

          <div className="mt-4 rounded-lg bg-slate-50 p-3 text-xs text-slate-600">
            <p className="font-semibold text-slate-800">Diagnóstico del build actual:</p>
            <ul className="mt-2 space-y-1">
              <li>
                URL en build:{' '}
                {supabaseBuildInfo.tieneUrl
                  ? `sí → ${supabaseBuildInfo.host}`
                  : 'no (falta Redeploy)'}
              </li>
              <li>
                Clave en build:{' '}
                {supabaseBuildInfo.tieneKey
                  ? supabaseBuildInfo.keyOk
                    ? 'sí, formato OK'
                    : 'sí, pero formato raro'
                  : 'no (falta Redeploy)'}
              </li>
            </ul>
          </div>
        </div>
      )}
    </div>
  )
}
