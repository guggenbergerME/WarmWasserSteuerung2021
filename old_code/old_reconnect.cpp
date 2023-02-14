
// handle message arrived
  String content="";
  char character;
    for (int num=0;num<length;num++) {
        character = payload[num];
        content.concat(character);
      }

      Serial.print("***** MQTT callback - Content : ");
      Serial.print(topic);Serial.print(" | ");Serial.println(content);

    // *********************************************************** Relais 0
    if (strcmp(topic,"Warmwasser/ww_relais_0")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 0 -> AN");
            pcf8574.digitalWrite(P0, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 0 -> AUS");
            pcf8574.digitalWrite(P0, !LOW);
        }

    // *********************************************************** Relais 1
    if (strcmp(topic,"Warmwasser/ww_relais_1")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 1 -> AN");
            pcf8574.digitalWrite(P1, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 1 -> AUS");
            pcf8574.digitalWrite(P1, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 2
    if (strcmp(topic,"Warmwasser/ww_relais_2")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 2 -> AN");
            pcf8574.digitalWrite(P2, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 2 -> AUS");
            pcf8574.digitalWrite(P2, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 3
    if (strcmp(topic,"Warmwasser/ww_relais_3")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 3 -> AN");
            pcf8574.digitalWrite(P3, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 3 -> AUS");
            pcf8574.digitalWrite(P3, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 4
    if (strcmp(topic,"Warmwasser/ww_relais_4")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 4 -> AN");
            pcf8574.digitalWrite(P4, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 4 -> AUS");
            pcf8574.digitalWrite(P4, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 5
    if (strcmp(topic,"Warmwasser/ww_relais_5")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 5 -> AN");
            pcf8574.digitalWrite(P5, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 5 -> AUS");
            pcf8574.digitalWrite(P5, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 6
    if (strcmp(topic,"Warmwasser/ww_relais_6")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 6 -> AN");
            pcf8574.digitalWrite(P6, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 6 -> AUS");
            pcf8574.digitalWrite(P6, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 7
    if (strcmp(topic,"Warmwasser/ww_relais_7")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 7 -> AN");
            pcf8574.digitalWrite(P7, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 7 -> AUS");
            pcf8574.digitalWrite(P7, !LOW);
        }        

    } // IF strcmp



}