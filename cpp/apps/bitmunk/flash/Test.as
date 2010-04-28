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
            ExternalInterface.addCallback("aes_flash_128", aes_flash_128);
            ExternalInterface.addCallback("aes_test", aes_test);
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
   
   /*******************/
   
   private var init:Boolean = false;
   private var Nb:uint = 4;
   private var sbox:Array;
   private var isbox:Array;
   private var rcon:Array;
   private var mix:Array;
   private var imix:Array;
   
   private function initialize():void
   {
      init = true;
      rcon =
         [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36];
      
      // compute xtime table which maps i onto GF(i, 0x02)
      var xtime:Array = new Array(256);
      var i:uint;
      for(i = 0; i < 128; i++)
      {
         xtime[i] = i << 1;
         xtime[i + 128] = (i + 128) << 1 ^ 0x11B;
      }
      
      // compute all other tables
      sbox = new Array(256);
      isbox = new Array(256);
      mix = new Array(4);
      imix = new Array(4);
      for(i = 0; i < 4; i++)
      {
         mix[i] = new Array(256);
         imix[i] = new Array(256);
      }
      var e:uint = 0, ei:uint = 0, e2:uint,
         e4:uint, e8:uint, sx:uint, sx2:uint, me:uint, ime:uint;
      for(i = 0; i < 256; i++)
      {
         // apply affine transformation
         sx = ei ^ (ei << 1) ^ (ei << 2) ^ (ei << 3) ^ (ei << 4);
         sx = (sx >> 8) ^ (sx & 0xFF) ^ 0x63;
         
         // update tables
         sbox[e] = sx;
         isbox[sx] = e;
         
         // calculate mix and imix table values
         sx2 = xtime[sx];
         e2 = xtime[e];
         e4 = xtime[e2];
         e8 = xtime[e4];
         me =
            (sx2 << 24) ^  // 2
            (sx << 16) ^   // 1
            (sx << 8) ^    // 1
            (sx ^ sx2);    // 3
         ime =
            (e2 ^ e4 ^ e8) << 24 ^  // E (14)
            (e ^ e8) << 16 ^        // 9
            (e ^ e4 ^ e8) << 8 ^    // D (13)
            (e ^ e2 ^ e8);          // B (11)
         // produce each of the mix tables by rotating the 2,1,1,3 value
         for(var n:uint = 0; n < 4; n++)
         {
            mix[n][e] = me;
            imix[n][sx] = ime;
            // cycle the right most byte to the left most position
            // ie: 2,1,1,3 becomes 3,2,1,1
            me = me << 24 | me >>> 8;
            ime = ime << 24 | ime >>> 8;
         }
         
         // get next element and inverse
         if(e == 0)
         {
            // 1 is the inverse of 1
            e = ei = 1;
         }
         else
         {
            // e = 2e + 2*2*2*(10e)) = multiply e by 82 (chosen generator)
            // ei = ei + 2*2*ei = multiply ei by 5 (inverse generator)
            e = e2^xtime[xtime[xtime[e2^e8]]];
            ei ^= xtime[xtime[ei]];
         }
      }
   }
   
   private function expandKey(key:Array, decrypt:Boolean):Array
   {
      // copy the key's words to initialize the key schedule
      var w:Array = key.slice(0);
      
      /* RotWord() will rotate a word, moving the first byte to the last
         byte's position (shifting the other bytes left).
         
         We will be getting the value of Rcon at i / Nk. 'i' will iterate
         from Nk to (Nb * Nr+1). Nk = 4 (4 byte key), Nb = 4 (4 words in
         a block), Nr = Nk + 6 (10). Therefore 'i' will iterate from
         4 to 44 (exclusive). Each time we iterate 4 times, i / Nk will
         increase by 1. We use a counter iNk to keep track of this.
      */
      
      // go through the rounds expanding the key
      var temp:uint, iNk:uint = 1;
      var Nk:uint = w.length;
      var Nr1:uint = Nk + 6 + 1;
      var i:uint;
      for(i = Nk; i < Nb * Nr1; i++)
      {
         temp = w[i - 1];
         if(i % Nk == 0)
         {
            // temp = SubWord(RotWord(temp)) ^ Rcon[i / Nk]
            temp =
               sbox[temp >>> 16 & 0xFF] << 24 ^
               sbox[temp >>> 8 & 0xFF] << 16 ^
               sbox[temp & 0xFF] << 8 ^
               sbox[temp >>> 24] ^ (rcon[iNk] << 24);
            iNk++;
         }
         else if(Nk > 6 && (i % Nk == 4))
         {
            // temp = SubWord(temp)
            temp =
               sbox[temp >>> 24] << 24 ^
               sbox[temp >>> 16 & 0xFF] << 16 ^
               sbox[temp >>> 8 & 0xFF] << 8 ^
               sbox[temp & 0xFF];
         }
         w[i] = w[i - Nk] ^ temp;
      }
      if(decrypt)
      {
         var imx0:Array = imix[0];
         var imx1:Array = imix[1];
         var imx2:Array = imix[2];
         var imx3:Array = imix[3];
         // do not modify the first or last round key (round keys are Nb words)
         // as no column mixing is performed before they are added 
         for(i = Nb; i < w.length - Nb; i++)
         {
            // substitute each round key byte because the inverse-mix table
            // will inverse-substitute it (effectively cancel the substitution
            // because round key bytes aren't sub'd in decryption mode)
            w[i] =
               imx0[sbox[w[i] >>> 24]] ^
               imx1[sbox[w[i] >>> 16 & 0xFF]] ^
               imx2[sbox[w[i] >>> 8 & 0xFF]] ^
               imx3[sbox[w[i] & 0xFF]];
         }
      }
      
      return w;
   }
   
   private function updateBlock(
      w:Array, input:Array, idx:uint, output:Array, decrypt:Boolean):void
   {
      // get tables
      var mx0:Array, mx1:Array, mx2:Array, mx3:Array, sub:Array;
      if(decrypt)
      {
         // use inverted tables for decryption
         mx0 = imix[0];
         mx1 = imix[1];
         mx2 = imix[2];
         mx3 = imix[3];
         sub = isbox;
      }
      else
      {
         mx0 = mix[0];
         mx1 = mix[1];
         mx2 = mix[2];
         mx3 = mix[3];
         sub = sbox;
      }
      
      // Encrypt: AddRoundKey(state, w[0, Nb-1])
      // Decrypt: AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
      var Nr:uint = w.length / 4 - 1;
      var i:uint = decrypt ? Nr * Nb : 0;
      var state:Array = input;
      var s0:uint = state[idx] ^ w[i];
      var s1:uint = state[idx + 1] ^ w[i + 1];
      var s2:uint = state[idx + 2] ^ w[i + 2];
      var s3:uint = state[idx + 3] ^ w[i + 3];
      
      var order:Array;
      var t0:uint, t1:uint, t2:uint, t3:uint;
      for(var round:uint = 1; round < Nr; round++)
      {
         if(decrypt)
         {
            i -= 4;
            order = [s3, s0, s1, s2];
         }
         else
         {
            i += 4;
            order = [s1, s2, s3, s0];
         }
         t0 =
            mx0[s0 >>> 24] ^
            mx1[order[0] >>> 16 & 0xFF] ^
            mx2[s2 >>> 8 & 0xFF] ^
            mx3[order[2] & 0xFF] ^ w[i];
         t1 =
            mx0[s1 >>> 24] ^
            mx1[order[1] >>> 16 & 0xFF] ^
            mx2[s3 >>> 8 & 0xFF] ^
            mx3[order[3] & 0xFF] ^ w[i + 1];
         t2 =
            mx0[s2 >>> 24] ^
            mx1[order[2] >>> 16 & 0xFF] ^
            mx2[s0 >>> 8 & 0xFF] ^
            mx3[order[0] & 0xFF] ^ w[i + 2];
         t3 =
            mx0[s3 >>> 24] ^
            mx1[order[3] >>> 16 & 0xFF] ^
            mx2[s1 >>> 8 & 0xFF] ^
            mx3[order[1] & 0xFF] ^ w[i + 3];
         s0 = t0;
         s1 = t1;
         s2 = t2;
         s3 = t3;
      }
      
      if(decrypt)
      {
         i -= 4;
         order = [s3, s0, s1, s2];
      }
      else
      {
         i += 4;
         order = [s1, s2, s3, s0];
      }
      
      /*
       Encrypt:
       SubBytes(state)
       ShiftRows(state)
       AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
       
       Decrypt:
       InvShiftRows(state)
       InvSubBytes(state)
       AddRoundKey(state, w[0, Nb-1])
      */
      // Note: rows are shifted inline
      s0 =
         (sub[t0 >>> 24] << 24) ^
         (sub[order[0] >>> 16 & 0xFF] << 16) ^
         (sub[t2 >>> 8 & 0xFF] << 8) ^
         (sub[order[2] & 0xFF]) ^ w[i];
      s1 =
         (sub[t1 >>> 24] << 24) ^
         (sub[order[1] >>> 16 & 0xFF] << 16) ^
         (sub[t3 >>> 8 & 0xFF] << 8) ^
         (sub[order[3] & 0xFF]) ^ w[i + 1];
      s2 =
         (sub[t2 >>> 24] << 24) ^
         (sub[order[2] >>> 16 & 0xFF] << 16) ^
         (sub[t0 >>> 8 & 0xFF] << 8) ^
         (sub[order[0] & 0xFF]) ^ w[i + 2];
      s3 =
         (sub[t3 >>> 24] << 24) ^
         (sub[order[3] >>> 16 & 0xFF] << 16) ^
         (sub[t1 >>> 8 & 0xFF] << 8) ^
         (sub[order[1] & 0xFF]) ^ w[i + 3];
      output.push(s0);
      output.push(s1);
      output.push(s2);
      output.push(s3);
   }
   
   private function createCipher(
      key:Array, output:Array, decrypt:Boolean):Object
   {
      var cipher:Object = null;
      
      if(!init)
      {
         initialize();
      }
      
      // FIXME: validate key, allow for other kinds of inputs
      var w:Array;
      if(key.length == 4 || key.length == 6 || key.length == 8)
      {
         w = expandKey(key, decrypt);
         
         cipher = new Object();
         cipher.key = w;
         cipher.output = output || new Array();
         cipher.update = function(input:Array, idx:uint = 0):void
         {
            updateBlock(cipher.key, input, idx, cipher.output, decrypt);
         };
      }
      return cipher;
   }
   
   private function startEncrypting(key:Array, output:Array = null):Object
   {
      var cipher:Object = createCipher(key, output, false);
      
      // FIXME: handle IV, padding, etc.
      
      return cipher;
   }
   
   private function startDecrypting(key:Array, output:Array = null):Object
   {
      var cipher:Object = createCipher(key, output, true);
      
      // FIXME: handle IV, padding, etc.
      
      return cipher;
   }
   
   private function word_array_to_string(wa:Array):String
   {
      var str:String = '[';
      for(var i:uint = 0; i < wa.length; i++)
      {
      /*
         // handle signed hex
         if(wa[i] < 0)
         {
            str += (wa[i] + 0xFFFFFFFF + 1).toString(16);
         }
         else
         {
            str += wa[i].toString(16);
         }
         */
         str += wa[i].toString(16);
         if(i + 1 < wa.length)
         {
            str += ',';
         }
      }
      str += ']';
      return str;
   }
   
   private function aes_flash_128():void
   {
      log('testing flash AES-128 ...');
      
      try
      {
         var block:Array = new Array();
         block.push(0x00112233);
         block.push(0x44556677);
         block.push(0x8899aabb);
         block.push(0xccddeeff);
         
         var key:Array = new Array();
         key.push(0x00010203);
         key.push(0x04050607);
         key.push(0x08090a0b);
         key.push(0x0c0d0e0f);
         
         var expect:Array = new Array();
         expect.push(0x69c4e0d8);
         expect.push(0x6a7b0430);
         expect.push(0xd8cdb780);
         expect.push(0x70b4c55a);
         
         var cipher:Object = startEncrypting(key);
         cipher.update(block);
         log('ciphered: ' + word_array_to_string(cipher.output));
         log('expect: ' + word_array_to_string(expect));
         log('assert: ' +
            (word_array_to_string(cipher.output) ==
            word_array_to_string(expect)));

         var ct:Array = cipher.output;
         cipher = startDecrypting(key);
         cipher.update(ct);
         log('plain: ' + word_array_to_string(cipher.output));
         log('expect: ' + word_array_to_string(block));
         log('assert: ' +
            (word_array_to_string(cipher.output) ==
            word_array_to_string(block)));
      }
      catch(e:Error)
      {
         log("error=" + e.errorID + "," + e.name + "," + e.message);
         throw e;
      }
   }
   
   private function aes_test():void
   {
      log('testing AES (flash test) ...');
      
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
            var k:String = keys[i];
            var key:Array = new Array();
            key.push(parseInt(k.substr(0, 8), 16));
            key.push(parseInt(k.substr(8, 8), 16));
            key.push(parseInt(k.substr(16, 8), 16));
            key.push(parseInt(k.substr(24, 8), 16));
            
            var bytes:Array = new Array();
            var x:uint;
            for(x = 0; x < data.length; x++)
            {
               bytes.push(data.charCodeAt(x));
            }
            bytes.push(0x00);
            bytes.push(0x00);
            bytes.push(0x00);
            bytes.push(0x00);
            bytes.push(0x00);
            bytes.push(0x00);
            
            var block:Array = new Array();
            var word:uint;
            for(x = 0; x < bytes.length; x += 4)
            {
               word =
                  bytes[x] << 24 |
                  bytes[x + 1] << 16 |
                  bytes[x + 2] << 8 |
                  bytes[x + 3];
               block.push(word);
            }
            
            // encrypt
            var cipher:Object = startEncrypting(key);
            now = getTimer();
            cipher.update(block);
            totalEncrypt += getTimer() - now;

            // decrypt
            var ct:Array = cipher.output;
            cipher = startDecrypting(key);
            now = getTimer();
            cipher.update(ct);
            totalDecrypt += getTimer() - now;
         }
      }

      log('encrypt time: ' + (totalEncrypt / totalTimes) + ' ms');
      log('decrypt time: ' + (totalDecrypt / totalTimes) + ' ms');
      log('krypto aes test complete.');   
   }
}

} // end package

