# 🚀 Deploy en Netlify - FrioSeguro

## Opción 1: Deploy desde la interfaz web (más fácil)

1. Ir a: https://app.netlify.com/drop
2. Arrastrar la carpeta `web-app/` completa
3. Esperar que suba
4. ¡Listo! Te dará una URL como `https://random-name.netlify.app`

## Opción 2: Conectar con GitHub

1. Subir el proyecto a GitHub
2. Ir a https://app.netlify.com
3. Click en "Add new site" → "Import an existing project"
4. Seleccionar el repositorio
5. Configurar:
   - **Base directory**: `web-app`
   - **Publish directory**: `web-app`
6. Click en "Deploy"

## Configuración ya incluida

- ✅ `netlify.toml` - Configuración de build
- ✅ `manifest.json` - PWA manifest
- ✅ `.gitignore` - Archivos a ignorar
- ✅ Supabase ya configurado en el código

## URLs de prueba

Una vez deployado, la web mostrará:
- Dispositivos de **Campamento Parametican** (REEFER-01)
- Dispositivos de **Carnicería Demo** (CARNICERIA-01, 02, 03)

## Datos de prueba

Ya hay datos en Supabase:
- 2 organizaciones
- 4 dispositivos
- 1 lectura de prueba

---

**Supabase Project**: https://supabase.com/dashboard/project/xhdeacnwdzvkivfjzard
