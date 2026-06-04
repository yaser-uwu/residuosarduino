import type { RegistroResiduo } from '../types'
import { normalizarCategoria, CATEGORIA_CONFIG } from '../lib/categorias'
import { formatKg } from '../lib/pesos'

interface RecentRecordsTableProps {
  registros: RegistroResiduo[]
}

function formatearFecha(fecha: string) {
  return new Intl.DateTimeFormat('es-AR', {
    day: '2-digit',
    month: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  }).format(new Date(fecha))
}

export function RecentRecordsTable({ registros }: RecentRecordsTableProps) {
  const ultimos = registros.slice(0, 10)

  return (
    <section className="min-w-0 max-w-full rounded-2xl border-2 border-[#d4cfc4] bg-white p-5 shadow-[0_3px_0_#d4cfc4] sm:p-6">
      <div className="mb-5">
        <h2 className="text-2xl font-bold text-stone-900">Actividad reciente</h2>
        <p className="mt-1 text-stone-600">Últimos 10 pesajes</p>
      </div>

      {ultimos.length === 0 ? (
        <p className="rounded-xl bg-stone-50 py-10 text-center font-medium text-stone-500">
          Cuando alguien use un tacho, el registro aparece acá.
        </p>
      ) : (
        <>
          <ul className="space-y-3 md:hidden">
            {ultimos.map((registro) => {
              const categoria = normalizarCategoria(registro.categoria)
              const config = categoria ? CATEGORIA_CONFIG[categoria] : null

              return (
                <li
                  key={registro.id}
                  className="rounded-xl border border-stone-200 bg-stone-50/80 p-4"
                >
                  <div className="flex flex-wrap items-center justify-between gap-2">
                    <span
                      className={`rounded-lg px-3 py-1 text-sm font-bold ${
                        config
                          ? config.accentLight
                          : 'bg-stone-100 text-stone-700'
                      }`}
                    >
                      {registro.categoria}
                    </span>
                    <span className="tabular-nums text-lg font-bold text-stone-900">
                      {formatKg(Number(registro.peso))}
                    </span>
                  </div>
                  <p className="mt-2 text-sm font-medium text-stone-600">
                    {formatearFecha(registro.fecha_hora)}
                    {registro.usuario ? ` · ${registro.usuario}` : ''}
                  </p>
                </li>
              )
            })}
          </ul>

          <div className="hidden min-w-0 overflow-x-auto md:block">
            <table className="w-full text-left">
              <thead>
                <tr className="border-b-2 border-stone-200 text-sm font-semibold text-stone-500">
                  <th className="pb-3 pr-4">Tipo</th>
                  <th className="pb-3 pr-4">Peso</th>
                  <th className="pb-3 pr-4">Hora</th>
                  <th className="pb-3">Usuario</th>
                </tr>
              </thead>
              <tbody>
                {ultimos.map((registro) => {
                  const categoria = normalizarCategoria(registro.categoria)
                  const config = categoria ? CATEGORIA_CONFIG[categoria] : null

                  return (
                    <tr
                      key={registro.id}
                      className="border-b border-stone-100 last:border-0"
                    >
                      <td className="py-3.5 pr-4">
                        <span
                          className={`inline-block rounded-lg px-3 py-1 text-sm font-bold ${
                            config
                              ? config.accentLight
                              : 'bg-stone-100 text-stone-700'
                          }`}
                        >
                          {registro.categoria}
                        </span>
                      </td>
                      <td className="py-3.5 pr-4 tabular-nums text-lg font-bold text-stone-900">
                        {formatKg(Number(registro.peso))}
                      </td>
                      <td className="py-3.5 pr-4 font-medium text-stone-600">
                        {formatearFecha(registro.fecha_hora)}
                      </td>
                      <td className="py-3.5 text-stone-600">
                        {registro.usuario || '—'}
                      </td>
                    </tr>
                  )
                })}
              </tbody>
            </table>
          </div>
        </>
      )}
    </section>
  )
}
