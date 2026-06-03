import type { CategoriaConfig, CategoriaTacho } from '../types'

export const CATEGORIAS: CategoriaTacho[] = ['Vidrio', 'Plástico', 'Papel']

export const MAX_PESO_TACHO = 10

export const CATEGORIA_CONFIG: Record<CategoriaTacho, CategoriaConfig> = {
  Vidrio: {
    nombre: 'Vidrio',
    accent: 'bg-emerald-600',
    accentLight: 'bg-emerald-50 text-emerald-900',
    accentBorder: 'border-emerald-600',
    progressFill: 'bg-emerald-500',
    chartColor: '#059669',
    label: 'text-emerald-800',
  },
  Plástico: {
    nombre: 'Plástico',
    accent: 'bg-blue-600',
    accentLight: 'bg-blue-50 text-blue-900',
    accentBorder: 'border-blue-600',
    progressFill: 'bg-blue-500',
    chartColor: '#2563eb',
    label: 'text-blue-800',
  },
  Papel: {
    nombre: 'Papel',
    accent: 'bg-amber-500',
    accentLight: 'bg-amber-50 text-amber-950',
    accentBorder: 'border-amber-500',
    progressFill: 'bg-amber-400',
    chartColor: '#d97706',
    label: 'text-amber-900',
  },
}

export function normalizarCategoria(categoria: string): CategoriaTacho | null {
  const mapa: Record<string, CategoriaTacho> = {
    vidrio: 'Vidrio',
    plastico: 'Plástico',
    plástico: 'Plástico',
    papel: 'Papel',
  }

  return mapa[categoria.toLowerCase().trim()] ?? null
}
