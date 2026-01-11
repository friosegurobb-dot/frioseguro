#!/usr/bin/env python3
"""
test_supabase.py - Verificar conexión y datos en Supabase
Sistema Monitoreo Reefer - Campamento Parametican Silver

Uso:
    python test_supabase.py              # Ver estado general
    python test_supabase.py --insert     # Insertar dato de prueba
    python test_supabase.py --readings   # Ver últimas lecturas
    python test_supabase.py --alerts     # Ver alertas activas
    python test_supabase.py --devices    # Ver dispositivos
    python test_supabase.py --monitor    # Monitoreo en tiempo real
"""

import os
import sys
import json
import time
import argparse
from datetime import datetime, timedelta

try:
    import requests
except ImportError:
    print("Instalando requests...")
    os.system("pip install requests")
    import requests

# ============================================================================
# CONFIGURACIÓN SUPABASE
# ============================================================================
SUPABASE_URL = "https://xhdeacnwdzvkivfjzard.supabase.co"
SUPABASE_ANON_KEY = "sb_publishable_JhTUv1X2LHMBVILUaysJ3g_Ho11zu-Q"

# Headers para todas las peticiones
HEADERS = {
    "apikey": SUPABASE_ANON_KEY,
    "Authorization": f"Bearer {SUPABASE_ANON_KEY}",
    "Content-Type": "application/json",
    "Prefer": "return=representation"
}

# ============================================================================
# FUNCIONES DE API
# ============================================================================

def api_get(table, params=None):
    """GET request a Supabase"""
    url = f"{SUPABASE_URL}/rest/v1/{table}"
    try:
        response = requests.get(url, headers=HEADERS, params=params, timeout=10)
        if response.status_code == 200:
            return response.json()
        else:
            print(f"Error {response.status_code}: {response.text}")
            return None
    except Exception as e:
        print(f"Error de conexión: {e}")
        return None

def api_post(table, data):
    """POST request a Supabase"""
    url = f"{SUPABASE_URL}/rest/v1/{table}"
    try:
        response = requests.post(url, headers=HEADERS, json=data, timeout=10)
        if response.status_code in [200, 201]:
            return response.json()
        else:
            print(f"Error {response.status_code}: {response.text}")
            return None
    except Exception as e:
        print(f"Error de conexión: {e}")
        return None

def api_patch(table, match, data):
    """PATCH request a Supabase"""
    url = f"{SUPABASE_URL}/rest/v1/{table}?{match}"
    try:
        response = requests.patch(url, headers=HEADERS, json=data, timeout=10)
        return response.status_code in [200, 204]
    except Exception as e:
        print(f"Error de conexión: {e}")
        return False

# ============================================================================
# FUNCIONES DE VERIFICACIÓN
# ============================================================================

def check_connection():
    """Verificar conexión a Supabase"""
    print("\n" + "="*60)
    print("🔌 VERIFICANDO CONEXIÓN A SUPABASE")
    print("="*60)
    print(f"URL: {SUPABASE_URL}")
    
    # Intentar obtener dispositivos
    devices = api_get("devices", {"select": "device_id,name", "limit": "1"})
    
    if devices is not None:
        print("✅ Conexión exitosa!")
        return True
    else:
        print("❌ No se pudo conectar a Supabase")
        print("\nVerifica:")
        print("  1. URL y API Key correctos")
        print("  2. Tablas creadas (ejecutar schema_v2.sql)")
        print("  3. Políticas RLS configuradas")
        return False

def show_devices():
    """Mostrar todos los dispositivos"""
    print("\n" + "="*60)
    print("📱 DISPOSITIVOS REGISTRADOS")
    print("="*60)
    
    devices = api_get("devices", {
        "select": "device_id,name,location,is_online,last_seen_at,wifi_rssi",
        "order": "device_id"
    })
    
    if not devices:
        print("No hay dispositivos registrados")
        return
    
    for d in devices:
        online = "🟢" if d.get('is_online') else "🔴"
        last_seen = d.get('last_seen_at', 'Nunca')
        if last_seen and last_seen != 'Nunca':
            # Parsear fecha
            try:
                dt = datetime.fromisoformat(last_seen.replace('Z', '+00:00'))
                diff = datetime.now(dt.tzinfo) - dt
                if diff.total_seconds() < 60:
                    last_seen = f"hace {int(diff.total_seconds())}s"
                elif diff.total_seconds() < 3600:
                    last_seen = f"hace {int(diff.total_seconds()/60)}min"
                else:
                    last_seen = f"hace {int(diff.total_seconds()/3600)}h"
            except:
                pass
        
        print(f"\n{online} {d['device_id']} - {d.get('name', 'Sin nombre')}")
        print(f"   📍 {d.get('location', 'Sin ubicación')}")
        print(f"   📶 WiFi: {d.get('wifi_rssi', '--')} dBm | Visto: {last_seen}")

def show_readings(device_id=None, limit=10):
    """Mostrar últimas lecturas"""
    print("\n" + "="*60)
    print("🌡️ ÚLTIMAS LECTURAS")
    print("="*60)
    
    params = {
        "select": "device_id,temp_avg,humidity,door1_open,ac_power,alert_active,created_at",
        "order": "created_at.desc",
        "limit": str(limit)
    }
    
    if device_id:
        params["device_id"] = f"eq.{device_id}"
    
    readings = api_get("readings", params)
    
    if not readings:
        print("No hay lecturas registradas")
        return
    
    print(f"\n{'Dispositivo':<12} {'Temp':>8} {'Hum':>6} {'Puerta':>8} {'Luz':>6} {'Alerta':>8} {'Hora':<20}")
    print("-" * 80)
    
    for r in readings:
        temp = f"{r.get('temp_avg', '--'):.1f}°C" if r.get('temp_avg') else "--"
        hum = f"{r.get('humidity', '--'):.0f}%" if r.get('humidity') else "--"
        door = "ABIERTA" if r.get('door1_open') else "cerrada"
        power = "✓" if r.get('ac_power', True) else "⚡SIN"
        alert = "🚨 SI" if r.get('alert_active') else "OK"
        
        created = r.get('created_at', '')[:19].replace('T', ' ')
        
        print(f"{r['device_id']:<12} {temp:>8} {hum:>6} {door:>8} {power:>6} {alert:>8} {created:<20}")

def show_alerts(active_only=True):
    """Mostrar alertas"""
    print("\n" + "="*60)
    print("🚨 ALERTAS" + (" ACTIVAS" if active_only else ""))
    print("="*60)
    
    params = {
        "select": "device_id,alert_type,severity,message,acknowledged,resolved,created_at",
        "order": "created_at.desc",
        "limit": "20"
    }
    
    if active_only:
        params["resolved"] = "eq.false"
    
    alerts = api_get("alerts", params)
    
    if not alerts:
        print("✅ No hay alertas" + (" activas" if active_only else ""))
        return
    
    for a in alerts:
        severity_icon = {
            'emergency': '🔴',
            'critical': '🟠',
            'warning': '🟡',
            'info': '🔵'
        }.get(a.get('severity', ''), '⚪')
        
        ack = "✓ ACK" if a.get('acknowledged') else ""
        resolved = "✓ RESUELTO" if a.get('resolved') else ""
        
        print(f"\n{severity_icon} [{a.get('severity', 'unknown').upper()}] {a['device_id']}")
        print(f"   Tipo: {a.get('alert_type', '--')}")
        print(f"   Mensaje: {a.get('message', '--')}")
        print(f"   Fecha: {a.get('created_at', '')[:19]} {ack} {resolved}")

def insert_test_reading(device_id="REEFER-01"):
    """Insertar lectura de prueba"""
    print("\n" + "="*60)
    print("📝 INSERTANDO LECTURA DE PRUEBA")
    print("="*60)
    
    import random
    
    data = {
        "device_id": device_id,
        "temp1": round(-20 + random.uniform(-2, 2), 2),
        "temp2": round(-19 + random.uniform(-2, 2), 2),
        "temp_avg": round(-19.5 + random.uniform(-2, 2), 2),
        "humidity": round(45 + random.uniform(-5, 5), 1),
        "door1_open": random.choice([True, False, False, False]),  # 25% probabilidad abierta
        "ac_power": True,
        "relay_on": False,
        "alert_active": False,
        "defrost_mode": False,
        "simulation_mode": True,  # Marcar como simulación
        "wifi_rssi": random.randint(-70, -40),
        "uptime_sec": random.randint(1000, 100000)
    }
    
    print(f"Dispositivo: {device_id}")
    print(f"Temperatura: {data['temp_avg']}°C")
    print(f"Humedad: {data['humidity']}%")
    print(f"Puerta: {'ABIERTA' if data['door1_open'] else 'cerrada'}")
    
    result = api_post("readings", data)
    
    if result:
        print("\n✅ Lectura insertada correctamente!")
        print(f"ID: {result[0].get('id') if result else 'N/A'}")
        return True
    else:
        print("\n❌ Error al insertar lectura")
        return False

def insert_test_alert(device_id="REEFER-01"):
    """Insertar alerta de prueba"""
    print("\n" + "="*60)
    print("🚨 INSERTANDO ALERTA DE PRUEBA")
    print("="*60)
    
    data = {
        "device_id": device_id,
        "alert_type": "temperature",
        "severity": "warning",
        "message": f"Prueba de alerta desde Python - {datetime.now().strftime('%H:%M:%S')}",
        "temperature": -8.5,
        "acknowledged": False,
        "resolved": False,
        "telegram_sent": False,
        "sms_sent": False
    }
    
    result = api_post("alerts", data)
    
    if result:
        print("✅ Alerta insertada correctamente!")
        return True
    else:
        print("❌ Error al insertar alerta")
        return False

def monitor_realtime(device_id=None, interval=5):
    """Monitoreo en tiempo real"""
    print("\n" + "="*60)
    print("📊 MONITOREO EN TIEMPO REAL")
    print("="*60)
    print(f"Actualizando cada {interval} segundos. Ctrl+C para salir.\n")
    
    last_id = 0
    
    try:
        while True:
            # Obtener última lectura
            params = {
                "select": "*",
                "order": "created_at.desc",
                "limit": "1"
            }
            if device_id:
                params["device_id"] = f"eq.{device_id}"
            
            readings = api_get("readings", params)
            
            if readings and len(readings) > 0:
                r = readings[0]
                current_id = r.get('id', 0)
                
                # Limpiar pantalla (solo en terminales que lo soporten)
                print("\033[H\033[J", end="")
                
                print("="*60)
                print(f"📊 MONITOREO - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
                print("="*60)
                
                # Estado
                is_new = "🆕" if current_id != last_id else "  "
                last_id = current_id
                
                temp = r.get('temp_avg', 0)
                temp_color = "🔵" if temp < -15 else ("🟡" if temp < -10 else "🔴")
                
                print(f"\n{is_new} Dispositivo: {r['device_id']}")
                print(f"   {temp_color} Temperatura: {temp:.1f}°C")
                print(f"   💧 Humedad: {r.get('humidity', '--')}%")
                print(f"   🚪 Puerta: {'ABIERTA 🔓' if r.get('door1_open') else 'Cerrada 🔒'}")
                print(f"   ⚡ Luz: {'OK ✓' if r.get('ac_power', True) else 'SIN LUZ ⚠️'}")
                print(f"   🚨 Alerta: {'ACTIVA ⚠️' if r.get('alert_active') else 'Normal ✓'}")
                print(f"   📶 WiFi: {r.get('wifi_rssi', '--')} dBm")
                print(f"\n   Última lectura: {r.get('created_at', '')[:19]}")
                print("\n" + "-"*60)
                print("Presiona Ctrl+C para salir")
            
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print("\n\n👋 Monitoreo detenido")

def show_stats():
    """Mostrar estadísticas generales"""
    print("\n" + "="*60)
    print("📈 ESTADÍSTICAS GENERALES")
    print("="*60)
    
    # Contar lecturas
    readings = api_get("readings", {"select": "id", "limit": "1", "order": "id.desc"})
    if readings:
        print(f"\n📊 Total lecturas: ~{readings[0].get('id', 0):,}")
    
    # Contar alertas
    alerts_active = api_get("alerts", {"select": "id", "resolved": "eq.false"})
    alerts_total = api_get("alerts", {"select": "id", "limit": "1", "order": "id.desc"})
    
    print(f"🚨 Alertas activas: {len(alerts_active) if alerts_active else 0}")
    if alerts_total:
        print(f"🚨 Total alertas: ~{alerts_total[0].get('id', 0):,}")
    
    # Dispositivos online
    devices = api_get("devices", {"select": "device_id,is_online"})
    if devices:
        online = sum(1 for d in devices if d.get('is_online'))
        print(f"\n📱 Dispositivos: {len(devices)} total, {online} online")

# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='Verificar datos en Supabase')
    parser.add_argument('--insert', action='store_true', help='Insertar lectura de prueba')
    parser.add_argument('--alert', action='store_true', help='Insertar alerta de prueba')
    parser.add_argument('--readings', action='store_true', help='Ver últimas lecturas')
    parser.add_argument('--alerts', action='store_true', help='Ver alertas activas')
    parser.add_argument('--devices', action='store_true', help='Ver dispositivos')
    parser.add_argument('--monitor', action='store_true', help='Monitoreo en tiempo real')
    parser.add_argument('--stats', action='store_true', help='Ver estadísticas')
    parser.add_argument('--device', type=str, default=None, help='Filtrar por device_id')
    parser.add_argument('--limit', type=int, default=10, help='Límite de resultados')
    
    args = parser.parse_args()
    
    print("\n" + "="*60)
    print("🏔️ SISTEMA MONITOREO REEFER - Verificador Supabase")
    print("   Campamento Parametican Silver")
    print("="*60)
    
    # Verificar conexión primero
    if not check_connection():
        sys.exit(1)
    
    # Ejecutar acción solicitada
    if args.insert:
        insert_test_reading(args.device or "REEFER-01")
    elif args.alert:
        insert_test_alert(args.device or "REEFER-01")
    elif args.readings:
        show_readings(args.device, args.limit)
    elif args.alerts:
        show_alerts()
    elif args.devices:
        show_devices()
    elif args.monitor:
        monitor_realtime(args.device)
    elif args.stats:
        show_stats()
    else:
        # Mostrar todo
        show_devices()
        show_readings(limit=5)
        show_alerts()
        show_stats()
    
    print("\n")

if __name__ == "__main__":
    main()
