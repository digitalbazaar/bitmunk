/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 * 
 * @author Dave Longley
 */
package
{
   import flash.display.Sprite;
   
   // permits socket access to backend
   import flash.system.Security;
   Security.loadPolicyFile("xmlsocket://localhost:19843");
   
   public class Test extends Sprite
   {
      import com.hurlant.crypto.cert.X509Certificate;
      import com.hurlant.crypto.cert.X509CertificateCollection;
      import com.hurlant.crypto.tls.TLSConfig;
      import com.hurlant.crypto.tls.TLSEngine;
      import com.hurlant.util.der.PEM;
      
      import db.net.HttpConnection;
      import db.net.HttpConnectionEvent;
      import db.net.HttpRequest;
      import db.net.HttpResponse;
      import flash.net.URLRequestHeader;
      import flash.net.URLRequestMethod;
      import flash.net.URLVariables;
      import flash.events.IOErrorEvent;
      import flash.events.SecurityErrorEvent;
      import flash.utils.ByteArray;
      
      private static const CRLF:String = "\r\n";
      
      public function Test()
      {
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
