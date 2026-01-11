package com.parametican.alertarift

import android.content.Intent
import android.os.Bundle
import android.view.animation.AnimationUtils
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import kotlin.concurrent.thread

/**
 * LoginActivity para la versión CLOUD
 * Se conecta directamente a Supabase, no necesita IP local
 */
class LoginActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Verificar si ya está configurado
        val prefs = getSharedPreferences("parametican_cloud", MODE_PRIVATE)
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

        // Animación de entrada
        val slideUp = AnimationUtils.loadAnimation(this, android.R.anim.slide_in_left)
        slideUp.duration = 500
        cardLogin.startAnimation(slideUp)

        // Ocultar campo de IP (no se necesita para cloud)
        etServerIp.visibility = android.view.View.GONE
        
        // Cambiar texto del botón
        btnAutoConnect.text = "☁️ Conectar a la Nube"
        btnConnect.visibility = android.view.View.GONE

        // Conectar a Supabase
        btnAutoConnect.setOnClickListener {
            tvStatus.text = "☁️ Conectando a Supabase..."
            connectToSupabase(tvStatus)
        }
        
        tvStatus.text = "Presiona el botón para conectar a la nube"
    }
    
    private fun connectToSupabase(tvStatus: TextView) {
        thread {
            try {
                val connected = SupabaseClient.checkConnection()
                
                if (connected) {
                    val devices = SupabaseClient.getDevicesStatus()
                    
                    runOnUiThread {
                        if (devices.isNotEmpty()) {
                            tvStatus.text = "✅ Conectado - ${devices.size} dispositivos encontrados"
                            
                            // Guardar estado
                            getSharedPreferences("parametican_cloud", MODE_PRIVATE).edit()
                                .putBoolean("logged_in", true)
                                .putBoolean("cloud_mode", true)
                                .apply()
                            
                            // Ir a MainActivity después de 1 segundo
                            tvStatus.postDelayed({ goToMain() }, 1000)
                        } else {
                            tvStatus.text = "⚠️ Conectado pero no hay dispositivos registrados"
                        }
                    }
                } else {
                    runOnUiThread {
                        tvStatus.text = "❌ No se pudo conectar a Supabase\n\nVerifica tu conexión a internet"
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    tvStatus.text = "❌ Error: ${e.message}"
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
