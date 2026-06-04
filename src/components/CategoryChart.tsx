import {
  Bar,
  BarChart,
  CartesianGrid,
  Cell,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from 'recharts'
import { CATEGORIAS, CATEGORIA_CONFIG } from '../lib/categorias'
import type { CategoriaTacho } from '../types'

interface CategoryChartProps {
  pesosPorCategoria: Record<CategoriaTacho, number>
}

export function CategoryChart({ pesosPorCategoria }: CategoryChartProps) {
  const data = CATEGORIAS.map((categoria) => ({
    categoria,
    peso: Number(pesosPorCategoria[categoria].toFixed(2)),
    color: CATEGORIA_CONFIG[categoria].chartColor,
  }))

  return (
    <section className="min-w-0 max-w-full rounded-2xl border-2 border-[#d4cfc4] bg-white p-5 shadow-[0_3px_0_#d4cfc4] sm:p-6">
      <div className="mb-6">
        <h2 className="text-2xl font-bold text-stone-900">Kilos por tacho</h2>
        <p className="mt-1 text-stone-600">Comparación entre categorías</p>
      </div>

      <div className="h-72 min-w-0 w-full max-w-full sm:h-80">
        <ResponsiveContainer width="100%" height="100%">
          <BarChart data={data} margin={{ top: 12, right: 8, left: 0, bottom: 4 }}>
            <CartesianGrid stroke="#e7e5e4" strokeDasharray="4 4" vertical={false} />
            <XAxis
              dataKey="categoria"
              tick={{ fill: '#44403c', fontSize: 15, fontWeight: 600 }}
              axisLine={{ stroke: '#d6d3d1' }}
              tickLine={false}
            />
            <YAxis
              tick={{ fill: '#78716c', fontSize: 13 }}
              axisLine={false}
              tickLine={false}
              unit=" kg"
            />
            <Tooltip
              formatter={(value) => [
                `${Number(value).toFixed(2)} kg`,
                'Peso',
              ]}
              contentStyle={{
                borderRadius: '10px',
                border: '2px solid #d4cfc4',
                fontFamily: 'Figtree, sans-serif',
                fontWeight: 600,
              }}
            />
            <Bar dataKey="peso" radius={[6, 6, 0, 0]} maxBarSize={80}>
              {data.map((entry) => (
                <Cell key={entry.categoria} fill={entry.color} />
              ))}
            </Bar>
          </BarChart>
        </ResponsiveContainer>
      </div>
    </section>
  )
}
