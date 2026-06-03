export interface RegistroResiduo {
  id: number
  categoria: string
  peso: number
  fecha_hora: string
  usuario: string
}

export type CategoriaTacho = 'Vidrio' | 'Plástico' | 'Papel'

export interface CategoriaConfig {
  nombre: CategoriaTacho
  accent: string
  accentLight: string
  accentBorder: string
  progressFill: string
  chartColor: string
  label: string
}
