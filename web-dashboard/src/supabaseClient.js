// Configuración de Supabase
const SUPABASE_URL = 'https://xhdeacnwdzvkivfjzard.supabase.co';
const SUPABASE_KEY = 'sb_publishable_JhTUv1X2LHMBVILUaysJ3g_Ho11zu-Q';

const headers = {
  'apikey': SUPABASE_KEY,
  'Authorization': `Bearer ${SUPABASE_KEY}`,
  'Content-Type': 'application/json',
  'Prefer': 'return=representation'
};

// GET request
async function apiGet(endpoint) {
  const response = await fetch(`${SUPABASE_URL}${endpoint}`, { headers });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

// PATCH request
async function apiPatch(endpoint, data) {
  const response = await fetch(`${SUPABASE_URL}${endpoint}`, {
    method: 'PATCH',
    headers,
    body: JSON.stringify(data)
  });
  return response.ok;
}

// Obtener todos los dispositivos con su última lectura
export async function getDevicesWithReadings() {
  try {
    const devices = await apiGet('/rest/v1/devices?select=*&order=device_id');
    
    // Obtener última lectura de cada dispositivo
    const devicesWithReadings = await Promise.all(devices.map(async (device) => {
      try {
        const readings = await apiGet(`/rest/v1/readings?device_id=eq.${device.device_id}&order=created_at.desc&limit=1`);
        const reading = readings[0] || null;
        
        return {
          ...device,
          reading: reading ? {
            tempAvg: reading.temp_avg,
            temp1: reading.temp1,
            temp2: reading.temp2,
            humidity: reading.humidity,
            door1Open: reading.door1_open,
            acPower: reading.ac_power,
            alertActive: reading.alert_active,
            defrostMode: reading.defrost_mode,
            relayOn: reading.relay_on,
            uptimeSec: reading.uptime_sec,
            createdAt: reading.created_at
          } : null
        };
      } catch (e) {
        return { ...device, reading: null };
      }
    }));
    
    return devicesWithReadings;
  } catch (e) {
    console.error('Error fetching devices:', e);
    return [];
  }
}

// Obtener configuración de un dispositivo
export async function getDeviceConfig(deviceId) {
  try {
    const devices = await apiGet(`/rest/v1/devices?device_id=eq.${deviceId}&limit=1`);
    if (devices.length === 0) return null;
    
    const d = devices[0];
    return {
      deviceId: d.device_id,
      name: d.name,
      location: d.location,
      tempMax: d.temp_max,
      tempCritical: d.temp_critical,
      alertDelaySec: d.alert_delay_sec,
      doorOpenMaxSec: d.door_open_max_sec,
      defrostCooldownSec: d.defrost_cooldown_sec,
      telegramEnabled: d.telegram_enabled,
      supabaseEnabled: d.supabase_enabled
    };
  } catch (e) {
    console.error('Error fetching config:', e);
    return null;
  }
}

// Actualizar configuración de un dispositivo
export async function updateDeviceConfig(deviceId, config) {
  try {
    return await apiPatch(`/rest/v1/devices?device_id=eq.${deviceId}`, config);
  } catch (e) {
    console.error('Error updating config:', e);
    return false;
  }
}

// Obtener alertas activas
export async function getActiveAlerts() {
  try {
    return await apiGet('/rest/v1/alerts?resolved=eq.false&order=created_at.desc&limit=50');
  } catch (e) {
    console.error('Error fetching alerts:', e);
    return [];
  }
}

// Reconocer alerta
export async function acknowledgeAlert(alertId) {
  return await apiPatch(`/rest/v1/alerts?id=eq.${alertId}`, { acknowledged: true });
}

// Resolver alerta
export async function resolveAlert(alertId) {
  return await apiPatch(`/rest/v1/alerts?id=eq.${alertId}`, { resolved: true, resolved_at: new Date().toISOString() });
}
