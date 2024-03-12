void readNFC(){

  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0};
  uint8_t uidLength;

  if(gate_locked)
  {
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if(success)
    {
      if (memcmp(uid, card1, sizeof(uid)) == 0)
      {
        unlock_now = true;
        Serial.println("same");
      }
      else if (memcmp(uid, card2, sizeof(uid)) == 0)
      {
        unlock_now = true;
        Serial.println("same2");
      }
      else if (memcmp(uid, card3, sizeof(uid)) == 0)
      {
        unlock_now = true;
        Serial.println("same3");
      }
      else if (memcmp(uid, tag1, sizeof(uid)) == 0)
      {
        unlock_now = true;
        Serial.println("same4");
      }
      else if (memcmp(uid, tag2, sizeof(uid)) == 0)
      {
        unlock_now = true;
        Serial.println("same5");
      }
    }
  }
}
