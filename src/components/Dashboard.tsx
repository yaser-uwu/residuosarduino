import { Loader2, Recycle } from 'lucide-react'
import { useMemo } from 'react'
import { BinCard } from './BinCard'
import { CategoryChart } from './CategoryChart'
import { ConfigError } from './ConfigError'
import { RecentRecordsTable } from './RecentRecordsTable'
import { StatsOverview } from './StatsOverview'
import { useRegistrosResiduos } from '../hooks/useRegistrosResiduos'
import { CATEGORIAS } from '../lib/categorias'
import { pesoActualPorCategoria, totalPesoActual } from '../lib/pesos'
import { supabaseConfigError } from '../lib/supabase'
import type { CategoriaTacho } from '../types'

export function Dashboard() {
  const configError = supabaseConfigError
  const { registros, loading, error, conectado, modoRealtime } =
    useRegistrosResiduos()
  const errorMostrado = configError ?? error

  const pesosPorCategoria = useMemo(
    () => pesoActualPorCategoria(registros),
    [registros],
  )

  const totalKg = useMemo(
    () => totalPesoActual(pesosPorCategoria),
    [pesosPorCategoria],
  )

  const porcentajesPorCategoria = useMemo(() => {
    const porcentajes: Record<CategoriaTacho, number> = {
      Vidrio: 0,
      Plástico: 0,
      Papel: 0,
    }

    if (totalKg === 0) return porcentajes

    for (const categoria of CATEGORIAS) {
      porcentajes[categoria] = (pesosPorCategoria[categoria] / totalKg) * 100
    }

    return porcentajes
  }, [pesosPorCategoria, totalKg])

  return (
    <div className="min-h-screen">
      <header className="bg-[#1a5c38] text-white">
        <div className="mx-auto flex max-w-6xl flex-col gap-4 px-4 py-6 sm:flex-row sm:items-center sm:justify-between sm:px-6">
          <div className="flex items-center gap-4">
            <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-white/15">
              <Recycle className="h-7 w-7" strokeWidth={2} />
            </div>
            <div>
              <h1 className="text-2xl font-extrabold tracking-tight sm:text-3xl">
                Tachos de reciclaje
              </h1>
              <p className="mt-0.5 text-sm font-medium text-green-100/90 sm:text-base">
                Stand de feria · pesaje en tiempo real
              </p>
            </div>
          </div>

          <div
            className={`inline-flex w-fit items-center gap-2 rounded-full px-4 py-2 text-sm font-bold ${
              conectado
                ? 'bg-white text-[#1a5c38]'
                : 'bg-amber-400 text-amber-950'
            }`}
          >
            <span
              className={`h-2.5 w-2.5 rounded-full ${
                conectado ? 'bg-emerald-500' : 'bg-amber-800'
              }`}
            />
            {modoRealtime
              ? 'En vivo'
              : conectado
                ? 'Sincronizando'
                : 'Actualizando cada 1 s'}
          </div>
        </div>
      </header>

      <main className="mx-auto max-w-6xl space-y-10 px-4 py-8 sm:px-6 sm:py-10">
        {loading && !configError && (
          <div className="flex items-center justify-center gap-3 rounded-2xl border-2 border-[#d4cfc4] bg-white py-16 font-semibold text-stone-600 shadow-[0_3px_0_#d4cfc4]">
            <Loader2 className="h-6 w-6 animate-spin text-[#1a5c38]" />
            Cargando datos…
          </div>
        )}

        {errorMostrado && <ConfigError error={errorMostrado} />}

        {!loading && !errorMostrado && (
          <>
            <section>
              <div className="mb-5">
                <h2 className="text-2xl font-bold text-stone-900">Los tres tachos</h2>
                <p className="mt-1 text-stone-600">
                  Verde vidrio · azul plástico · amarillo papel
                </p>
              </div>
              <div className="grid gap-5 md:grid-cols-2 lg:grid-cols-3">
                {CATEGORIAS.map((categoria) => (
                  <BinCard
                    key={categoria}
                    categoria={categoria}
                    pesoActual={pesosPorCategoria[categoria]}
                  />
                ))}
              </div>
            </section>

            <StatsOverview
              totalKg={totalKg}
              cantidadRegistros={registros.length}
              porcentajesPorCategoria={porcentajesPorCategoria}
            />

            <div className="grid gap-6 lg:grid-cols-2">
              <CategoryChart pesosPorCategoria={pesosPorCategoria} />
              <RecentRecordsTable registros={registros} />
            </div>
          </>
        )}
      </main>
    </div>
  )
}
