-- ============================================
-- SCHEMA FRIOSEGURO v2 - MONITOREO EN LA NUBE
-- Supabase Project: xhdeacnwdzvkivfjzard
-- Soporta: Campamento Parametican + Carnicerías + Otros
-- ============================================

-- Eliminar tablas existentes si es necesario (CUIDADO en producción)
-- DROP TABLE IF EXISTS commands CASCADE;
-- DROP TABLE IF EXISTS alerts CASCADE;
-- DROP TABLE IF EXISTS readings CASCADE;
-- DROP TABLE IF EXISTS devices CASCADE;
-- DROP TABLE IF EXISTS organizations CASCADE;

-- ============================================
-- ORGANIZACIONES (Clientes)
-- ============================================
CREATE TABLE IF NOT EXISTS organizations (
  id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
  slug TEXT UNIQUE NOT NULL,                 -- "parametican", "carniceria-norte"
  name TEXT NOT NULL,                        -- "Campamento Parametican Silver"
  type TEXT NOT NULL DEFAULT 'reefer',       -- "reefer", "carniceria", "restaurante"
  logo_url TEXT,
  contact_email TEXT,
  contact_phone TEXT,
  timezone TEXT DEFAULT 'America/Argentina/Buenos_Aires',
  created_at TIMESTAMPTZ DEFAULT NOW(),
  config JSONB DEFAULT '{}'::jsonb
);

-- Insertar organizaciones iniciales
INSERT INTO organizations (slug, name, type) VALUES 
  ('parametican', 'Campamento Parametican Silver', 'reefer'),
  ('carniceria-demo', 'Carnicería Demo', 'carniceria')
ON CONFLICT (slug) DO NOTHING;

-- ============================================
-- DISPOSITIVOS (ESP32/Reefer)
-- ============================================
CREATE TABLE IF NOT EXISTS devices (
  id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
  device_id TEXT UNIQUE NOT NULL,            -- "REEFER-001", "CARNICERIA-LOCAL1"
  org_slug TEXT NOT NULL,                    -- Referencia a organización por slug
  name TEXT NOT NULL,                        -- "Freezer Principal"
  location TEXT,                             -- "Local Centro"
  location_detail TEXT,                      -- "Cocina - Freezer 1"
  latitude FLOAT,
  longitude FLOAT,
  created_at TIMESTAMPTZ DEFAULT NOW(),
  last_seen TIMESTAMPTZ DEFAULT NOW(),
  is_online BOOLEAN DEFAULT false,
  
  -- MODO DESCONGELAMIENTO
  alerts_disabled BOOLEAN DEFAULT false,
  alerts_disabled_until TIMESTAMPTZ,
  alerts_disabled_reason TEXT,
  
  -- UMBRALES (configurables desde web/app)
  temp_min FLOAT DEFAULT -40.0,
  temp_max FLOAT DEFAULT -18.0,
  temp_critical FLOAT DEFAULT -10.0,
  door_open_max_sec INT DEFAULT 180,
  alert_delay_sec INT DEFAULT 300,
  
  -- Estado
  firmware_version TEXT DEFAULT '2.0',
  simulation_mode BOOLEAN DEFAULT false,
  sensor1_enabled BOOLEAN DEFAULT true,
  sensor2_enabled BOOLEAN DEFAULT false,
  door_sensor_enabled BOOLEAN DEFAULT false,
  relay_enabled BOOLEAN DEFAULT true,
  telegram_enabled BOOLEAN DEFAULT true,
  
  config JSONB DEFAULT '{}'::jsonb
);

-- ============================================
-- LECTURAS (datos cada 5 seg)
-- ============================================
CREATE TABLE IF NOT EXISTS readings (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT NOT NULL,
  temp1 FLOAT,
  temp2 FLOAT,
  temp_avg FLOAT,
  humidity FLOAT,
  door_open BOOLEAN DEFAULT false,
  door_open_seconds INT DEFAULT 0,
  relay_on BOOLEAN DEFAULT false,
  alert_active BOOLEAN DEFAULT false,
  simulation_mode BOOLEAN DEFAULT false,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================
-- ALERTAS (historial)
-- ============================================
CREATE TABLE IF NOT EXISTS alerts (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT NOT NULL,
  alert_type TEXT NOT NULL,                  -- 'temperature', 'door', 'offline', 'back_online'
  severity TEXT NOT NULL,                    -- 'info', 'warning', 'critical'
  message TEXT NOT NULL,
  temp_value FLOAT,
  acknowledged BOOLEAN DEFAULT false,
  acknowledged_at TIMESTAMPTZ,
  acknowledged_by TEXT,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================
-- COMANDOS (web/app → ESP32)
-- ============================================
CREATE TABLE IF NOT EXISTS commands (
  id BIGSERIAL PRIMARY KEY,
  device_id TEXT NOT NULL,
  command TEXT NOT NULL,                     -- 'stop_alert', 'disable_alerts', 'enable_alerts', 'update_config'
  payload JSONB DEFAULT '{}'::jsonb,
  executed BOOLEAN DEFAULT false,
  executed_at TIMESTAMPTZ,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================
-- SUSCRIPTORES PUSH (para PWA)
-- ============================================
CREATE TABLE IF NOT EXISTS push_subscriptions (
  id BIGSERIAL PRIMARY KEY,
  org_slug TEXT NOT NULL,
  endpoint TEXT NOT NULL,
  p256dh TEXT,
  auth TEXT,
  created_at TIMESTAMPTZ DEFAULT NOW(),
  UNIQUE(endpoint)
);

-- ============================================
-- ÍNDICES
-- ============================================
CREATE INDEX IF NOT EXISTS idx_devices_org ON devices(org_slug);
CREATE INDEX IF NOT EXISTS idx_readings_device_time ON readings(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_readings_time ON readings(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_device_time ON alerts(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_commands_pending ON commands(device_id, executed) WHERE executed = false;

-- ============================================
-- FUNCIONES
-- ============================================

-- Actualizar last_seen cuando llega lectura
CREATE OR REPLACE FUNCTION update_device_last_seen()
RETURNS TRIGGER AS $$
BEGIN
  UPDATE devices 
  SET last_seen = NOW(), is_online = true 
  WHERE device_id = NEW.device_id;
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trigger_update_last_seen ON readings;
CREATE TRIGGER trigger_update_last_seen
  AFTER INSERT ON readings
  FOR EACH ROW
  EXECUTE FUNCTION update_device_last_seen();

-- Marcar offline después de 2 min sin datos
CREATE OR REPLACE FUNCTION mark_devices_offline()
RETURNS void AS $$
DECLARE
  dev RECORD;
BEGIN
  FOR dev IN 
    SELECT device_id, name FROM devices 
    WHERE is_online = true AND last_seen < NOW() - INTERVAL '2 minutes'
  LOOP
    UPDATE devices SET is_online = false WHERE device_id = dev.device_id;
    
    -- Crear alerta de offline
    INSERT INTO alerts (device_id, alert_type, severity, message)
    VALUES (dev.device_id, 'offline', 'warning', 'Dispositivo sin conexión: ' || dev.name);
  END LOOP;
END;
$$ LANGUAGE plpgsql;

-- Limpiar lecturas viejas (mantener 48 horas)
CREATE OR REPLACE FUNCTION cleanup_old_readings()
RETURNS void AS $$
BEGIN
  DELETE FROM readings WHERE created_at < NOW() - INTERVAL '48 hours';
END;
$$ LANGUAGE plpgsql;

-- Reactivar alertas si pasó el tiempo
CREATE OR REPLACE FUNCTION reactivate_alerts()
RETURNS void AS $$
BEGIN
  UPDATE devices 
  SET alerts_disabled = false, alerts_disabled_until = NULL, alerts_disabled_reason = NULL
  WHERE alerts_disabled = true 
    AND alerts_disabled_until IS NOT NULL 
    AND alerts_disabled_until < NOW();
END;
$$ LANGUAGE plpgsql;

-- ============================================
-- RLS (Row Level Security)
-- ============================================
ALTER TABLE organizations ENABLE ROW LEVEL SECURITY;
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;
ALTER TABLE readings ENABLE ROW LEVEL SECURITY;
ALTER TABLE alerts ENABLE ROW LEVEL SECURITY;
ALTER TABLE commands ENABLE ROW LEVEL SECURITY;
ALTER TABLE push_subscriptions ENABLE ROW LEVEL SECURITY;

-- Políticas públicas (usando anon key)
CREATE POLICY "public_read_orgs" ON organizations FOR SELECT USING (true);
CREATE POLICY "public_read_devices" ON devices FOR SELECT USING (true);
CREATE POLICY "public_insert_devices" ON devices FOR INSERT WITH CHECK (true);
CREATE POLICY "public_update_devices" ON devices FOR UPDATE USING (true);
CREATE POLICY "public_read_readings" ON readings FOR SELECT USING (true);
CREATE POLICY "public_insert_readings" ON readings FOR INSERT WITH CHECK (true);
CREATE POLICY "public_read_alerts" ON alerts FOR SELECT USING (true);
CREATE POLICY "public_insert_alerts" ON alerts FOR INSERT WITH CHECK (true);
CREATE POLICY "public_update_alerts" ON alerts FOR UPDATE USING (true);
CREATE POLICY "public_all_commands" ON commands FOR ALL USING (true);
CREATE POLICY "public_all_push" ON push_subscriptions FOR ALL USING (true);

-- ============================================
-- REALTIME
-- ============================================
ALTER PUBLICATION supabase_realtime ADD TABLE readings;
ALTER PUBLICATION supabase_realtime ADD TABLE alerts;
ALTER PUBLICATION supabase_realtime ADD TABLE devices;
ALTER PUBLICATION supabase_realtime ADD TABLE commands;

-- ============================================
-- DATOS DE EJEMPLO
-- ============================================
INSERT INTO devices (device_id, org_slug, name, location, location_detail) VALUES 
  ('REEFER-01', 'parametican', 'Reefer Principal', 'Campamento Parametican', 'Cerro Moro, Santa Cruz')
ON CONFLICT (device_id) DO NOTHING;
