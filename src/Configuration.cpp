#include "Configuration.h"

Configuration::Configuration() : _initialized(false) { setDefaults(); }

void Configuration::begin() {
  _prefs.begin("silvia", false); // "silvia" namespace, false = RW mode
  load();
  _initialized = true;
}

void Configuration::setDefaults() {
  // defaults from config.h where possible or sane defaults
  strlcpy(_data.mqtt_server, MQTT_BROKER, sizeof(_data.mqtt_server));
  _data.mqtt_port = MQTT_PORT;
  strlcpy(_data.mqtt_user, MQTT_USER, sizeof(_data.mqtt_user));
  strlcpy(_data.mqtt_pass, MQTT_PASS, sizeof(_data.mqtt_pass));

  _data.pid_kp = PID_KP_DEFAULT;
  _data.pid_ki = PID_KI_DEFAULT;
  _data.pid_kd = PID_KD_DEFAULT;
  _data.pid_setpoint = 95.0;  // Typical espresso temp
  _data.mqtt_enabled = false; // Disabled by default
  _data.heater_enabled = true;
}

void Configuration::load() {
  if (_prefs.isKey("init")) {
    _prefs.getString("mqtt_server", _data.mqtt_server,
                     sizeof(_data.mqtt_server));
    _data.mqtt_port = _prefs.getInt("mqtt_port", MQTT_PORT);
    _prefs.getString("mqtt_user", _data.mqtt_user, sizeof(_data.mqtt_user));
    _prefs.getString("mqtt_pass", _data.mqtt_pass, sizeof(_data.mqtt_pass));

    _data.mqtt_enabled = _prefs.getBool("mqtt_enabled", false);
    _data.heater_enabled = _prefs.getBool("heater_enabled", true); // Default ON

    _data.pid_kp = _prefs.getFloat("pid_kp", PID_KP_DEFAULT);
    _data.pid_ki = _prefs.getFloat("pid_ki", PID_KI_DEFAULT);
    _data.pid_kd = _prefs.getFloat("pid_kd", PID_KD_DEFAULT);
    _data.pid_setpoint = _prefs.getFloat("pid_setpoint", 95.0);
  } else {
    // First run, save defaults
    save();
    _prefs.putBool("init", true);
  }
}

void Configuration::save() {
  _prefs.putString("mqtt_server", _data.mqtt_server);
  _prefs.putInt("mqtt_port", _data.mqtt_port);
  _prefs.putString("mqtt_user", _data.mqtt_user);
  _prefs.putString("mqtt_pass", _data.mqtt_pass);

  _prefs.putBool("mqtt_enabled", _data.mqtt_enabled);
  _prefs.putBool("heater_enabled", _data.heater_enabled);

  _prefs.putFloat("pid_kp", _data.pid_kp);
  _prefs.putFloat("pid_ki", _data.pid_ki);
  _prefs.putFloat("pid_kd", _data.pid_kd);
  _prefs.putFloat("pid_setpoint", _data.pid_setpoint);
}
