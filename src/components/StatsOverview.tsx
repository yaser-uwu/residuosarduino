import { CATEGORIAS, CATEGORIA_CONFIG } from '../lib/categorias'
import { formatKg } from '../lib/pesos'
import type { CategoriaTacho } from '../types'

interface StatsOverviewProps {
  totalKg: number
  cantidadRegistros: number
  porcentajesPorCategoria: Record<CategoriaTacho, number>
}

export function StatsOverview({
  totalKg,
  cantidadRegistros,
  porcentajesPorCategoria,
}: StatsOverviewProps) {
  const tarjetas = [
    { titulo: 'Kilos en total', valor: formatKg(totalKg) },
    { titulo: 'Registros', valor: String(cantidadRegistros) },
    { titulo: 'Actualización', valor: 'En vivo' },
  ]

  return (
    <section className="space-y-6">
      <div>
        <h2 className="text-2xl font-bold text-stone-900">Resumen</h2>
        <p className="mt-1 text-stone-600">Datos acumulados del stand</p>
      </div>

      <div className="grid gap-4 sm:grid-cols-3">
        {tarjetas.map(({ titulo, valor }) => (
          <article
            key={titulo}
            className="rounded-2xl border-2 border-[#d4cfc4] bg-white px-5 py-5 shadow-[0_3px_0_#d4cfc4]"
          >
            <p className="text-sm font-semibold uppercase tracking-wide text-stone-500">
              {titulo}
            </p>
            <p className="mt-2 min-w-0 break-words tabular-nums text-[clamp(1.5rem,8vw,2.25rem)] font-extrabold text-stone-900">
              {valor}
            </p>
          </article>
        ))}
      </div>

      <article className="rounded-2xl border-2 border-[#d4cfc4] bg-white p-5 shadow-[0_3px_0_#d4cfc4] sm:p-6">
        <h3 className="mb-5 font-bold text-stone-900">Por tipo de residuo</h3>
        <div className="grid gap-5 sm:grid-cols-3">
          {CATEGORIAS.map((categoria) => {
            const config = CATEGORIA_CONFIG[categoria]
            const porcentaje = porcentajesPorCategoria[categoria]

            return (
              <div key={categoria}>
                <div className="mb-2 flex items-center justify-between">
                  <span className={`font-bold ${config.label}`}>
                    {categoria}
                  </span>
                  <span className="tabular-nums font-bold text-stone-800">
                    {porcentaje.toFixed(0)}%
                  </span>
                </div>
                <div className="h-3 overflow-hidden rounded-full bg-stone-100">
                  <div
                    className={`h-full rounded-full transition-[width] duration-500 ${config.progressFill}`}
                    style={{ width: `${Math.max(porcentaje, 2)}%` }}
                  />
                </div>
              </div>
            )
          })}
        </div>
      </article>
    </section>
  )
}
