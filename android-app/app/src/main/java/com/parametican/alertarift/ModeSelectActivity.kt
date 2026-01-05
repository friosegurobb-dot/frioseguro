package com.parametican.alertarift

import android.content.Context
import android.content.Intent
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import kotlin.concurrent.thread
import java.net.HttpURLConnection
import java.net.URL

class ModeSelectActivity : AppCompatActivity() {
    
    companion object {
        const val MODE_LOCAL = "local"
        const val MODE_INTERNET = "internet"
        const val SUPABASE_URL = "https://xhdeacnwdzvkivfjzard.supabase.co"
        const val SUPABASE_KEY = "sb_publishable_JhTUv1X2LHMBVILUaysJ3g_Ho11zu-Q"
        const val SERVICE_TYPE = "_http._tcp."  // Tipo de servicio mDNS
        
        // Organizaciones disponibles
        val ORGANIZATIONS = mapOf(
            "parametican" to "Campamento Parametican",
            "carniceria-demo" to "Carnicería Demo"
        )
    }
    
    private var selectedMode = MODE_LOCAL
    private var selectedOrg = "parametican"
    private var nsdManager: NsdManager? = null
    private var discoveryListener: NsdManager.DiscoveryListener? = null
    private var isDiscovering = false
    private val handler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val prefs = getSharedPreferences("frioseguro", MODE_PRIVATE)
        
        // Si ya eligió modo y está logueado, ir directo
        if (prefs.getBoolean("mode_selected", false) && prefs.getBoolean("logged_in", false)) {
            goToMain()
            return
        }
        
        setContentView(R.layout.activity_mode_select)
        
        // Cargar organización guardada o mostrar selector
        selectedOrg = prefs.getString("org_slug", "parametican") ?: "parametican"
        
        val btnLocal = findViewById<LinearLayout>(R.id.btnModeLocal)
        val btnInternet = findViewById<LinearLayout>(R.id.btnModeInternet)
        val tvStatus = findViewById<TextView>(R.id.tvStatus)
        
        btnLocal.setOnClickListener {
            tvStatus.text = "📡 Buscando Reefer en la red local..."
            connectLocal(tvStatus)
        }
        
        btnInternet.setOnClickListener {
            tvStatus.text = "🌐 Conectando a la nube..."
            connectInternet(tvStatus)
        }
        
        // IP Manual
        val manualIpContainer = findViewById<LinearLayout>(R.id.manualIpContainer)
        val etManualIp = findViewById<EditText>(R.id.etManualIp)
        val btnConnectManual = findViewById<Button>(R.id.btnConnectManual)
        val tvShowManualIp = findViewById<TextView>(R.id.tvShowManualIp)
        
        tvShowManualIp.setOnClickListener {
            manualIpContainer.visibility = if (manualIpContainer.visibility == View.GONE) View.VISIBLE else View.GONE
        }
        
        btnConnectManual.setOnClickListener {
            val ip = etManualIp.text.toString().trim()
            if (ip.isNotEmpty()) {
                tvStatus.text = "📡 Conectando a $ip..."
                connectToIp(ip, tvStatus)
            }
        }
    }
    
    private fun connectToIp(ip: String, tvStatus: TextView) {
        thread {
            try {
                val url = URL("http://$ip/api/status")
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 5000
                conn.readTimeout = 5000
                val code = conn.responseCode
                conn.disconnect()
                
                if (code == 200) {
                    runOnUiThread {
                        tvStatus.text = "✅ Reefer encontrado en $ip"
                        
                        getSharedPreferences("frioseguro", MODE_PRIVATE).edit()
                            .putString("connection_mode", MODE_LOCAL)
                            .putString("server_ip", ip)
                            .putString("org_slug", selectedOrg)
                            .putBoolean("mode_selected", true)
                            .putBoolean("logged_in", true)
                            .apply()
                        
                        tvStatus.postDelayed({ goToMain() }, 800)
                    }
                } else {
                    runOnUiThread { tvStatus.text = "❌ No responde en $ip (código $code)" }
                }
            } catch (e: Exception) {
                runOnUiThread { tvStatus.text = "❌ Error conectando a $ip: ${e.message}" }
            }
        }
    }
    
    private fun connectLocal(tvStatus: TextView) {
        // Usar NSD (Network Service Discovery) para encontrar el ESP32 automáticamente
        nsdManager = getSystemService(Context.NSD_SERVICE) as NsdManager
        
        var foundDevice = false
        
        discoveryListener = object : NsdManager.DiscoveryListener {
            override fun onDiscoveryStarted(serviceType: String) {
                runOnUiThread { tvStatus.text = "📡 Buscando Reefer en la red..." }
            }
            
            override fun onServiceFound(serviceInfo: NsdServiceInfo) {
                // Buscar servicios que contengan "reefer" en el nombre
                if (serviceInfo.serviceName.lowercase().contains("reefer") && !foundDevice) {
                    foundDevice = true
                    nsdManager?.resolveService(serviceInfo, object : NsdManager.ResolveListener {
                        override fun onResolveFailed(si: NsdServiceInfo, errorCode: Int) {
                            runOnUiThread { tvStatus.text = "⚠️ Error resolviendo servicio" }
                        }
                        
                        override fun onServiceResolved(si: NsdServiceInfo) {
                            val host = si.host.hostAddress ?: return
                            stopDiscovery()
                            connectToFoundDevice(host, tvStatus)
                        }
                    })
                }
            }
            
            override fun onServiceLost(serviceInfo: NsdServiceInfo) {}
            override fun onDiscoveryStopped(serviceType: String) {}
            override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {
                runOnUiThread { tvStatus.text = "⚠️ Error iniciando búsqueda" }
                fallbackSearch(tvStatus)
            }
            override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {}
        }
        
        try {
            nsdManager?.discoverServices(SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, discoveryListener)
            isDiscovering = true
            
            // Timeout de 8 segundos, luego fallback a búsqueda por IP
            handler.postDelayed({
                if (!foundDevice) {
                    stopDiscovery()
                    runOnUiThread { tvStatus.text = "📡 Buscando por IP..." }
                    fallbackSearch(tvStatus)
                }
            }, 8000)
        } catch (e: Exception) {
            fallbackSearch(tvStatus)
        }
    }
    
    private fun stopDiscovery() {
        if (isDiscovering) {
            try {
                nsdManager?.stopServiceDiscovery(discoveryListener)
            } catch (e: Exception) {}
            isDiscovering = false
        }
    }
    
    private fun connectToFoundDevice(ip: String, tvStatus: TextView) {
        thread {
            try {
                val url = URL("http://$ip/api/status")
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 5000
                conn.readTimeout = 5000
                val code = conn.responseCode
                conn.disconnect()
                
                if (code == 200) {
                    runOnUiThread {
                        tvStatus.text = "✅ Reefer encontrado en $ip"
                        saveAndGo(ip)
                    }
                } else {
                    runOnUiThread { tvStatus.text = "⚠️ Dispositivo no responde" }
                }
            } catch (e: Exception) {
                runOnUiThread { tvStatus.text = "❌ Error: ${e.message}" }
            }
        }
    }
    
    private fun saveAndGo(ip: String) {
        getSharedPreferences("frioseguro", MODE_PRIVATE).edit()
            .putString("connection_mode", MODE_LOCAL)
            .putString("server_ip", ip)
            .putString("org_slug", selectedOrg)
            .putBoolean("mode_selected", true)
            .putBoolean("logged_in", true)
            .apply()
        
        handler.postDelayed({ goToMain() }, 800)
    }
    
    // Búsqueda fallback por IPs comunes si NSD no encuentra nada
    private fun fallbackSearch(tvStatus: TextView) {
        thread {
            // Primero intentar reefer.local directamente
            val addresses = listOf(
                "reefer.local",
                "192.168.4.1",  // AP del ESP32
                "192.168.1.1", "192.168.1.100", "192.168.1.50",
                "192.168.0.1", "192.168.0.100", "192.168.0.50",
                "10.0.0.1", "10.0.0.100", "10.0.1.1"
            )
            
            for (addr in addresses) {
                runOnUiThread { tvStatus.text = "📡 Probando $addr..." }
                try {
                    val url = URL("http://$addr/api/status")
                    val conn = url.openConnection() as HttpURLConnection
                    conn.connectTimeout = 1500
                    conn.readTimeout = 1500
                    val code = conn.responseCode
                    conn.disconnect()
                    
                    if (code == 200) {
                        runOnUiThread {
                            tvStatus.text = "✅ Reefer encontrado en $addr"
                            saveAndGo(addr)
                        }
                        return@thread
                    }
                } catch (e: Exception) { }
            }
            
            runOnUiThread {
                tvStatus.text = "❌ No se encontró Reefer.\nUsá IP manual o conectate al WiFi 'Reefer-Setup'"
                findViewById<LinearLayout>(R.id.manualIpContainer).visibility = View.VISIBLE
            }
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        stopDiscovery()
    }
    
    private fun connectInternet(tvStatus: TextView) {
        thread {
            try {
                val url = URL("$SUPABASE_URL/rest/v1/devices?org_slug=eq.$selectedOrg&limit=1")
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 10000
                conn.setRequestProperty("apikey", SUPABASE_KEY)
                conn.setRequestProperty("Authorization", "Bearer $SUPABASE_KEY")
                val code = conn.responseCode
                conn.disconnect()
                
                if (code == 200) {
                    runOnUiThread {
                        tvStatus.text = "✅ Conectado a FrioSeguro Cloud"
                        
                        getSharedPreferences("frioseguro", MODE_PRIVATE).edit()
                            .putString("connection_mode", MODE_INTERNET)
                            .putString("supabase_url", SUPABASE_URL)
                            .putString("supabase_key", SUPABASE_KEY)
                            .putString("org_slug", selectedOrg)
                            .putBoolean("mode_selected", true)
                            .putBoolean("logged_in", true)
                            .apply()
                        
                        tvStatus.postDelayed({ goToMain() }, 800)
                    }
                } else {
                    runOnUiThread { 
                        tvStatus.text = "❌ Error de conexión ($code)\nIntenta modo LOCAL" 
                    }
                }
            } catch (e: Exception) {
                runOnUiThread { 
                    tvStatus.text = "❌ Sin internet\nUsa modo LOCAL si estás en el campamento" 
                }
            }
        }
    }
    
    private fun goToMain() {
        startActivity(Intent(this, MainActivity::class.java))
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out)
        finish()
    }
}
