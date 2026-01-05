package com.parametican.alertarift

import android.content.Intent
import android.os.Bundle
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
        
        // Organizaciones disponibles
        val ORGANIZATIONS = mapOf(
            "parametican" to "Campamento Parametican",
            "carniceria-demo" to "Carnicería Demo"
        )
    }
    
    private var selectedMode = MODE_LOCAL
    private var selectedOrg = "parametican"

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
        // Buscar en todas las IPs comunes de redes locales
        val addresses = mutableListOf<String>()
        
        // Primero mDNS
        addresses.add("reefer.local")
        
        // Luego IPs comunes en diferentes subredes
        for (subnet in listOf("192.168.0", "192.168.1", "192.168.2", "192.168.4", "10.0.0", "10.0.1")) {
            for (host in listOf(1, 100, 101, 102, 50, 200)) {
                addresses.add("$subnet.$host")
            }
        }
        
        // AP del ESP32
        addresses.add("192.168.4.1")
        
        thread {
            for (addr in addresses) {
                try {
                    val url = URL("http://$addr/api/status")
                    val conn = url.openConnection() as HttpURLConnection
                    conn.connectTimeout = 2000
                    conn.readTimeout = 2000
                    val code = conn.responseCode
                    conn.disconnect()
                    
                    if (code == 200) {
                        runOnUiThread {
                            tvStatus.text = "✅ Reefer encontrado en $addr"
                            
                            getSharedPreferences("frioseguro", MODE_PRIVATE).edit()
                                .putString("connection_mode", MODE_LOCAL)
                                .putString("server_ip", addr)
                                .putString("org_slug", selectedOrg)
                                .putBoolean("mode_selected", true)
                                .putBoolean("logged_in", true)
                                .apply()
                            
                            tvStatus.postDelayed({ goToMain() }, 800)
                        }
                        return@thread
                    }
                } catch (e: Exception) { }
            }
            
            runOnUiThread {
                tvStatus.text = "❌ No se encontró Reefer en la red.\n¿Estás conectado al mismo WiFi?"
            }
        }
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
