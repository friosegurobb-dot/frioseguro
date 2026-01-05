-- ============================================
-- SCHEMA PARA FRIOSEGURO - MONITOREO EN LA NUBE
-- Supabase Project: xhdeacnwdzvkivfjzard
-- Soporta: Campamento Parametican + Carnicerías
-- ============================================

-- Tabla de organizaciones/clientes
CREATE TABLE IF NOT EXISTS organizations (
  id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
  slug TEXT UNIQUE NOT NULL,                 -- "parametican", "carniceria-don-pedro"
  name TEXT NOT NULL,                        -- "Campamento Parametican Silver"
  type TEXT NOT NULL DEFAULT 'reefer',       -- "reefer", "carniceria", "restaurante"
  logo_url TEXT,
  contact_email TEXT,
  contact_phone TEXT,
  created_at TIMESTAMPTZ DEFAULT NOW(),
  config JSONB DEFAULT '{}'::jsonb           -- Configuración general
);

-- Tabla de dispositivos (cada ESP32/Reefer)
CREATE TABLE IF NOT EXISTS devices (
  id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
  device_id TEXT UNIQUE NOT NULL,           -- ID único del dispositivo (ej: "REEFER-001")
  org_id UUID REFERENCES organizations(id), -- A qué organización pertenece
  name TEXT NOT NULL,                        -- Nombre amigable (ej: "Freezer Principal")
  location TEXT,                             -- Ubicación (ej: "Local Centro")
  location_detail TEXT,                      -- Detalle (ej: "Cocina - Freezer 1")
  latitude FLOAT,
  longitude FLOAT,
  created_at TIMESTAMPTZ DEFAULT NOW(),
  last_seen TIMESTAMPTZ DEFAULT NOW(),
  is_online BOOLEAN DEFAULT false,
  alerts_disabled BOOLEAN DEFAULT false,     -- MODO DESCONGELAMIENTO - desactiva alertas
  alerts_disabled_until TIMESTAMPTZ,         -- Hasta cuándo están desactivadas
  firmware_version TEXT,
  temp_max FLOAT DEFAULT -18.0,              -- Umbral de alerta
  temp_critical FLOAT DEFAULT -10.0,         -- Umbral crítico
  door_open_max_sec INT DEFAULT 180,         -- Máx tiempo puerta abierta
  alert_delay_sec INT DEFAULT 300,           -- Delay antes de alertar
  config JSONB DEFAULT '{}'::jsonb           -- Configuración adicional
);

-- Tabla de lecturas (datos de sensores cada 5 seg)
CREATE TABLE IF NOT EXISTS readings (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
  temp1 FLOAT,                               -- Temperatura sensor 1
  temp2 FLOAT,                               -- Temperatura sensor 2
  temp_avg FLOAT,                            -- Promedio
  humidity FLOAT,                            -- Humedad (si hay DHT)
  door_open BOOLEAN DEFAULT false,           -- Estado puerta
  door_open_seconds INT DEFAULT 0,           -- Segundos puerta abierta
  relay_on BOOLEAN DEFAULT false,            -- Estado relay/sirena
  alert_active BOOLEAN DEFAULT false,        -- Hay alerta activa?
  simulation_mode BOOLEAN DEFAULT false,     -- Está en modo simulación?
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Tabla de alertas (historial de alertas)
CREATE TABLE IF NOT EXISTS alerts (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
  alert_type TEXT NOT NULL,                  -- 'temperature', 'door', 'offline'
  severity TEXT NOT NULL,                    -- 'warning', 'critical'
  message TEXT NOT NULL,
  temp_value FLOAT,
  acknowledged BOOLEAN DEFAULT false,
  acknowledged_at TIMESTAMPTZ,
  acknowledged_by TEXT,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Tabla de comandos (para enviar comandos al ESP desde la nube)
CREATE TABLE IF NOT EXISTS commands (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
  command TEXT NOT NULL,                     -- 'stop_alert', 'toggle_relay', 'set_config'
  payload JSONB DEFAULT '{}'::jsonb,
  executed BOOLEAN DEFAULT false,
  executed_at TIMESTAMPTZ,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Índices para mejor performance
CREATE INDEX IF NOT EXISTS idx_readings_device_time ON readings(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_device_time ON alerts(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_commands_device_pending ON commands(device_id, executed) WHERE executed = false;

-- Función para actualizar last_seen del dispositivo
CREATE OR REPLACE FUNCTION update_device_last_seen()
RETURNS TRIGGER AS $$
BEGIN
  UPDATE devices 
  SET last_seen = NOW(), is_online = true 
  WHERE device_id = NEW.device_id;
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Trigger para actualizar last_seen cuando llega una lectura
DROP TRIGGER IF EXISTS trigger_update_last_seen ON readings;
CREATE TRIGGER trigger_update_last_seen
  AFTER INSERT ON readings
  FOR EACH ROW
  EXECUTE FUNCTION update_device_last_seen();

-- Función para marcar dispositivos offline (sin datos por 2 min)
CREATE OR REPLACE FUNCTION mark_devices_offline()
RETURNS void AS $$
BEGIN
  UPDATE devices 
  SET is_online = false 
  WHERE last_seen < NOW() - INTERVAL '2 minutes';
END;
$$ LANGUAGE plpgsql;

-- Limpiar lecturas viejas (mantener solo últimas 24 horas para ahorrar espacio)
CREATE OR REPLACE FUNCTION cleanup_old_readings()
RETURNS void AS $$
BEGIN
  DELETE FROM readings WHERE created_at < NOW() - INTERVAL '24 hours';
END;
$$ LANGUAGE plpgsql;

-- RLS (Row Level Security) - Permitir acceso público de lectura
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;
ALTER TABLE readings ENABLE ROW LEVEL SECURITY;
ALTER TABLE alerts ENABLE ROW LEVEL SECURITY;
ALTER TABLE commands ENABLE ROW LEVEL SECURITY;

-- Políticas de acceso público (usando anon key)
CREATE POLICY "Allow public read devices" ON devices FOR SELECT USING (true);
CREATE POLICY "Allow public insert devices" ON devices FOR INSERT WITH CHECK (true);
CREATE POLICY "Allow public update devices" ON devices FOR UPDATE USING (true);

CREATE POLICY "Allow public read readings" ON readings FOR SELECT USING (true);
CREATE POLICY "Allow public insert readings" ON readings FOR INSERT WITH CHECK (true);

CREATE POLICY "Allow public read alerts" ON alerts FOR SELECT USING (true);
CREATE POLICY "Allow public insert alerts" ON alerts FOR INSERT WITH CHECK (true);
CREATE POLICY "Allow public update alerts" ON alerts FOR UPDATE USING (true);

CREATE POLICY "Allow public read commands" ON commands FOR SELECT USING (true);
CREATE POLICY "Allow public insert commands" ON commands FOR INSERT WITH CHECK (true);
CREATE POLICY "Allow public update commands" ON commands FOR UPDATE USING (true);

-- Vista para el último estado de cada dispositivo
CREATE OR REPLACE VIEW device_status AS
SELECT 
  d.id,
  d.device_id,
  d.name,
  d.location,
  d.location_detail,
  d.is_online,
  d.last_seen,
  r.temp1,
  r.temp2,
  r.temp_avg,
  r.humidity,
  r.door_open,
  r.door_open_seconds,
  r.relay_on,
  r.alert_active,
  r.simulation_mode,
  r.created_at as reading_time
FROM devices d
LEFT JOIN LATERAL (
  SELECT * FROM readings 
  WHERE device_id = d.device_id 
  ORDER BY created_at DESC 
  LIMIT 1
) r ON true;

-- Habilitar Realtime para las tablas importantes
ALTER PUBLICATION supabase_realtime ADD TABLE readings;
ALTER PUBLICATION supabase_realtime ADD TABLE alerts;
ALTER PUBLICATION supabase_realtime ADD TABLE devices;
