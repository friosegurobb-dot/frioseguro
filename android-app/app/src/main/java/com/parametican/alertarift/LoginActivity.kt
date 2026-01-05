package com.parametican.alertarift

import android.content.Intent
import android.os.Bundle
import android.view.animation.AnimationUtils
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import kotlin.concurrent.thread
import java.net.HttpURLConnection
import java.net.URL

class LoginActivity : AppCompatActivity() {
    
    companion object {
        const val MODE_LOCAL = "local"
        const val MODE_INTERNET = "internet"
        const val SUPABASE_URL = "https://xhdeacnwdzvkivfjzard.supabase.co"
        const val SUPABASE_KEY = "sb_publishable_JhTUv1X2LHMBVILUaysJ3g_Ho11zu-Q"
    }
    
    private var selectedMode = MODE_LOCAL

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Verificar si ya está configurado
        val prefs = getSharedPreferences("parametican", MODE_PRIVATE)
        if (prefs.getBoolean("logged_in", false)) {
            goToMain()
            return
        }
        
        setContentView(R.layout.activity_login)

        val etServerIp = findViewById<EditText>(R.id.etServerIp)
        val btnConnect = findViewById<Button>(R.id.btnConnect)
        val btnAutoConnect = findViewById<Button>(R.id.btnAutoConnect)
        val cardLogin = findViewById<LinearLayout>(R.id.cardLogin)
        val tvStatus = findViewById<TextView>(R.id.tvConnectionStatus)
        
        // Botones de modo
        val btnModeLocal = findViewById<Button>(R.id.btnModeLocal)
        val btnModeInternet = findViewById<Button>(R.id.btnModeInternet)
        val layoutLocalConfig = findViewById<LinearLayout>(R.id.layoutLocalConfig)

        // Animación de entrada
        val slideUp = AnimationUtils.loadAnimation(this, android.R.anim.slide_in_left)
        slideUp.duration = 500
        cardLogin.startAnimation(slideUp)

        // Siempre mostrar rift.local por defecto
        etServerIp.setText("rift.local")
        
        // Selector de modo
        btnModeLocal?.setOnClickListener {
            selectedMode = MODE_LOCAL
            btnModeLocal.alpha = 1f
            btnModeInternet?.alpha = 0.5f
            layoutLocalConfig?.visibility = android.view.View.VISIBLE
            tvStatus.text = "📡 Modo LOCAL - Conectar al RIFT en tu red WiFi"
        }
        
        btnModeInternet?.setOnClickListener {
            selectedMode = MODE_INTERNET
            btnModeLocal?.alpha = 0.5f
            btnModeInternet.alpha = 1f
            layoutLocalConfig?.visibility = android.view.View.GONE
            tvStatus.text = "🌐 Modo INTERNET - Conectar via Supabase"
        }

        // Auto-conectar con rift.local
        btnAutoConnect.setOnClickListener {
            if (selectedMode == MODE_INTERNET) {
                tvStatus.text = "🌐 Conectando a la nube..."
                testInternetConnection(tvStatus)
            } else {
                tvStatus.text = "🔍 Buscando Reefer en la red..."
                tryAutoConnect(etServerIp, tvStatus)
            }
        }

        btnConnect.setOnClickListener {
            if (selectedMode == MODE_INTERNET) {
                tvStatus.text = "🌐 Conectando a la nube..."
                testInternetConnection(tvStatus)
            } else {
                val ip = etServerIp.text.toString().trim()
                if (ip.isEmpty()) {
                    Toast.makeText(this, "Ingresa la IP del servidor", Toast.LENGTH_SHORT).show()
                    return@setOnClickListener
                }
                tvStatus.text = "🔄 Conectando..."
                testConnection(ip, tvStatus)
            }
        }
        
        tvStatus.text = "Selecciona el modo de conexión"
    }
    
    private fun tryAutoConnect(etServerIp: EditText, tvStatus: TextView) {
        val addresses = listOf(
            "reefer.local",
            "192.168.0.11:3000",
            "192.168.1.100",
            "192.168.0.100",
            "192.168.4.1"
        )
        
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
                            etServerIp.setText(addr)
                            tvStatus.text = "✅ RIFT encontrado en $addr"
                            
                            getSharedPreferences("parametican", MODE_PRIVATE).edit()
                                .putString("server_ip", addr)
                                .putString("connection_mode", MODE_LOCAL)
                                .putBoolean("logged_in", true)
                                .apply()
                            
                            etServerIp.postDelayed({ goToMain() }, 1000)
                        }
                        return@thread
                    }
                } catch (e: Exception) {
                    // Continuar
                }
            }
            
            runOnUiThread {
                tvStatus.text = "❌ No se encontró RIFT. Ingresa la IP manualmente."
            }
        }
    }
    
    private fun testConnection(ip: String, tvStatus: TextView) {
        thread {
            try {
                val url = URL("http://$ip/api/status")
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 5000
                val code = conn.responseCode
                conn.disconnect()
                
                if (code == 200) {
                    runOnUiThread {
                        tvStatus.text = "✅ Conectado"
                        getSharedPreferences("parametican", MODE_PRIVATE).edit()
                            .putString("server_ip", ip)
                            .putString("connection_mode", MODE_LOCAL)
                            .putBoolean("logged_in", true)
                            .apply()
                        goToMain()
                    }
                } else {
                    runOnUiThread { tvStatus.text = "❌ Error: código $code" }
                }
            } catch (e: Exception) {
                runOnUiThread { tvStatus.text = "❌ Error: ${e.message}" }
            }
        }
    }
    
    private fun testInternetConnection(tvStatus: TextView) {
        thread {
            try {
                val url = URL("$SUPABASE_URL/rest/v1/devices?limit=1")
                val conn = url.openConnection() as HttpURLConnection
                conn.connectTimeout = 10000
                conn.setRequestProperty("apikey", SUPABASE_KEY)
                conn.setRequestProperty("Authorization", "Bearer $SUPABASE_KEY")
                val code = conn.responseCode
                conn.disconnect()
                
                if (code == 200) {
                    runOnUiThread {
                        tvStatus.text = "✅ Conectado a la nube"
                        getSharedPreferences("parametican", MODE_PRIVATE).edit()
                            .putString("supabase_url", SUPABASE_URL)
                            .putString("supabase_key", SUPABASE_KEY)
                            .putString("connection_mode", MODE_INTERNET)
                            .putBoolean("logged_in", true)
                            .apply()
                        
                        Toast.makeText(this, "🌐 Modo INTERNET activado", Toast.LENGTH_SHORT).show()
                        goToMain()
                    }
                } else {
                    runOnUiThread { tvStatus.text = "❌ Error de conexión: $code" }
                }
            } catch (e: Exception) {
                runOnUiThread { tvStatus.text = "❌ Sin internet: ${e.message}" }
            }
        }
    }

    private fun goToMain() {
        startActivity(Intent(this, MainActivity::class.java))
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out)
        finish()
    }
}
