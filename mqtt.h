void setup_mqtt(){

  device.setName("GateLock");

  lock.onCommand(onLockCommand);
  lock.setName("My door lock");

  sonar.setName("Sonar");
  reed.setName("Reed");

  buzzer.onCommand(onSwitchCommand);
  buzzer.setName("Buzzer");

  mqtt.begin("192.168.29.141", "homeassistant", "password");

  delay(500);
}