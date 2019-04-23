/*
void callback(char* topic, byte* payload, unsigned int length_1) {

  //  Serial.print("Message length = "); Serial.println(length);

  // Make a string of the received message
  String  received_message = "";
  for (unsigned int i = 0; i < length_1; i++) {
    received_message = received_message + ((char)payload[i]);
  }

  // Serial.print("Message arrived and made into string: "); Serial.println(received_message);

  // Is this message for me? Only if it contains either my MAC address or ALL (broadcast)
  if (received_message.indexOf(My_MAC) >= 0 || received_message.indexOf(Broadcast_All) >= 0) {

    Serial.print("Valid Message: "); Serial.print(topic); Serial.print("  "); Serial.println(received_message);

    // Valid message received, remove the headers either All or Mac
    // Remove *ALL if present
    if  (received_message.indexOf(Broadcast_All) >= 0) {
      received_message.replace(Broadcast_All, "");       // Remove the command string characters
    } // End of strip and test for ALL broadcast

    // Remove the MAC number if present
    if  (received_message.indexOf(My_MAC) >= 0) {
      received_message.replace(My_MAC, "");       // Remove the command string characters
    } // End of strip and test for ALL broadcast

    // *********** Test for Reboot ***********
    if ((received_message.indexOf("REBOOT") >= 0) ) {
      Event = "REBOOT";
    } // End of reboot test

    Report_Requested == true;

  } // End of a valid message received

  yield();

} // End of callback
*/

void reconnect() {
  // Loop until we're reconnected
  // (or until the watchdog bites)
  while (!psClient.connected()) {

    if (WiFi.status() != WL_CONNECTED) {
      Connect_To_Any_Known_WiFi();
    }

    Serial.print("Attempting MQTT Broker connection...");
    // Attempt to connect

    // Make MAC address array used for Client ID
    char MAC_array[My_MAC.length()];
    My_MAC.toCharArray(MAC_array, (My_MAC.length() + 1));

    // Connect client and use MAC address array as the Client ID
    if (psClient.connect(MAC_array, mqtt_username, mqtt_password)) {

      Serial.println("connected");
      Serial.print("This is the client ID used: "); Serial.println(MAC_array);

      // Once connected, publish an announcement...
      Report_Requested = true;   // Request a report after power up

      // ... and resubscribe
      psClient.subscribe(InControl);
      delay(10);  // It needs a delay here else does not subscribe correctly!
      Serial.print("Subscribed to: "); Serial.println(InControl);

    } else {
      Serial.print("Failed, rc=");
      Serial.print(psClient.state());
      Serial.println(" Try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} // End of reconnect
