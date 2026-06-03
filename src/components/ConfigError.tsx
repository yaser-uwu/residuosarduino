import { AlertCircle, ExternalLink, KeyRound } from 'lucide-react'

interface ConfigErrorProps {
  error: string
}

export function ConfigError({ error }: ConfigErrorProps) {
  const esClaveInvalida =
    error.toLowerCase().includes('invalid api key') ||
    error.toLowerCase().includes('api key')

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

      {esClaveInvalida && (
        <div className="space-y-4 rounded-xl border border-red-100 bg-white p-5">
          <div className="flex items-center gap-2 text-slate-800">
            <KeyRound className="h-5 w-5 text-slate-500" />
            <p className="font-medium">Cómo corregir la clave API</p>
          </div>

          <ol className="list-decimal space-y-2 pl-5 text-sm text-slate-600">
            <li>
              Entra a tu proyecto en{' '}
              <a
                href="https://supabase.com/dashboard/project/fllgcqincjxqavpfmmbi/settings/api-keys"
                target="_blank"
                rel="noreferrer"
                className="inline-flex items-center gap-1 font-medium text-indigo-600 hover:underline"
              >
                Supabase → Settings → API Keys
                <ExternalLink className="h-3.5 w-3.5" />
              </a>
            </li>
            <li>
              Copia la clave <strong>Publishable</strong> (
              <code className="rounded bg-slate-100 px-1">sb_publishable_...</code>
              ) o la clave legacy <strong>anon</strong> (
              <code className="rounded bg-slate-100 px-1">eyJ...</code>
              ).
            </li>
            <li>
              Pégala en el archivo <code className="rounded bg-slate-100 px-1">.env</code>{' '}
              como <code className="rounded bg-slate-100 px-1">VITE_SUPABASE_ANON_KEY</code>.
            </li>
            <li>
              Reinicia el servidor con{' '}
              <code className="rounded bg-slate-100 px-1">npm run dev</code>.
            </li>
          </ol>

          <p className="text-xs text-slate-500">
            La clave actual no es reconocida por Supabase. Verifica que esté
            completa, sin espacios extra, y que corresponda al proyecto{' '}
            <code className="rounded bg-slate-100 px-1">fllgcqincjxqavpfmmbi</code>.
          </p>
        </div>
      )}
    </div>
  )
}
