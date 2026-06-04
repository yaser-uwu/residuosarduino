import { FileText, Package, Wine } from 'lucide-react'
import type { CategoriaTacho } from '../types'
import { CATEGORIA_CONFIG, MAX_PESO_TACHO } from '../lib/categorias'
import { formatKg } from '../lib/pesos'

const ICONOS = {
  Vidrio: Wine,
  Plástico: Package,
  Papel: FileText,
} as const

interface BinCardProps {
  categoria: CategoriaTacho
  pesoActual: number
}

export function BinCard({ categoria, pesoActual }: BinCardProps) {
  const config = CATEGORIA_CONFIG[categoria]
  const Icono = ICONOS[categoria]
  const porcentaje = Math.min((pesoActual / MAX_PESO_TACHO) * 100, 100)

  return (
    <article className="min-w-0 overflow-hidden rounded-2xl border-2 border-[#d4cfc4] bg-white shadow-[0_4px_0_#d4cfc4]">
      <div className={`h-3 ${config.accent}`} />

      <div className="p-5 sm:p-6">
        <div className="mb-5 flex items-center gap-4">
          <div
            className={`flex h-14 w-14 shrink-0 items-center justify-center rounded-xl text-white ${config.accent}`}
          >
            <Icono className="h-7 w-7" strokeWidth={2.25} />
          </div>
          <div>
            <h3 className={`text-xl font-bold sm:text-2xl ${config.label}`}>
              {categoria}
            </h3>
            <p className="text-sm font-medium text-stone-500">
              Capacidad {MAX_PESO_TACHO} kg
            </p>
          </div>
        </div>

        <p className="flex min-w-0 flex-wrap items-baseline gap-x-2 gap-y-0 tabular-nums text-[clamp(2rem,12vw,3.75rem)] font-extrabold leading-none tracking-tight text-stone-900">
          {formatKg(pesoActual, false)}
          <span className="text-[clamp(1.125rem,5vw,1.5rem)] font-bold text-stone-500">
            kg
          </span>
        </p>

        <div className="mt-6">
          <div className="mb-2 flex justify-between text-sm font-semibold text-stone-600">
            <span>Llenado</span>
            <span className={config.label}>{porcentaje.toFixed(1)}%</span>
          </div>
          <div className="h-4 overflow-hidden rounded-full bg-stone-100">
            <div
              className={`h-full rounded-full transition-[width] duration-500 ${config.progressFill}`}
              style={{ width: `${Math.max(porcentaje, porcentaje > 0 ? 3 : 0)}%` }}
            />
          </div>
        </div>
      </div>
    </article>
  )
}
