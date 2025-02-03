# ESP8266-base64

These are functions used to encode and decode data to and from the Base64 format.

The original code is from this github repository: https://github.com/AxisCommunications/arduino-websocket-server/blob/master/Base64.cpp.

## API:

The library provides two functions: one for encoding data into Base64 and one for decode the Base64 data:

 int encoded_lenght = b64_encode( char *data_out, char *data_in, int data_in_lenght );

 int decoded_lenght = b64_decode( char *data_out, char *data_in, int data_in_lenght );


## How to use:

 Copy the two files **PasswordBase64.cpp** and **PasswordBase64.h** into your source folder.
 
 Include the **PasswordBase64.h** file in your source code:

``
  #include "PasswordBase64.h"
``

 Notice that I've changed the file names to avoid clashing to the previous name used (base64.h and base64.cpp) which could be used by some frameworks, more specifically the Arduino framework.

### Encoding:
    char b64data[256];   // Size is just an example.
    String sEncoded = "Hello Sensor Community world!";
	
    Serial.println("Message: " );
    Serial.println( sEncoded) ;    
    
    Serial.println(" Encoded message:");
	
    int b64len = b64_encode(b64data, (char *)sEncoded.c_str(),sEncoded.length());
	
    Serial.println ( String(b64data) );
    Serial.println ("The lenght is: " + String(b64len) );


### Decoding:
    char decoded[256];
    String sDecode(b64data);
    b64_decode( decoded , (char *)sDecode.c_str() , sDecode.length() );
	  Serial.println(" Decoded: " + String(decoded));

 or

    b64_decode( decodec, b64data, b64len);
    Serial.println("Decoded: " + String(decoded));
