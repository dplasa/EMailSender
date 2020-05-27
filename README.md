# Library to send EMail via ESP8266/ESP32 

## Tutorial: 

To download. click the DOWNLOADS button in the top right corner, rename the uncompressed folder SMTPSender. Check that the SMTPSender folder contains `SMTPSender.cpp` and `SMTPSender.h`. Place the SMTPSender library folder in your `<arduinosketchfolder>/libraries/` folder. You may need to create the libraries subfolder if its your first library. Restart the IDE.

## SMTP / SMTPS
This library provides an SMTPSender, which can only connect to unencrypted SMTP Servers. If you need to connect to a SMTPS server using SSL/TLS, you need to use the SMTPSSender. This will increase your compiled sketch by ~100k since all the ciphers need to be included.

### Constructor
Just place somwhere in the beginning of your sketch one of the two possibilities: 
```cpp
	SMTPSender plainSender;     // just able to talk to smtp servers
    SMTPSSender fancySSLSender; // can only talk to smtps servers
```

### Provide your smtp server's credentials
To provide your smtp(s) server's credentials, you need to call `begin()` with ServerInfo:
```cpp
    SMTPSender::ServerInfo mailServer("myLogin", "myPassword", "smtp.eample.com", 25);
	plainSender.begin(mailServer);
```

### Sending a Mail
To send a Mail, there are two ways:
* send the mail blocking, i.e. the function will only return on error or if the mail was sent:
    ```cpp
        SMTPSender::Message message("from@example.com", "to@example.com", "Subject", "Message");
        SMTPSender::Status s = plainSender.send(message);
        if (s.code = 0)
            // send ok
        else
            // send failed, see s.code and s.desc (string) for the reason
    ```
* queue the mail without blocking. Make sure to call `handleSMTP()` frequently, e.g. in `loop()`
    ```cpp
        SMTPSender::Message message("from@example.com", "to@example.com", "Subject", "Message");
        if (plainSender.queue(message))
            // message queued
        else
            // queue failed, a previous mail has not yet been fully processed
    ```
    To check, if the mail was sent, get the Status with `check()`:  
    ```cpp
        SMTPSender::Status s = plainSender.send(message);
        if (s.code = 0)
            // send ok
        else
            // send failed, see s.code and s.desc (string) for the reason
    ```

### Spoiler
Of course you need to be connected to WIFI. To enable debugging on the Serial monitor, enable debug messages in your IDE (see [IDE Debugging](https://arduino-esp8266.readthedocs.io/en/latest/Troubleshooting/debugging.html))