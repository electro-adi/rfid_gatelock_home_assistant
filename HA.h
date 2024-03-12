void onLockCommand(HALock::LockCommand cmd, HALock* sender) {

  if (cmd == HALock::CommandLock) 
  {
    sender->setState(HALock::StateLocked);
    myservo.attach(SERVO_PIN, 100, 3500);
    myservo.write(180);
    myDFPlayer.playAdvertisement(3);
    gate_locked = true;
  } 
  else if (cmd == HALock::CommandUnlock) 
  {
    sender->setState(HALock::StateUnlocked);
    myservo.attach(SERVO_PIN, 100, 3500);
    myservo.write(0);
    myDFPlayer.playAdvertisement(3);
    gate_locked = false;
  } 
  else if (cmd == HALock::CommandOpen) 
  {
    if (sender->getCurrentState() != HALock::StateUnlocked) {
      return;  // optionally you can verify if the lock is unlocked before opening
    }

    Serial.println("Command: Open");
  }
}

void onSwitchCommand(bool state, HASwitch* sender)
{
    digitalWrite(BUZZER_PIN, (state ? HIGH : LOW));
    sender->setState(state);
}