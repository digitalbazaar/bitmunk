/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package
{
   import flash.display.Sprite;
   import flash.system.Security;
   
   public class Test extends Sprite
   {
      import com.hurlant.crypto.cert.X509Certificate;
      import com.hurlant.crypto.cert.X509CertificateCollection;
      import com.hurlant.crypto.symmetric.AESKey;
      import com.hurlant.crypto.tls.TLSConfig;
      import com.hurlant.crypto.tls.TLSEngine;
      import com.hurlant.util.der.PEM;
      import com.hurlant.util.Hex;
      
      import db.net.HttpConnection;
      import db.net.HttpConnectionEvent;
      import db.net.HttpRequest;
      import db.net.HttpResponse;
      import flash.external.ExternalInterface;
      import flash.net.URLRequestHeader;
      import flash.net.URLRequestMethod;
      import flash.net.URLVariables;
      import flash.events.IOErrorEvent;
      import flash.events.SecurityErrorEvent;
      import flash.utils.ByteArray;
      import flash.utils.getTimer;
      
      private static const CRLF:String = "\r\n";
      
      public function Test()
      {
         if(!ExternalInterface.available)
         {
            trace("ExternalInterface is not available");
            throw new Error(
               "Flash's ExternalInterface is not available. This is a " +
               "requirement of SocketPool and therefore, it will be " +
               "unavailable.");
         }
         else
         {
            try
            {
               // set up javascript access:
               ExternalInterface.addCallback("aes", aes);
               ExternalInterface.addCallback("http", http);
            }
            catch(e:Error)
            {
               log("error=" + e.errorID + "," + e.name + "," + e.message);
               throw e;
            }
            
            log("ready");
         }
      }
      
      /**
       * A debug logging function.
       * 
       * @param obj the string or error to log.
       */
      private function log(obj:Object):void
      {
         if(obj is String)
         {
            var str:String = obj as String;
            ExternalInterface.call("console.log", "flashTest", str);
         }
         else if(obj is Error)
         {
            var e:Error = obj as Error;
            log("error=" + e.errorID + "," + e.name + "," + e.message);
         }
      }
      
      private function aes():void
      {
         log('flash running aes test...');
         
         // 128-bit test keys
         var keys:Array = [
            "00010203050607080A0B0C0D0F101112",
            "14151617191A1B1C1E1F202123242526",
            "28292A2B2D2E2F30323334353738393A",
            "3C3D3E3F41424344464748494B4C4D4E",
            "50515253555657585A5B5C5D5F606162",
            "64656667696A6B6C6E6F707173747576",
            "78797A7B7D7E7F80828384858788898A",
            "8C8D8E8F91929394969798999B9C9D9E",
            "A0A1A2A3A5A6A7A8AAABACADAFB0B1B2",
            "B4B5B6B7B9BABBBCBEBFC0C1C3C4C5C6",
            "C8C9CACBCDCECFD0D2D3D4D5D7D8D9DA",
            "DCDDDEDFE1E2E3E4E6E7E8E9EBECEDEE",
            "F0F1F2F3F5F6F7F8FAFBFCFDFE010002",
            "04050607090A0B0C0E0F101113141516",
            "2C2D2E2F31323334363738393B3C3D3E",
            "40414243454647484A4B4C4D4F505152",
            "54555657595A5B5C5E5F606163646566",
            "68696A6B6D6E6F70727374757778797A",
            "7C7D7E7F81828384868788898B8C8D8E",
            "A4A5A6A7A9AAABACAEAFB0B1B3B4B5B6"];
         
         // test data to encrypt
         var data:String =
            "GET /my/url/index.html HTTP/1.1\r\n" +
            "Accept: text/html,*/*\r\n" +
            "Accept-Charset: UTF-8,*\r\n" +
            "Accept-Encoding: gzip,deflate\r\n" +
            "Accept-Language: en-us,en;q=0.5\r\n" +
            "Cache-Control: max-age=0\r\n" +
            "Connection: keep-alive\r\n" +
            "Host: localhost:19300\r\n" +
            "If-Modified-Since: Fri, 23 Apr 2010 16:40:05 GMT\r\n" +
            "Keep-Alive: 115\r\n" +
            "User-Agent: MyBrowser/1.0\r\n" +
            "\r\n";
         
         var now:uint;
         var totalEncrypt:Number = 0;
         var totalDecrypt:Number = 0;
         var count:uint = 100;
         var totalTimes:uint = keys.length * count;
         for(var n:uint = 0; n < count; n++)
         {
            for(var i:uint = 0; i < keys.length; i++)
            {
               var key:ByteArray = Hex.toArray(keys[i]);
               var aes:AESKey = new AESKey(key);
               var ba:ByteArray = Hex.toArray(data);
               
   	         // encrypt
	            now = getTimer();
	            aes.encrypt(ba);
               totalEncrypt += getTimer() - now;
	            
   	         // decrypt
               now = getTimer();
	            aes.decrypt(ba);
               totalDecrypt += getTimer() - now;
            }
         }
         
         log('encrypt time: ' + (totalEncrypt / totalTimes) + ' ms');
         log('decrypt time: ' + (totalDecrypt / totalTimes) + ' ms');
         log('flash aes test complete.');
      }
      
      private function http():void
      {
         log('flash running http test...');
         
         // permits socket access to backend
         Security.loadPolicyFile("xmlsocket://localhost:19843");
            
         try
         {
            var conn:HttpConnection = new HttpConnection();
            conn.verifyCommonName = "localhost";
            
            conn.addEventListener(HttpConnectionEvent.OPEN, connected);
            conn.addEventListener(HttpConnectionEvent.REQUEST, requestSent);
            conn.addEventListener(HttpConnectionEvent.HEADER, headerReceived);
            conn.addEventListener(HttpConnectionEvent.LOADING, bodyLoading);
            conn.addEventListener(HttpConnectionEvent.BODY, bodyReceived);
            conn.addEventListener(HttpConnectionEvent.TIMEOUT, timedOut);
            conn.addEventListener(HttpConnectionEvent.IO_ERROR, ioError);
            conn.addEventListener(
               HttpConnectionEvent.SECURITY_ERROR, securityError);
            
            // FIXME: needs to get this from javascript configuration
            // to ensure only the generated cert from the server is trusted
            var cert:ByteArray = new ByteArray();
            cert.writeUTFBytes(
            "-----BEGIN CERTIFICATE-----" + CRLF +
            "MIIDBDCCAm2gAwIBAAIBADANBgkqhkiG9w0BAQUFADCByDErMCkGA1UEAxMiYml0" + CRLF +
            "bXVuay1wZWVyIENlcnRpZmljYXRlIEF1dGhvcml0eTFFMEMGA1UECxM8Qml0bXVu" + CRLF +
            "ayBFbmNyeXB0aW9uLU9ubHkgQ2VydGlmaWNhdGVzIC0gQXV0aG9yaXphdGlvbiB2" + CRLF +
            "aWEgQlRQMR0wGwYDVQQKExREaWdpdGFsIEJhemFhciwgSW5jLjELMAkGA1UEBhMC" + CRLF +
            "VVMxEzARBgNVBAcTCkJsYWNrc2J1cmcxETAPBgNVBAgTCFZpcmdpbmlhMB4XDTA5" + CRLF +
            "MDYwMzE3NDcxNloXDTI5MDUzMDE3NDcxNlowgcgxKzApBgNVBAMTImJpdG11bmst" + CRLF +
            "cGVlciBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxRTBDBgNVBAsTPEJpdG11bmsgRW5j" + CRLF +
            "cnlwdGlvbi1Pbmx5IENlcnRpZmljYXRlcyAtIEF1dGhvcml6YXRpb24gdmlhIEJU" + CRLF +
            "UDEdMBsGA1UEChMURGlnaXRhbCBCYXphYXIsIEluYy4xCzAJBgNVBAYTAlVTMRMw" + CRLF +
            "EQYDVQQHEwpCbGFja3NidXJnMREwDwYDVQQIEwhWaXJnaW5pYTCBnTANBgkqhkiG" + CRLF +
            "9w0BAQEFAAOBiwAwgYcCgYEAwBDkqtqiOMv76Fey9dnv40Z381+otWFUEyQVJIUJ" + CRLF +
            "3fwzBMn9NobkKM84r/0Wtz99qttdfAA7/ZJ5hORbI3aCyeJbM/ua+YgCo8n4RkIn" + CRLF +
            "o9cUeRavqBhpoKK3AQK8MIODSVpjEkECmtS/xbH8912K2/+B6nZgKq6l+Q8FpNU5" + CRLF +
            "1SUCAQMwDQYJKoZIhvcNAQEFBQADgYEAmSGMss3oulT6UwA/7rYWzMf/zaN9UoYO" + CRLF +
            "V59m79RCJDQT8LmBeIXRGlSl1e3RdXK+zFzXNvoGBqcNJaoszaMgChlbHgYKpak7" + CRLF +
            "aPvOqAe+KeFz7hRPtDL/YWC4ZuMsai6GwVtKUXfozXUa74s+yvepJmRakAlDLMMj" + CRLF +
            "fw6++JtKKX0=" + CRLF +
            "-----END CERTIFICATE-----");
            cert.position = 0;
            
            // create X509CertificateCollection
            var caStore:X509CertificateCollection =
               new X509CertificateCollection();
            var x509:X509Certificate = new X509Certificate(
               PEM.readCertIntoArray(cert.readUTFBytes(cert.length)));
            caStore.addCertificate(x509);
            
            // create TLS config
            // entity, cipher suites, compressions, cert, key, CA store
            conn.tlsconfig = new TLSConfig(
               TLSEngine.CLIENT, // entity
               null,             // cipher suites
               null,             // compressions
               null,             // certificate
               null,             // private key
               caStore);         // certificate authority storage
            
            // queue request(s)
            var request:HttpRequest;
            var vars:URLVariables;
            
            // GET test
            request = new HttpRequest();
            request.method = URLRequestMethod.GET;
            request.url = "/api/3.0/system/test/echo";
            vars = new URLVariables();
            vars.echo = "data";
            request.data = vars;
            conn.addRequest(request);
            
            // POST test
            request = new HttpRequest();
            request.method = URLRequestMethod.POST;
            request.url = "/api/3.0/system/test/echo";
            request.contentType = "application/x-www-form-urlencode";
            vars = new URLVariables();
            vars.echo = "data";
            request.data = vars;
            conn.addRequest(request);
            
            // connect
            conn.connect("https://localhost:19100");
         }
         catch(e:Error)
         {
            trace(e.getStackTrace());
         }
      }
      
      private function connected(e:HttpConnectionEvent):void
      {
         trace("ready to send request");
      }
      
      private function requestSent(e:HttpConnectionEvent):void
      {
         trace("request header:\n" + e.response.request.toString());
      }
      
      private function headerReceived(e:HttpConnectionEvent):void
      {
         trace("response header:\n" + e.response.toString());
      }
      
      private function bodyLoading(e:HttpConnectionEvent):void
      {
         trace("loading body");
      }
      
      private function bodyReceived(e:HttpConnectionEvent):void
      {
         if(e.response.data == null)
         {
            trace("there is no response data");
         }
         else
         {
            trace("response data:\n'" + e.response.data.toString() + "'");
         }
      }
      
      private function timedOut(e:HttpConnectionEvent):void
      {
         trace("response receive timed out");
      }
      
      private function ioError(e:HttpConnectionEvent):void
      {
         trace("io error: " + e.cause.text);
      }
      
      private function securityError(e:HttpConnectionEvent):void
      {
         trace("security error: " + e.cause.text);
      }
   }
}
