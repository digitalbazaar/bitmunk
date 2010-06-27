/**
 * A Javascript implementation of Transport Layer Security (TLS).
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * The TLS Handshake Protocol involves the following steps:
 *
 * - Exchange hello messages to agree on algorithms, exchange
 * random values, and check for session resumption.
 * 
 * - Exchange the necessary cryptographic parameters to allow the
 * client and server to agree on a premaster secret.
 * 
 * - Exchange certificates and cryptographic information to allow
 * the client and server to authenticate themselves.
 * 
 * - Generate a master secret from the premaster secret and
 * exchanged random values.
 * 
 * - Provide security parameters to the record layer.
 * 
 * - Allow the client and server to verify that their peer has
 * calculated the same security parameters and that the handshake
 * occurred without tampering by an attacker.
 * 
 * Up to 4 different messages may be sent during a key exchange.
 * The server certificate, the server key exchange, the client
 * certificate, and the client key exchange.
 * 
 * A typical handshake (from the client's perspective).
 * 
 * 1. Client sends ClientHello.
 * 2. Client receives ServerHello.
 * 3. Client receives optional Certificate.
 * 4. Client receives optional ServerKeyExchange.
 * 5. Client receives ServerHelloDone.
 * 6. Client sends optional Certificate.
 * 7. Client sends ClientKeyExchange.
 * 8. Client sends optional CertificateVerify.
 * 9. Client sends ChangeCipherSpec.
 * 10. Client sends Finished.
 * 11. Client receives ChangeCipherSpec.
 * 12. Client receives Finished.
 * 13. Client sends/receives application data.
 *
 * To reuse an existing session:
 * 
 * 1. Client sends ClientHello with session ID for reuse.
 * 2. Client receives ServerHello with same session ID if reusing.
 * 3. Client receives ChangeCipherSpec message if reusing.
 * 4. Client receives Finished.
 * 5. Client sends ChangeCipherSpec.
 * 6. Client sends Finished.
 * 
 * Note: Client ignores HelloRequest if in the middle of a handshake.
 * 
 * Record Layer:
 * 
 * The record layer fragments information blocks into TLSPlaintext
 * records carrying data in chunks of 2^14 bytes or less. Client message
 * boundaries are not preserved in the record layer (i.e., multiple
 * client messages of the same ContentType MAY be coalesced into a single
 * TLSPlaintext record, or a single message MAY be fragmented across
 * several records).
 * 
 * struct {
 *    uint8 major;
 *    uint8 minor;
 * } ProtocolVersion;
 * 
 * struct {
 *    ContentType type;
 *    ProtocolVersion version;
 *    uint16 length;
 *    opaque fragment[TLSPlaintext.length];
 * } TLSPlaintext;
 * 
 * type:
 *    The higher-level protocol used to process the enclosed fragment.
 * 
 * version:
 *    The version of the protocol being employed. TLS Version 1.2
 *    uses version {3, 3}. TLS Version 1.0 uses version {3, 1}. Note
 *    that a client that supports multiple versions of TLS may not know
 *    what version will be employed before it receives the ServerHello.
 * 
 * length:
 *    The length (in bytes) of the following TLSPlaintext.fragment. The
 *    length MUST NOT exceed 2^14 = 16384 bytes.
 * 
 * fragment:
 *    The application data. This data is transparent and treated as an
 *    independent block to be dealt with by the higher-level protocol
 *    specified by the type field.
 * 
 * Implementations MUST NOT send zero-length fragments of Handshake,
 * Alert, or ChangeCipherSpec content types. Zero-length fragments of
 * Application data MAY be sent as they are potentially useful as a
 * traffic analysis countermeasure.
 * 
 * Note: Data of different TLS record layer content types MAY be
 * interleaved. Application data is generally of lower precedence for
 * transmission than other content types. However, records MUST be
 * delivered to the network in the same order as they are protected by
 * the record layer. Recipients MUST receive and process interleaved
 * application layer traffic during handshakes subsequent to the first
 * one on a connection.
 * 
 * struct {
 *    ContentType type;       // same as TLSPlaintext.type
 *    ProtocolVersion version;// same as TLSPlaintext.version
 *    uint16 length;
 *    opaque fragment[TLSCompressed.length];
 * } TLSCompressed;
 * 
 * length:
 *    The length (in bytes) of the following TLSCompressed.fragment.
 *    The length MUST NOT exceed 2^14 + 1024.
 * 
 * fragment:
 *    The compressed form of TLSPlaintext.fragment.
 * 
 * Note: A CompressionMethod.null operation is an identity operation;
 * no fields are altered. In this implementation, since no compression
 * is supported, uncompressed records are always the same as compressed
 * records.
 * 
 * Encryption Information:
 * 
 * The encryption and MAC functions translate a TLSCompressed structure
 * into a TLSCiphertext. The decryption functions reverse the process. 
 * The MAC of the record also includes a sequence number so that missing,
 * extra, or repeated messages are detectable.
 * 
 * struct {
 *    ContentType type;
 *    ProtocolVersion version;
 *    uint16 length;
 *    select (SecurityParameters.cipher_type) {
 *       case stream: GenericStreamCipher;
 *       case block:  GenericBlockCipher;
 *       case aead:   GenericAEADCipher;
 *    } fragment;
 * } TLSCiphertext;
 * 
 * type:
 *    The type field is identical to TLSCompressed.type.
 * 
 * version:
 *    The version field is identical to TLSCompressed.version.
 * 
 * length:
 *    The length (in bytes) of the following TLSCiphertext.fragment.
 *    The length MUST NOT exceed 2^14 + 2048.
 * 
 * fragment:
 *    The encrypted form of TLSCompressed.fragment, with the MAC.
 * 
 * Note: Only CBC Block Ciphers are supported by this implementation.
 * 
 * The TLSCompressed.fragment structures are converted to/from block
 * TLSCiphertext.fragment structures.
 * 
 * struct {
 *    opaque IV[SecurityParameters.record_iv_length];
 *    block-ciphered struct {
 *        opaque content[TLSCompressed.length];
 *        opaque MAC[SecurityParameters.mac_length];
 *        uint8 padding[GenericBlockCipher.padding_length];
 *        uint8 padding_length;
 *    };
 * } GenericBlockCipher;
 * 
 * The MAC is generated as described in Section 6.2.3.1.
 * 
 * IV:
 *    The Initialization Vector (IV) SHOULD be chosen at random, and
 *    MUST be unpredictable. Note that in versions of TLS prior to 1.1,
 *    there was no IV field, and the last ciphertext block of the
 *    previous record (the "CBC residue") was used as the IV. This was
 *    changed to prevent the attacks described in [CBCATT]. For block
 *    ciphers, the IV length is of length
 *    SecurityParameters.record_iv_length, which is equal to the
 *    SecurityParameters.block_size.
 * 
 * padding:
 *    Padding that is added to force the length of the plaintext to be
 *    an integral multiple of the block cipher's block length. The
 *    padding MAY be any length up to 255 bytes, as long as it results
 *    in the TLSCiphertext.length being an integral multiple of the
 *    block length. Lengths longer than necessary might be desirable to
 *    frustrate attacks on a protocol that are based on analysis of the
 *    lengths of exchanged messages. Each uint8 in the padding data
 *    vector MUST be filled with the padding length value. The receiver
 *    MUST check this padding and MUST use the bad_record_mac alert to
 *    indicate padding errors.
 *
 * padding_length:
 *    The padding length MUST be such that the total size of the
 *    GenericBlockCipher structure is a multiple of the cipher's block
 *    length. Legal values range from zero to 255, inclusive. This
 *    length specifies the length of the padding field exclusive of the
 *    padding_length field itself.
 *
 * The encrypted data length (TLSCiphertext.length) is one more than the
 * sum of SecurityParameters.block_length, TLSCompressed.length,
 * SecurityParameters.mac_length, and padding_length.
 *
 * Example: If the block length is 8 bytes, the content length
 * (TLSCompressed.length) is 61 bytes, and the MAC length is 20 bytes,
 * then the length before padding is 82 bytes (this does not include the
 * IV. Thus, the padding length modulo 8 must be equal to 6 in order to
 * make the total length an even multiple of 8 bytes (the block length).
 * The padding length can be 6, 14, 22, and so on, through 254. If the
 * padding length were the minimum necessary, 6, the padding would be 6
 * bytes, each containing the value 6. Thus, the last 8 octets of the
 * GenericBlockCipher before block encryption would be xx 06 06 06 06 06
 * 06 06, where xx is the last octet of the MAC.
 *
 * Note: With block ciphers in CBC mode (Cipher Block Chaining), it is
 * critical that the entire plaintext of the record be known before any
 * ciphertext is transmitted. Otherwise, it is possible for the
 * attacker to mount the attack described in [CBCATT].
 *
 * Implementation note: Canvel et al. [CBCTIME] have demonstrated a
 * timing attack on CBC padding based on the time required to compute
 * the MAC. In order to defend against this attack, implementations
 * MUST ensure that record processing time is essentially the same
 * whether or not the padding is correct. In general, the best way to
 * do this is to compute the MAC even if the padding is incorrect, and
 * only then reject the packet. For instance, if the pad appears to be
 * incorrect, the implementation might assume a zero-length pad and then
 * compute the MAC. This leaves a small timing channel, since MAC
 * performance depends, to some extent, on the size of the data fragment,
 * but it is not believed to be large enough to be exploitable, due to
 * the large block size of existing MACs and the small size of the
 * timing signal.
 */
(function()
{
   // local alias for krypto stuff
   var krypto = window.krypto;
   
   /**
    * Generates pseudo random bytes by mixing the result of two hash
    * functions, MD5 and SHA-1.
    * 
    * prf_TLS1(secret, label, seed) =
    *    P_MD5(S1, label + seed) XOR P_SHA-1(S2, label + seed);
    * 
    * @param ms the master secret.
    * @param label the label to use.
    * @param seed the seed to use.
    * 
    * @return the pseudo random bytes.
    */
   var prf_TLS1 = function(ms, label, seed)
   {
      // FIXME: implement me
      
      // FIXME: do 5 iterations of md5
      // FIXME: do 4 iterations of sha1
   };
   
   /**
    * Generates pseudo random bytes using a SHA256 algorithm. For TLS 1.2.
    * 
    * @param ms the master secret.
    * @param ke the key expansion.
    * @param r the server random concatenated with the client random.
    * 
    * @return the pseudo random bytes.
    */
   var prf_sha256 = function(ms, ke, r)
   {
      // FIXME: implement me
   };
   
   /**
    * Gets a MAC for a record using the SHA-1 hash algorithm.
    * 
    * @param key the mac key.
    * @param state the sequence number.
    * @param record the record.
    * 
    * @return the sha-1 hash (20 bytes) for the given record.
    */
   var hmac_sha1 = function(key, seqNum, record)
   {
      /* MAC is computed like so:
      HMAC_hash(
         key, seqNum +
            TLSCompressed.type +
            TLSCompressed.version +
            TLSCompressed.length +
            TLSCompressed.fragment)
      */
      
      var b = krypto.util.createBuffer();
      b.putInt32(0);
      b.putInt32(seqNum);
      b.putByte(record.type);
      b.putByte(record.version.major);
      b.putByte(record.version.minor);
      b.putByte(record.length);
      b.putBytes(record.fragment.bytes());
      
      // FIXME: calculate MAC
   };
   
   /**
    * Encrypts the TLSCompressed record into a TLSCipherText record using
    * AES-128 in CBC mode.
    * 
    * @param record the TLSCompressed record to encrypt.
    * @param s the ConnectionState to use.
    * 
    * @return the TLSCipherText record on success, null on failure.
    */
   var encrypt_aes_128_cbc_sha1 = function(record, s)
   {
      var rval = null;
      
      // append MAC to fragment
      var mac = s.macFunction(s.macKey, s.sequenceNumber++, record);
      record.fragment.putBytes(mac);
      
      // FIXME: TLS 1.1 & 1.2 use an explicit IV every time to protect
      // against CBC attacks
      // var iv = krypto.random.getBytes(16);
      
      // use the pre-generated IV when initializing for TLS 1.0, otherwise
      // use the residue from the previous encryption
      var iv = s.cipherState.init ? null : s.cipherState.iv;
      s.cipherState.init = true;
      
      // start cipher
      var cipher = s.cipherState.cipher;
      cipher.start(iv);
      
      // write IV into output
      cipher.output.putBytes(iv);
      
      // do encryption (default padding is appropriate)
      if(cipher.update(record.fragment) && cipher.finish())
      {
         /* The encrypted data length (TLSCiphertext.length) is one more than
         the sum of SecurityParameters.block_length, TLSCompressed.length,
         SecurityParameters.mac_length, and padding_length. */
         
         // add padding length to output (there are always padding bytes and
         // their value is the length of the padding, so the last byte's value
         // will be the padding length)
         cipher.output.putByte(cipher.output.last());
         
         // set record fragment to encrypted output
         record.fragment = cipher.output;
         record.length = record.fragment.length();
         rval = record;
      }
      
      return rval;
   };
   
   /**
    * Handles padding for aes_128_cbc_sha1 in decrypt mode.
    * 
    * @param blockSize the block size.
    * @param output the output buffer.
    * @param decrypt true in decrypt mode, false in encrypt mode.
    * 
    * @return true on success, false on failure.
    */
   var decrypt_aes_128_cbc_sha1_padding = function(blockSize, output, decrypt)
   {
      var rval = true;
      if(decrypt)
      {
         // ensure padding bytes are valid, additional (last) padding byte
         // specifies the length of the padding exclusive of itself, keep
         // checking all bytes even if one is bad to keep time consistent
         var len = output.length();
         var count = output.at(len - 1) + 1;
         for(var i = len - count; i < len; i++)
         {
            rval = rval && (output.at(i) == count);
         }
         if(rval)
         {
            // trim off padding bytes
            output.truncate(count);
         }
      }
      return rval;
   };
   
   /**
    * Decrypts a TLSCipherText record into a TLSCompressed record using
    * AES-128 in CBC mode.
    * 
    * @param record the TLSCipherText record to decrypt.
    * @param s the ConnectionState to use.
    * 
    * @return the TLSCompressed record on success, null on failure.
    */
   var decrypt_aes_128_cbc_sha1 = function(record, s)
   {
      var rval = null;
      
      // FIXME: TLS 1.1 & 1.2 use an explicit IV every time to protect
      // against CBC attacks
      //var iv = record.fragment.getBytes(16);
      
      // use pre-generated IV when initializing for TLS 1.0, otherwise
      // use the residue from the previous decryption
      var iv = s.cipherState.init ? null : s.cipherState.iv;
      s.cipherState.init = true;
      
      // start cipher
      var cipher = s.cipherState.cipher;
      cipher.start(iv);
      
      // do decryption
      if(cipher.update(record.fragment) &&
         cipher.finish(decrypt_aes_128_cbc_sha1_padding))
      {
         // decrypted data:
         // first (len - 20) bytes = application data
         // last 20 bytes          = MAC
         var macLen = s.macLength;
         
         // create a zero'd out mac
         var mac = '';
         for(var i = 0; i < macLen; i++)
         {
            mac += String.fromCharCode(0);
         }
         
         // get fragment and mac
         var len = cipher.output.length();
         if(len >= macLen)
         {
            record.fragment = cipher.output.getBytes(len - macLen);
            mac = cipher.output.getBytes(macLen);
         }
         // bad data, but get bytes anyway to try to keep timing consistent
         else
         {
            record.fragment = cipher.output.getBytes();
         }
         record.length = record.fragment.length();
         
         // see if data integrity checks out
         var mac2 = s.macFunction(s.macKey, s.sequenceNumber++, record);
         if(mac2 === mac)
         {
            rval = record;
         }
      }
      
      return rval;
   };
   
   /**
    * Reads a TLS variable-length vector from a byte buffer.
    * 
    * Variable-length vectors are defined by specifying a subrange of legal
    * lengths, inclusively, using the notation <floor..ceiling>. When these
    * are encoded, the actual length precedes the vector's contents in the byte
    * stream. The length will be in the form of a number consuming as many
    * bytes as required to hold the vector's specified maximum (ceiling)
    * length. A variable-length vector with an actual length field of zero is
    * referred to as an empty vector.
    * 
    * @param b the byte buffer.
    * @param lenBytes the number of bytes required to store the length.
    * 
    * @return the resulting byte buffer.
    */
   var readVector = function(b, lenBytes)
   {
      var len = 0;
      switch(lenBytes)
      {
         case 1:
            len = b.getByte();
            break;
         case 2:
            len = b.getInt16();
            break;
         case 3:
            len = b.getInt24();
            break;
         case 4:
            len = b.getInt32();
            break;
      }
      
      // read vector bytes into a new buffer
      return krypto.util.createBuffer(b.getBytes(len));
   };
   
   /**
    * Writes a TLS variable-length vector to a byte buffer.
    * 
    * @param b the byte buffer.
    * @param lenBytes the number of bytes required to store the length.
    * @param v the byte buffer vector.
    */
   var writeVector = function(b, lenBytes, v)
   {
      // encode length at the start of the vector, where the number
      // of bytes for the length is the maximum number of bytes it
      // would take to encode the vector's ceiling
      b.putInt(v.length(), lenBytes * 8);
      b.putBuffer(v);
   };
   
   /**
    * The tls implementation.
    */
   var tls = {};
   
   /**
    * Version: TLS 1.2 = 3.3, TLS 1.1 = 3.2, TLS 1.0 = 3.1. Both TLS 1.1
    * and TLS 1.2 were still too new (ie: openSSL didn't implement them) at
    * the time of this implementation so TLS 1.0 was implemented instead. 
    */
   tls.Version =
   {
      major: 3,
      minor: 1
   };
   
   /**
    * Maximum fragment size.
    */
   tls.MaxFragment = 16384;
   
   /**
    * Whether this entity is considered the "client" or "server".
    * enum { server, client } ConnectionEnd;
    */
   tls.ConnectionEnd =
   {
      server: 0,
      client: 1
   };
   
   /**
    * Pseudo-random function algorithm used to generate keys from the
    * master secret.
    * enum { tls_prf_sha256 } PRFAlgorithm;
    */
   tls.PRFAlgorithm =
   {
      tls_prf_sha256: 0
   };
   
   /**
    * Bulk encryption algorithms.
    * enum { null, rc4, des3, aes } BulkCipherAlgorithm;
    */
   tls.BulkCipherAlgorithm =
   {
      none: null,
      rc4: 0,
      des3: 1,
      aes: 2
   };
   
   /**
    * Cipher types.
    * enum { stream, block, aead } CipherType;
    */
   tls.CipherType =
   {
      stream: 0,
      block: 1,
      aead: 2
   };
   
   /**
    * MAC (Message Authentication Code) algorithms.
    * enum { null, hmac_md5, hmac_sha1, hmac_sha256,
    *        hmac_sha384, hmac_sha512} MACAlgorithm;
    */
   tls.MACAlgorithm =
   {
      none: null,
      hmac_md5: 0,
      hmac_sha1: 1,
      hmac_sha256: 2,
      hmac_sha384: 3,
      hmac_sha512: 4
   };
   
   /**
    * Compression algorithms.
    * enum { null(0), (255) } CompressionMethod;
    */
   tls.CompressionMethod =
   {
      none: 0
   };
   
   /**
    * TLS record content types.
    * enum {
    *    change_cipher_spec(20), alert(21), handshake(22),
    *    application_data(23), (255)
    * } ContentType;
    */
   tls.ContentType =
   {
      change_cipher_spec: 20,
      alert: 21,
      handshake: 22,
      application_data: 23
   };
   
   /**
    * TLS handshake types.
    * enum {
    *    hello_request(0), client_hello(1), server_hello(2),
    *    certificate(11), server_key_exchange (12),
    *    certificate_request(13), server_hello_done(14),
    *    certificate_verify(15), client_key_exchange(16),
    *    finished(20), (255)
    * } HandshakeType;
    */
   tls.HandshakeType =
   {
      hello_request: 0,
      client_hello: 1,
      server_hello: 2,
      certificate: 11,
      server_key_exchange: 12,
      certificate_request: 13,
      server_hello_done: 14,
      certificate_verify: 15,
      client_key_exchange: 16,
      finished: 20
   };
   
   /**
    * TLS Alert Protocol.
    * 
    * enum { warning(1), fatal(2), (255) } AlertLevel;
    * 
    * enum {
    *    close_notify(0),
    *    unexpected_message(10),
    *    bad_record_mac(20),
    *    decryption_failed(21),
    *    record_overflow(22),
    *    decompression_failure(30),
    *    handshake_failure(40),
    *    bad_certificate(42),
    *    unsupported_certificate(43),
    *    certificate_revoked(44),
    *    certificate_expired(45),
    *    certificate_unknown(46),
    *    illegal_parameter(47),
    *    unknown_ca(48),
    *    access_denied(49),
    *    decode_error(50),
    *    decrypt_error(51),
    *    export_restriction(60),
    *    protocol_version(70),
    *    insufficient_security(71),
    *    internal_error(80),
    *    user_canceled(90),
    *    no_renegotiation(100),
    *    (255)
    * } AlertDescription;
    *
    * struct {
    *    AlertLevel level;
    *    AlertDescription description;
    * } Alert;   
    */
   tls.Alert = {};
   tls.Alert.Level =
   {
      warning: 1,
      fatal: 2
   };
   tls.Alert.Description =
   {
      close_notify: 0,
      unexpected_message: 10,
      bad_record_mac: 20,
      decryption_failed: 21,
      record_overflow: 22,
      decompression_failure: 30,
      handshake_failure: 40,
      bad_certificate: 42,
      unsupported_certificate: 43,
      certificate_revoked: 44,
      certificate_expired: 45,
      certificate_unknown: 46,
      illegal_parameter: 47,
      unknown_ca: 48,
      access_denied: 49,
      decode_error: 50,
      decrypt_error: 51,
      export_restriction: 60,
      protocol_version: 70,
      insufficient_security: 71,
      internal_error: 80,
      user_canceled: 90,
      no_renegotiation: 100
   };
   
   /**
    * Called when an invalid record is encountered.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleInvalid = function(c, record)
   {
      // error case
      c.error(c, {
         message: 'Unexpected message. Received TLS record out of order.',
         client: true,
         alert: {
            level: tls.Alert.Level.fatal,
            description: tls.Alert.Description.unexpected_message
         }
      });
   };
   
   /**
    * Called when the client receives a HelloRequest record.
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleHelloRequest = function(c, record, length)
   {
      // ignore renegotiation requests from the server
      console.log('got HelloRequest');
      
      // send alert warning
      var record = tls.createAlert({
         level: tls.Alert.Level.warning,
         description: tls.Alert.Description.no_renegotiation
      });
      tls.queue(c, record);
      tls.flush(c);
   };
   
   /**
    * Called when the client receives a ServerHello record.
    * 
    * When this message will be sent:
    *    The server will send this message in response to a client hello
    *    message when it was able to find an acceptable set of algorithms.
    *    If it cannot find such a match, it will respond with a handshake
    *    failure alert.
    * 
    * uint24 length;
    * struct {
    *    ProtocolVersion server_version;
    *    Random random;
    *    SessionID session_id;
    *    CipherSuite cipher_suite;
    *    CompressionMethod compression_method;
    *    select(extensions_present) {
    *       case false:
    *          struct {};
    *       case true:
    *          Extension extensions<0..2^16-1>;
    *   };
    * } ServerHello;
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleServerHello = function(c, record, length)
   {
      console.log('got ServerHello');
      
      // minimum of 38 bytes in message
      if(length < 38)
      {
         c.error(c, {
            message: 'Invalid ServerHello message. Message too short.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unexpected_message
            }
         });
      }
      else
      {
         var b = record.fragment;
         var msg =
         {
            version:
            {
               major: b.getByte(),
               minor: b.getByte()
            },
            random: krypto.util.createBuffer(b.getBytes(32)),
            session_id: readVector(b, 1),
            cipher_suite: b.getBytes(2),
            compression_methods: b.getByte(),
            extensions: []
         };
         
         // save server hello
         c.handshakeState.serverHello = msg;
         c.handshakeState.serverRandom = msg.random;
         
         // read extensions if there are any
         if(b.length() > 0)
         {
            msg.extensions = readVector(b, 2); 
         }
         
         // expect a server Certificate message next
         c.expect = SCE;
      }
   };
   
   /**
    * Called when the client receives a Certificate record.
    * 
    * When this message will be sent:
    *    The server must send a certificate whenever the agreed-upon key
    *    exchange method is not an anonymous one. This message will always
    *    immediately follow the server hello message.
    *
    * Meaning of this message:
    *    The certificate type must be appropriate for the selected cipher
    *    suite's key exchange algorithm, and is generally an X.509v3
    *    certificate. It must contain a key which matches the key exchange
    *    method, as follows. Unless otherwise specified, the signing
    *    algorithm for the certificate must be the same as the algorithm
    *    for the certificate key. Unless otherwise specified, the public
    *    key may be of any length.
    * 
    * opaque ASN.1Cert<1..2^24-1>;
    * struct {
    *    ASN.1Cert certificate_list<1..2^24-1>;
    * } Certificate;
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleCertificate = function(c, record, length)
   {
      console.log('got Certificate');
      
      // minimum of 3 bytes in message
      if(length < 3)
      {
         c.error(c, {
            message: 'Invalid Certificate message. Message too short.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unexpected_message
            }
         });
      }
      else
      {
         var b = record.fragment;
         var msg =
         {
            certificate_list: readVector(b, 3)
         };
         
         // FIXME: check server certificate, save in handshake state
         
         // expect a ServerKeyExchange message next
         c.expect = SKE;
      }
   };
   
   /**
    * Called when the client receives a ServerKeyExchange record.
    * 
    * When this message will be sent:
    *    This message will be sent immediately after the server
    *    certificate message (or the server hello message, if this is an
    *    anonymous negotiation).
    *
    *    The server key exchange message is sent by the server only when
    *    the server certificate message (if sent) does not contain enough
    *    data to allow the client to exchange a premaster secret.
    * 
    * Meaning of this message:
    *    This message conveys cryptographic information to allow the
    *    client to communicate the premaster secret: either an RSA public
    *    key to encrypt the premaster secret with, or a Diffie-Hellman
    *    public key with which the client can complete a key exchange
    *    (with the result being the premaster secret.)
    * 
    * enum {
    *    dhe_dss, dhe_rsa, dh_anon, rsa, dh_dss, dh_rsa
    * } KeyExchangeAlgorithm;
    * 
    * struct {
    *    opaque dh_p<1..2^16-1>;
    *    opaque dh_g<1..2^16-1>;
    *    opaque dh_Ys<1..2^16-1>;
    * } ServerDHParams;
    *
    * struct {
    *    select(KeyExchangeAlgorithm) {
    *       case dh_anon:
    *          ServerDHParams params;
    *       case dhe_dss:
    *       case dhe_rsa:
    *          ServerDHParams params;
    *          digitally-signed struct {
    *             opaque client_random[32];
    *             opaque server_random[32];
    *             ServerDHParams params;
    *          } signed_params;
    *       case rsa:
    *       case dh_dss:
    *       case dh_rsa:
    *          struct {};
    *    };
    * } ServerKeyExchange;
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleServerKeyExchange = function(c, record, length)
   {
      console.log('got ServerKeyExchange');
      
      // this implementation only supports RSA, no Diffie-Hellman support
      // so any length > 0 is invalid
      if(length > 0)
      {
         c.error(c, {
            message: 'Invalid key parameters. Only RSA is supported.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unsupported_certificate
            }
         });
      }
      else
      {
         // expect an optional CertificateRequest message next
         c.expect = SCR;
      }
   };
   
   /**
    * Called when the client receives a CertificateRequest record.
    * 
    * When this message will be sent:
    *    A non-anonymous server can optionally request a certificate from
    *    the client, if appropriate for the selected cipher suite. This
    *    message, if sent, will immediately follow the Server Key Exchange
    *    message (if it is sent; otherwise, the Server Certificate
    *    message).
    * 
    * enum {
    *    rsa_sign(1), dss_sign(2), rsa_fixed_dh(3), dss_fixed_dh(4),
    *    rsa_ephemeral_dh_RESERVED(5), dss_ephemeral_dh_RESERVED(6),
    *    fortezza_dms_RESERVED(20), (255)
    * } ClientCertificateType;
    * 
    * opaque DistinguishedName<1..2^16-1>;
    * 
    * struct {
    *    ClientCertificateType certificate_types<1..2^8-1>;
    *    SignatureAndHashAlgorithm supported_signature_algorithms<2^16-1>;
    *    DistinguishedName certificate_authorities<0..2^16-1>;
    * } CertificateRequest;
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleCertificateRequest = function(c, record, length)
   {
      console.log('got CertificateRequest');
      
      // minimum of 5 bytes in message
      if(len < 5)
      {
         c.error(c, {
            message: 'Invalid CertificateRequest. Message too short.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unexpected_message
            }
         });
      }
      else
      {
         var b = record.fragment;
         var msg =
         {
            certificate_types: readVector(b, 1),
            supported_signature_algorithms: readVector(b, 2),
            certificate_authorities: readVector(b, 2)
         };
         
         // save certificate request
         c.handshakeState.certificateRequest = msg;
         
         // expect a ServerHelloDone message next
         c.expect = SHD;
      }
   };
   
   /**
    * Called when the client receives a ServerHelloDone record.
    * 
    * When this message will be sent:
    *    The server hello done message is sent by the server to indicate
    *    the end of the server hello and associated messages. After
    *    sending this message the server will wait for a client response.
    *
    * Meaning of this message:
    *    This message means that the server is done sending messages to
    *    support the key exchange, and the client can proceed with its
    *    phase of the key exchange.
    *
    *    Upon receipt of the server hello done message the client should
    *    verify that the server provided a valid certificate if required
    *    and check that the server hello parameters are acceptable.
    * 
    * struct {} ServerHelloDone;
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleServerHelloDone = function(c, record, length)
   {
      console.log('got ServerHelloDone');
      
      // len must be 0 bytes
      if(length != 0)
      {
         c.error(c, {
            message: 'Invalid ServerHelloDone message. Invalid length.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unexpected_message
            }
         });
      }
      else
      {
         // create all of the client response records:
         record = null;
         
         // create client certificate message if requested
         if(c.handshakeState.certificateRequest !== null)
         {
            // FIXME: determine cert to send
            // FIXME: client-side certs not implemented yet, so send none
            record = tls.createRecord(
            {
               type: tls.ContentType.handshake,
               data: tls.createCertificate([])
            });
            console.log('TLS client certificate record created');
            tls.queue(c, record);
         }
         
         // FIXME: security params are from TLS 1.2, some values like
         // prf_algorithm are ignored for TLS 1.0 and the builtin as specified
         // in the spec is used
         
         // create security parameters
         var sp =
         {
            entity: tls.ConnectionEnd.client,
            prf_algorithm: tls.PRFAlgorithm.tls_prf_sha256,
            bulk_cipher_algorithm: tls.BulkCipherAlgorithm.aes,
            cipher_type: tls.CipherType.block,
            enc_key_length: 128,
            block_length: 16,
            fixed_iv_length: 16,
            record_iv_length: 16,
            mac_algorithm: tls.MACAlgorithm.hmac_sha1,
            mac_length: 80,
            mac_key_length: 20,
            compression_algorithm: tls.CompressionMethod.none,
            pre_master_secret: null,
            master_secret: null,
            client_random: c.handshakeState.clientRandom.bytes(),
            server_random: c.handshakeState.serverRandom.bytes()
         };
         
         // FIXME: get public key from server RSA cert
         // c.handshakeState.serverCertificate
         var pubKey = null;
         
         // create client key exchange message
         record = tls.createRecord(
         {
            type: tls.ContentType.handshake,
            data: tls.createClientKeyExchange(pubKey, sp)
         });
         console.log('TLS client key exchange record created');
         tls.queue(c, record);
         
         if(c.handshakeState.certificateRequest !== null)
         {
            /* FIXME: client certs not yet implemented
            // create certificate verify message
            record = tls.createRecord(
            {
               type: tls.ContentType.handshake,
               data: tls.createCertificateVerify(c.handshakeState)
            });
            console.log('TLS certificate verify record created');
            tls.queue(c, record);
            */
         }
         
         // create change cipher spec message
         record = tls.createRecord(
         {
            type: tls.ContentType.handshake,
            data: tls.createChangeCipherSpec()
         });
         console.log('TLS change cipher spec record created');
         tls.queue(c, record);
         
         // create pending state
         c.state.pending = tls.createConnectionState(c, sp);
         
         // change current write state to pending write state
         c.state.current.write = c.state.pending.write;
         
         // create finished message
         record = tls.createRecord(
         {
            type: tls.ContentType.handshake,
            data: tls.createFinished(c)
         });
         console.log('TLS finished record created');
         tls.queue(c, record);
         
         // expect a server ChangeCipherSpec message next
         c.expect = SCC;
      }
   };
  
   /**
    * Called when the client receives a ChangeCipherSpec record.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleChangeCipherSpec = function(c, record)
   {
      console.log('got ChangeCipherSpec');
      
      if(record.fragment.getByte() != 0x01)      
      {
         c.error(c, {
            message: 'Invalid ChangeCipherSpec message received.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unexpected_message
            }
         });
      }
      else
      {
         // change current read state to pending read state
         c.state.current.read = c.state.pending.read;
         c.state.pending = null;
      }
      
      // expect a Finished record next
      c.expect = SFI;
   };
   
   /**
    * Called when the client receives a Finished record.
    * 
    * When this message will be sent:
    *    A finished message is always sent immediately after a change
    *    cipher spec message to verify that the key exchange and
    *    authentication processes were successful. It is essential that a
    *    change cipher spec message be received between the other
    *    handshake messages and the Finished message.
    *
    * Meaning of this message:
    *    The finished message is the first protected with the just-
    *    negotiated algorithms, keys, and secrets. Recipients of finished
    *    messages must verify that the contents are correct.  Once a side
    *    has sent its Finished message and received and validated the
    *    Finished message from its peer, it may begin to send and receive
    *    application data over the connection.
    * 
    * struct {
    *    opaque verify_data[verify_data_length];
    * } Finished;
    * 
    * verify_data
    *    PRF(master_secret, finished_label, Hash(handshake_messages))
    *       [0..verify_data_length-1];
    * 
    * finished_label
    *    For Finished messages sent by the client, the string
    *    "client finished". For Finished messages sent by the server, the
    *    string "server finished".
    * 
    * verify_data_length depends on the cipher suite. If it is not specified
    * by the cipher suite, then it is 12. Versions of TLS < 1.2 always used
    * 12 bytes.
    * 
    * @param c the connection.
    * @param record the record.
    * @param length the length of the handshake message.
    */
   tls.handleFinished = function(c, record, length)
   {
      console.log('got Finished');
      
      // FIXME: for TLS 1.2 check length against expected verify_data_length
      var vdl = 12;
      if(length != vdl)
      {
         c.error(c, {
            message: 'Invalid Finished message. Invalid length.',
            client: true,
            alert: {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.unexpected_message
            }
         });
      }
      else
      {
         var b = record.fragment;
         var msg =
         {
            verify_data: b.getBytes()
         };
         
         // FIXME: ensure verify data is correct
         
         // FIXME: when creating a new session:
         // FIXME: verify_data should be the same as the one previously
         // calculated when sending Finished message?
         
         // FIXME: when resuming a session:
         // FIXME: calculate verify_data
         // FIXME: create client change cipher spec record and
         // finished record
         
         // expect server application data next
         c.expect = SAD;
         
         // now connected
         c.isConnected = true;
         if(!c.initHandshake)
         {
            c.connected(c);
         }
      }
   };
   
   /**
    * Called when the client receives an Alert record.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleAlert = function(c, record)
   {
      console.log('got Alert');
      
      // read alert
      var b = record.fragment;
      var alert =
      {
         level: b.getByte(),
         description: b.getByte()
      };
      
      // FIXME: consider using a table?
      // get appropriate message
      var msg;
      switch(alert.description)
      {
         case tls.Alert.Description.close_notify:
            msg = 'Connection closed.';
            break;
         case tls.Alert.Description.unexpected_message:
            msg = 'Unexpected message.';
            break;
         case tls.Alert.Description.bad_record_mac:
            msg = 'Bad record MAC.';
            break;
         case tls.Alert.Description.decryption_failed:
            msg = 'Decryption failed.';
            break;
         case tls.Alert.Description.record_overflow:
            msg = 'Record overflow.';
            break;
         case tls.Alert.Description.decompression_failure:
            msg = 'Decompression failed.';
            break;
         case tls.Alert.Description.handshake_failure:
            msg = 'Handshake failure.';
            break;
         case tls.Alert.Description.bad_certificate:
            msg = 'Bad certificate.';
            break;
         case tls.Alert.Description.unsupported_certificate:
            msg = 'Unsupported certificate.';
            break;
         case tls.Alert.Description.certificate_revoked:
            msg = 'Certificate revoked.';
            break;
         case tls.Alert.Description.certificate_expired:
            msg = 'Certificate expired.';
            break;
         case tls.Alert.Description.certificate_unknown:
            msg = 'Certificate unknown.';
            break;
         case tls.Alert.Description.illegal_parameter:
            msg = 'Illegal parameter.';
            break;
         case tls.Alert.Description.unknown_ca:
            msg = 'Unknown certificate authority.';
            break;
         case tls.Alert.Description.access_denied:
            msg = 'Access denied.';
            break;
         case tls.Alert.Description.decode_error:
            msg = 'Decode error.';
            break;
         case tls.Alert.Description.decrypt_error:
            msg = 'Decrypt error.';
            break;
         case tls.Alert.Description.export_restriction:
            msg = 'Export restriction.';
            break;
         case tls.Alert.Description.protocol_version:
            msg = 'Unsupported protocol version.';
            break;
         case tls.Alert.Description.insufficient_security:
            msg = 'Insufficient security.';
            break;
         case tls.Alert.Description.internal_error:
            msg = 'Internal error.';
            break;
         case tls.Alert.Description.user_canceled:
            msg = 'User canceled.';
            break;
         case tls.Alert.Description.no_renegotiation:
            msg = 'Renegotiation not supported.';
            break;
         default:
            msg = 'Unknown error.';
            break;
      };
      
      // call error handler
      c.error(c, {
         message: msg,
         client: false,
         alert: alert
      });
      
      // close connection
      c.close();
   };
   
   /**
    * Called when the client receives a Handshake record.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleHandshake = function(c, record)
   {
      console.log('got Handshake message');
      
      // get the handshake type and message length
      var b = record.fragment;
      var type = b.getByte();
      var length = b.getInt24();
      
      // see if the record fragment doesn't yet contain the full message
      if(length > b.length())
      {
         // cache the record and reset the buffer read pointer before
         // the type and length were read
         c.fragmented = record;
         b.read -= 4;
      }
      else
      {
         // full message now available, clear cache, reset read pointer to
         // before type and length
         c.fragmented = null;
         b.read -= 4;
         
         // update handshake hashes (includes type and length of handshake msg)
         var bytes = b.bytes();
         c.handshakeState.md5.update(bytes);
         c.handshakeState.sha1.update(bytes);
         
         // restore read pointer
         b.read += 4;
         
         // handle message
         if(type in hsTable[c.expect])
         {
            // handle specific handshake type record
            hsTable[c.expect][type](c, record, length);
         }
         else
         {
            // invalid handshake type
            c.error(c, {
               message: 'Invalid handshake type.',
               client: true,
               alert: {
                  level: fatal,
                  description: illegal_parameter
               }
            });
         }
      }
   };
   
   /**
    * Called when the client receives an ApplicationData record.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleApplicationData = function(c, record)
   {
      console.log('got application data');
      // buffer data, notify that its ready
      c.data.putBuffer(record.fragment);
      c.dataReady(c);
   };
   
   /**
    * The transistional state tables for receiving TLS records. It maps
    * the current TLS engine state and a received record to a function to
    * handle the record and update the state.
    * 
    * For instance, if the current state is SHE, then the TLS engine is
    * expecting a ServerHello record. Once a record is received, the handler
    * function is looked up using the state SHE and the record's content type.
    * 
    * The resulting function will either be an error handler or a record
    * handler. The function will take whatever action is appropriate and update
    * the state for the next record.
    * 
    * The states are all based on possible server record types. Note that the
    * client will never specifically expect to receive a HelloRequest or an
    * alert from the server so there is no state that reflects this. These
    * messages may occur at any time.
    * 
    * There are two tables for mapping states because there is a second tier
    * of types for handshake messages. Once a record with a content type of
    * handshake is received, the handshake record handler will look up the
    * handshake type in the secondary map to get its appropriate handler.
    * 
    * Valid message orders are as follows:
    *
    * =======================FULL HANDSHAKE======================
    * Client                                               Server
    * 
    * ClientHello                  -------->
    *                                                 ServerHello
    *                                                Certificate*
    *                                          ServerKeyExchange*
    *                                         CertificateRequest*
    *                              <--------      ServerHelloDone
    * Certificate*
    * ClientKeyExchange
    * CertificateVerify*
    * [ChangeCipherSpec]
    * Finished                     -------->
    *                                          [ChangeCipherSpec]
    *                              <--------             Finished
    * Application Data             <------->     Application Data
    * 
    * =====================SESSION RESUMPTION=====================
    * Client                                                Server
    *
    * ClientHello                   -------->
    *                                                  ServerHello
    *                                           [ChangeCipherSpec]
    *                               <--------             Finished
    * [ChangeCipherSpec]
    * Finished                      -------->
    * Application Data              <------->     Application Data   
    */
   // expect states (indicate which records are expected to be received)
   var SHE = 0; // rcv server hello
   var SCE = 1; // rcv server certificate
   var SKE = 3; // rcv server key exchange
   var SCR = 4; // rcv certificate request
   var SHD = 5; // rcv server hello done
   var SCC = 6; // rcv change cipher spec
   var SFI = 7; // rcv finished
   var SAD = 8; // rcv application data
   
   // map current expect state and content type to function
   var __ = tls.handleInvalid;
   var F0 = tls.handleChangeCipherSpec;
   var F1 = tls.handleAlert;
   var F2 = tls.handleHandshake;
   var F3 = tls.handleApplicationData;
   var ctTable = [
   //      CC,AL,HS,AD
   /*SHE*/[__,__,F2,__],
   /*SCE*/[__,F1,F2,__],
   /*SKE*/[__,F1,F2,__],
   /*SCR*/[__,F1,F2,__],
   /*SHD*/[__,F1,F2,__],
   /*SCC*/[F0,F1,__,__],
   /*SFI*/[__,F1,F2,__]
   /*SAD*/[__,F1,F2,F3]
   ];
   
   // map current expect state and handshake type to function
   var F4 = tls.handleHelloRequest;
   var F5 = tls.handleServerHello;
   var F6 = tls.handleCertificate;
   var F7 = tls.handleServerKeyExchange;
   var F8 = tls.handleCertificateRequest;
   var F9 = tls.handleServerHelloDone;
   var FA = tls.handleCertificateVerify;
   var FB = tls.handleFinished;
   var hsTable = [
   //      HR,01,SH,03,04,05,06,07,08,09,10,SC,SK,CR,HD,CV,CK,17,18,19,FI
   /*SHE*/[__,__,F5,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__],
   /*SCE*/[F4,__,__,__,__,__,__,__,__,__,__,F6,F7,F8,F9,__,__,__,__,__,__],
   /*SKE*/[F4,__,__,__,__,__,__,__,__,__,__,__,F7,F8,F9,__,__,__,__,__,__],
   /*SCR*/[F4,__,__,__,__,__,__,__,__,__,__,__,__,F8,F9,__,__,__,__,__,__],
   /*SHD*/[F4,__,__,__,__,__,__,__,__,__,__,__,__,__,F9,__,__,__,__,__,__],
   /*SCC*/[F4,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__],
   /*SFI*/[F4,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,FB],
   /*SAD*/[F4,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__]
   ];
   
   /**
    * Generates the master_secret and keys using the given security parameters.
    * 
    * The security parameters for a TLS connection state are defined as
    * such:
    * 
    * struct {
    *    ConnectionEnd          entity;
    *    PRFAlgorithm           prf_algorithm;
    *    BulkCipherAlgorithm    bulk_cipher_algorithm;
    *    CipherType             cipher_type;
    *    uint8                  enc_key_length;
    *    uint8                  block_length;
    *    uint8                  fixed_iv_length;
    *    uint8                  record_iv_length;
    *    MACAlgorithm           mac_algorithm;
    *    uint8                  mac_length;
    *    uint8                  mac_key_length;
    *    CompressionMethod      compression_algorithm;
    *    opaque                 master_secret[48];
    *    opaque                 client_random[32]; 
    *    opaque                 server_random[32];
    * } SecurityParameters;
    * 
    * Note that this definition is from TLS 1.2. In TLS 1.0 some of these
    * parameters are ignored because, for instance, the PRFAlgorithm is a
    * builtin-fixed algorithm combining iterations of MD5 and SHA-1 in
    * TLS 1.0.
    * 
    * The Record Protocol requires an algorithm to generate keys required
    * by the current connection state.
    * 
    * The master secret is expanded into a sequence of secure bytes, which
    * is then split to a client write MAC key, a server write MAC key, a
    * client write encryption key, and a server write encryption key. In TLS
    * 1.0 a client write IV and server write IV are also generated. Each
    * of these is generated from the byte sequence in that order. Unused
    * values are empty. In TLS 1.2, some AEAD ciphers may additionally require
    * a client write IV and a server write IV (see Section 6.2.3.3).
    *
    * When keys, MAC keys, and IVs are generated, the master secret is used as
    * an entropy source.
    *
    * To generate the key material, compute:
    * 
    * master_secret = PRF(pre_master_secret, "master secret",
    *                     ClientHello.random + ServerHello.random)
    *
    * key_block = PRF(SecurityParameters.master_secret,
    *                 "key expansion",
    *                 SecurityParameters.server_random +
    *                 SecurityParameters.client_random);
    *
    * until enough output has been generated. Then, the key_block is
    * partitioned as follows:
    *
    * client_write_MAC_key[SecurityParameters.mac_key_length]
    * server_write_MAC_key[SecurityParameters.mac_key_length]
    * client_write_key[SecurityParameters.enc_key_length]
    * server_write_key[SecurityParameters.enc_key_length]
    * client_write_IV[SecurityParameters.fixed_iv_length]
    * server_write_IV[SecurityParameters.fixed_iv_length]
    *
    * In TLS 1.2, the client_write_IV and server_write_IV are only generated
    * for implicit nonce techniques as described in Section 3.2.1 of
    * [AEAD]. This implementation uses TLS 1.0 so IVs are generated.
    *
    * Implementation note: The currently defined cipher suite which
    * requires the most material is AES_256_CBC_SHA256. It requires 2 x 32
    * byte keys and 2 x 32 byte MAC keys, for a total 128 bytes of key
    * material. In TLS 1.0 it also requires 2 x 16 byte IVs, so it actually
    * takes 160 bytes of key material.
    * 
    * @param sp the security parameters to use.
    * 
    * @return the security keys.
    */
   tls.generateKeys = function(sp)
   {
      // TLS_RSA_WITH_AES_128_CBC_SHA (required to be compliant with TLS 1.2)
      // is the only cipher suite implemented at present
      
      // TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA is required to be compliant with
      // TLS 1.0 but we don't care because AES is better and we have an
      // implementation for it
      
      // FIXME: TLS 1.2 implementation
      /*
      // determine the PRF
      var prf;
      switch(sp.prf_algorithm)
      {
         case tls.PRFAlgorithm.tls_prf_sha256:
            prf = prf_sha256;
            break;
         default:
            // should never happen
            throw 'Invalid PRF';
            break;
      }
      
      // concatenate server and client random
      var random = sp.server_random.concat(sp.client_random);
      
      // create master secret, clean up pre-master secret
      prf(sp.pre_master_secret, 'master secret', random);
      sp.pre_master_secret = null;
      
      // generate the amount of key material needed
      var len = 2 * sp.mac_key_length + 2 * sp.enc_key_length;
      var key_block;
      var km = [];
      while(km.length < len)
      {
         key_block = prf(sp.master_secret, 'key expansion', random);
         km = km.concat(key_block);
      }
      
      // split the key material into the MAC and encryption keys
      return {
         client_write_MAC_key: km.splice(0, sp.mac_key_length),
         server_write_MAC_key: km.splice(0, sp.mac_key_length),
         client_write_key: km.splice(0, sp.enc_key_length),
         server_write_key: km.splice(0, sp.enc_key_length)
      };
      */
      
      // TLS 1.0 implementation
      var prf = prf_TLS1;
      
      // concatenate server and client random
      var random = sp.server_random.concat(sp.client_random);
      
      // create master secret, clean up pre-master secret
      prf(sp.pre_master_secret, 'master secret', random);
      sp.pre_master_secret = null;
      
      // generate the amount of key material needed
      var len =
         2 * sp.mac_key_length +
         2 * sp.enc_key_length +
         2 * sp.fixed_iv_length;
      var key_block;
      var km = [];
      while(km.length < len)
      {
         key_block = prf(sp.master_secret, 'key expansion', random);
         km = km.concat(key_block);
      }
      
      // split the key material into the MAC and encryption keys
      return {
         client_write_MAC_key: km.splice(0, sp.mac_key_length),
         server_write_MAC_key: km.splice(0, sp.mac_key_length),
         client_write_key: km.splice(0, sp.enc_key_length),
         server_write_key: km.splice(0, sp.enc_key_length),
         client_write_IV: km.splice(0, sp.fixed_iv_length),
         server_write_IV: km.splice(0, sp.fixed_iv_length)
      };
   };

   /**
    * Creates a new initialized TLS connection state. A connection state has
    * a read mode and a write mode.
    * 
    * compression state:
    *    The current state of the compression algorithm.
    * 
    * cipher state:
    *    The current state of the encryption algorithm. This will consist of
    *    the scheduled key for that connection. For stream ciphers, this
    *    will also contain whatever state information is necessary to allow
    *    the stream to continue to encrypt or decrypt data.
    * 
    * MAC key:
    *    The MAC key for the connection.
    * 
    * sequence number:
    *    Each connection state contains a sequence number, which is
    *    maintained separately for read and write states. The sequence
    *    number MUST be set to zero whenever a connection state is made the
    *    active state. Sequence numbers are of type uint64 and may not
    *    exceed 2^64-1.  Sequence numbers do not wrap. If a TLS
    *    implementation would need to wrap a sequence number, it must
    *    renegotiate instead. A sequence number is incremented after each
    *    record: specifically, the first record transmitted under a
    *    particular connection state MUST use sequence number 0.
    * 
    * @param c the connection.
    * @param sp the security parameters used to initialize the state.
    * 
    * @return the new initialized TLS connection state.
    */
   tls.createConnectionState = function(c, sp)
   {
      var createMode = function()
      {
         var mode =
         {
            sequenceNumber: 0,
            macKey: null,
            macLength: 0,
            macFunction: null,
            cipherState: null,
            cipherFunction: function(record){return true;},
            compressionState: null,
            compressFunction: function(record){return true;}
         };
         return mode;
      };
      var state =
      {
         read: createMode(),
         write: createMode()
      };
      
      // update function in read mode will decrypt then decompress a record
      state.read.update = function(c, record)
      {
         if(!state.read.cipherFunction(record, state.read))
         {
            c.error(c, {
               message: 'Could not decrypt record.',
               client: true,
               alert: {
                  level: tls.Alert.Level.fatal,
                  description: tls.Alert.Description.decryption_failed
               }
            });
         }
         else if(!state.read.compressFunction(record, state.read))
         {
            c.error(c, {
               message: 'Could not decompress record.',
               client: true,
               alert: {
                  level: tls.Alert.Level.fatal,
                  description: tls.Alert.Description.decompression_failure
               }
            });
         }
         return !c.fail;
      };
      
      // update function in write mode will compress then encrypt a record
      state.write.update = function(c, record)
      {
         if(!state.write.compressFunction(record, state.read))
         {
            c.error(c, {
               message: 'Could not compress record.',
               client: true,
               alert: {
                  level: tls.Alert.Level.fatal,
                  description: tls.Alert.Description.internal_error
               }
            });
         }
         else if(!state.write.cipherFunction(record, state.read))
         {
            c.error(c, {
               message: 'Could not encrypt record.',
               client: true,
               alert: {
                  level: tls.Alert.Level.fatal,
                  description: tls.Alert.Description.internal_error
               }
            });
         }
         return !c.fail;
      };
      
      // handle security parameters
      if(sp)
      {
         // generate keys
         var keys = tls.generateKeys(sp);
         
         // mac setup
         state.read.macKey = keys.server_write_MAC_key;
         state.write.macKey = keys.client_write_MAC_key;
         state.read.macLength = state.write.macLength = sp.mac_length;
         switch(sp.mac_algorithm)
         {
            case tls.MACAlgorithm.hmac_sha1:
               state.read.macFunction = state.write.macFunction = hmac_sha1;
               break;
             default:
                throw 'Unsupported MAC algorithm';
         }
         
         // cipher setup
         switch(sp.bulk_cipher_algorithm)
         {
            case tls.BulkCipherAlgorithm.aes:
               console.log('creating aes cipher');
               state.read.cipherState =
               {
                  init: false,
                  cipher: krypto.aes.createDecryptionCipher(
                     keys.server_write_key),
                  iv: keys.server_write_IV
               };
               state.write.cipherState =
               {
                  init: false,
                  cipher: krypto.aes.createEncryptionCipher(
                     keys.client_write_key),
                  iv: keys.client_write_IV
               };
               state.read.cipherFunction = decrypt_aes_128_cbc_sha1;
               state.write.cipherFunction = encrypt_aes_128_cbc_sha1;
               break;
            default:
               throw 'Unsupported cipher algorithm';
         }
         switch(sp.cipher_type)
         {
            case tls.CipherType.block:
               break;
            default:
               throw 'Unsupported cipher type';
         }
         
         // compression setup
         switch(sp.compression_algorithm)
         {
            case tls.CompressionMethod.none:
               break;
            default:
               throw 'Unsupported compression algorithm';
         }
      }
      
      console.log('CREATED STATE', state);
      
      return state;
   };
   
   /**
    * Creates a Random structure.
    * 
    * struct {
    *    uint32 gmt_unix_time;
    *    opaque random_bytes[28];
    * } Random;
    * 
    * gmt_unix_time:
    *    The current time and date in standard UNIX 32-bit format
    *    (seconds since the midnight starting Jan 1, 1970, UTC, ignoring
    *    leap seconds) according to the sender's internal clock. Clocks
    *    are not required to be set correctly by the basic TLS protocol;
    *    higher-level or application protocols may define additional
    *    requirements. Note that, for historical reasons, the data
    *    element is named using GMT, the predecessor of the current
    *    worldwide time base, UTC.
    * random_bytes:
    *    28 bytes generated by a secure random number generator.
    * 
    * @return the Random structure as a byte array.
    */
   tls.createRandom = function()
   {
      // get UTC milliseconds
      var d = new Date();
      var utc = +d + d.getTimezoneOffset() * 60000;
      var rval = krypto.util.createBuffer();
      rval.putInt32(utc);
      rval.putBytes(krypto.random.getBytes(28));
      return rval;
   };
   
   /**
    * Creates a TLS record with the given type and data.
    * 
    * @param options:
    *    type: the record type.
    *    data: the plain text data in a byte buffer.
    * 
    * @return the created record.
    */
   tls.createRecord = function(options)
   {
      console.log('creating TLS record', options);
      var record =
      {
         type: options.type,
         version:
         {
            major: tls.Version.major,
            minor: tls.Version.minor
         },
         length: options.data.length(),
         fragment: options.data
      };
      return record;
   };
   
   /**
    * Creates a TLS alert record.
    * 
    * @param alert:
    *    level: the TLS alert level.
    *    description: the TLS alert description.
    * 
    * @return the created alert record.
    */
   tls.createAlert = function(alert)
   {
      console.log('creating TLS alert record', alert);
      
      var b = krypto.util.createBuffer();
      b.putByte(alert.level);
      b.putByte(alert.description);
      return tls.createRecord({
         type: tls.ContentType.alert,
         data: b
      });
   };
   
   /* The structure of a TLS handshake message.
    * 
    * struct {
    *    HandshakeType msg_type;    // handshake type
    *    uint24 length;             // bytes in message
    *    select(HandshakeType) {
    *       case hello_request:       HelloRequest;
    *       case client_hello:        ClientHello;
    *       case server_hello:        ServerHello;
    *       case certificate:         Certificate;
    *       case server_key_exchange: ServerKeyExchange;
    *       case certificate_request: CertificateRequest;
    *       case server_hello_done:   ServerHelloDone;
    *       case certificate_verify:  CertificateVerify;
    *       case client_key_exchange: ClientKeyExchange;
    *       case finished:            Finished;
    *    } body;
    * } Handshake;
    */
   
   /**
    * Creates a ClientHello message.
    * 
    * opaque SessionID<0..32>;
    * enum { null(0), (255) } CompressionMethod;
    * uint8 CipherSuite[2];
    * 
    * struct {
    *    ProtocolVersion client_version;
    *    Random random;
    *    SessionID session_id;
    *    CipherSuite cipher_suites<2..2^16-2>;
    *    CompressionMethod compression_methods<1..2^8-1>;
    *    select(extensions_present) {
    *        case false:
    *            struct {};
    *        case true:
    *            Extension extensions<0..2^16-1>;
    *    };
    * } ClientHello;
    * 
    * @param sessionId the session ID to use.
    * @param random the client random structure to use.
    * 
    * @return the ClientHello byte buffer.
    */
   tls.createClientHello = function(sessionId, random)
   {
      // determine length of the handshake message
      // cipher suites and compression methods size will need to be
      // updated if more get added to the list
      var length =
         sessionId.length + 1 + // session ID vector
         2 +                    // version (major + minor)
         4 + 28 +               // random time and random bytes
         2 + 2 +                // cipher suites vector (1 supported)
         1 + 1 +                // compression methods vector
         0;                     // no extensions (FIXME: add TLS SNI)
      
      // create supported cipher suites, only 1 at present
      // TLS_RSA_WITH_AES_128_CBC_SHA = { 0x00,0x2F }
      var cipherSuites = krypto.util.createBuffer();
      cipherSuites.putByte(0x00);
      cipherSuites.putByte(0x2F);
      
      // create supported compression methods, only null supported
      var compressionMethods = krypto.util.createBuffer();
      compressionMethods.putByte(0x00);

      // build record fragment
      var rval = krypto.util.createBuffer();
      rval.putByte(tls.HandshakeType.client_hello);
      rval.putInt24(length);               // handshake length
      rval.putByte(tls.Version.major);     // major version
      rval.putByte(tls.Version.minor);     // minor version
      rval.putBytes(random.bytes());       // random time + bytes
      writeVector(rval, 1, krypto.util.createBuffer(sessionId));
      writeVector(rval, 2, cipherSuites);
      writeVector(rval, 1, compressionMethods);
      return rval;
   };
   
   /**
    * Creates a Certificate message.
    * 
    * When this message will be sent:
    *    This is the first message the client can send after receiving a
    *    server hello done message. This message is only sent if the
    *    server requests a certificate. If no suitable certificate is
    *    available, the client should send a certificate message
    *    containing no certificates. If client authentication is required
    *    by the server for the handshake to continue, it may respond with
    *    a fatal handshake failure alert.
    * 
    * opaque ASN.1Cert<1..2^24-1>;
    *
    * struct {
    *     ASN.1Cert certificate_list<0..2^24-1>;
    * } Certificate;
    * 
    * @param certs the certificates to use.
    * 
    * @return the Certificate byte buffer.
    */
   tls.createCertificate = function(certs)
   {
      var certBuffer = krypto.util.createBuffer();
      // FIXME: fill buffer with certs to support client side certs
      
      // determine length of the handshake message
      var length = 3 + certBuffer.length(); // cert vector
      
      // build record fragment
      var rval = krypto.util.createBuffer();
      rval.putByte(tls.HandshakeType.certificate);
      rval.putInt24(length);               // handshake length
      writeVector(rval, 3, certBuffer);
      return rval;
   };
   
   /**
    * Creates a ClientKeyExchange message.
    * 
    * When this message will be sent:
    *    This message is always sent by the client. It will immediately
    *    follow the client certificate message, if it is sent. Otherwise
    *    it will be the first message sent by the client after it receives
    *    the server hello done message.
    *
    * Meaning of this message:
    *    With this message, the premaster secret is set, either though
    *    direct transmission of the RSA-encrypted secret, or by the
    *    transmission of Diffie-Hellman parameters which will allow each
    *    side to agree upon the same premaster secret. When the key
    *    exchange method is DH_RSA or DH_DSS, client certification has
    *    been requested, and the client was able to respond with a
    *    certificate which contained a Diffie-Hellman public key whose
    *    parameters (group and generator) matched those specified by the
    *    server in its certificate, this message will not contain any
    *    data.
    * 
    * Meaning of this message:
    *    If RSA is being used for key agreement and authentication, the
    *    client generates a 48-byte premaster secret, encrypts it using
    *    the public key from the server's certificate or the temporary RSA
    *    key provided in a server key exchange message, and sends the
    *    result in an encrypted premaster secret message. This structure
    *    is a variant of the client key exchange message, not a message in
    *    itself.
    * 
    * struct {
    *    select(KeyExchangeAlgorithm) {
    *       case rsa: EncryptedPreMasterSecret;
    *       case diffie_hellman: ClientDiffieHellmanPublic;
    *    } exchange_keys;
    * } ClientKeyExchange;
    * 
    * struct {
    *    ProtocolVersion client_version;
    *    opaque random[46];
    * } PreMasterSecret;
    *
    * struct {
    *    public-key-encrypted PreMasterSecret pre_master_secret;
    * } EncryptedPreMasterSecret;
    * 
    * @param pubKey the RSA public key to use to encrypt the pre-master secret.
    * @param sp the security parameters to set the pre_master_secret in.
    * 
    * @return the ClientKeyExchange byte buffer.
    */
   tls.createClientKeyExchange = function(pubKey, sp)
   {
      // create buffer to encrypt
      var b = krypto.util.createBuffer();
      
      // add highest client-supported protocol to help server avoid version
      // rollback attacks
      b.putByte(tls.Version.major);
      b.putByte(tls.Version.minor);
      
      // generate and add 46 random bytes
      b.putBytes(krypto.random.getBytes(46));
      
      // save pre_master_secret
      sp.pre_master_secret = b.bytes();
      
      // FIXME: do RSA encryption
      console.log('FIXME: do RSA encryption');
      
      // determine length of the handshake message
      var length = b.length();
      
      // build record fragment
      var rval = krypto.util.createBuffer();
      rval.putByte(tls.HandshakeType.client_key_exchange);
      rval.putInt24(length);
      rval.putBuffer(b);
      return rval;
   };
   
   /**
    * Creates a CertificateVerify message.
    * 
    * Meaning of this message:
    *    This structure conveys the client's Diffie-Hellman public value
    *    (Yc) if it was not already included in the client's certificate.
    *    The encoding used for Yc is determined by the enumerated
    *    PublicValueEncoding. This structure is a variant of the client
    *    key exchange message, not a message in itself.
    *   
    * When this message will be sent:
    *    This message is used to provide explicit verification of a client
    *    certificate. This message is only sent following a client
    *    certificate that has signing capability (i.e. all certificates
    *    except those containing fixed Diffie-Hellman parameters). When
    *    sent, it will immediately follow the client key exchange message.
    * 
    * struct {
    *    Signature signature;
    * } CertificateVerify;
    *   
    * CertificateVerify.signature.md5_hash
    *    MD5(handshake_messages);
    *
    * Certificate.signature.sha_hash
    *    SHA(handshake_messages);
    *
    * Here handshake_messages refers to all handshake messages sent or
    * received starting at client hello up to but not including this
    * message, including the type and length fields of the handshake
    * messages.
    * 
    * @param privKey the private key to sign with.
    * @param hs the current handshake state.
    * 
    * @return the CertificateVerify byte buffer.
    */
   tls.createCertificateVerify = function(privKey, hs)
   {
      // FIXME: not used because client-side certificates not implemented
      // FIXME: combine hs.md5.digest() and hs.sha1.digest() to produce
      // signature
   };
   
   /**
    * Creates a ChangeCipherSpec message.
    * 
    * The change cipher spec protocol exists to signal transitions in
    * ciphering strategies. The protocol consists of a single message,
    * which is encrypted and compressed under the current (not the pending)
    * connection state. The message consists of a single byte of value 1.
    * 
    * struct {
    *    enum { change_cipher_spec(1), (255) } type;
    * } ChangeCipherSpec;
    * 
    * @return the ChangeCipherSpec byte buffer.
    */
   tls.createChangeCipherSpec = function()
   {
      var rval = krypto.util.createBuffer();
      rval.putByte(0x01);
      return rval;
   };
   
   /**
    * Creates a Finished message.
    * 
    * struct {
    *    opaque verify_data[12];
    * } Finished;
    *
    * verify_data
    *    PRF(master_secret, finished_label, MD5(handshake_messages) +
    *    SHA-1(handshake_messages)) [0..11];
    *
    * finished_label
    *    For Finished messages sent by the client, the string "client
    *    finished". For Finished messages sent by the server, the
    *    string "server finished".
    *
    * handshake_messages
    *    All of the data from all handshake messages up to but not
    *    including this message. This is only data visible at the
    *    handshake layer and does not include record layer headers.
    *    This is the concatenation of all the Handshake structures as
    *    defined in 7.4 exchanged thus far.
    * 
    * @param c the connection.
    * 
    * @return the Finished byte buffer.
    */
   tls.createFinished = function(c)
   {
      var rval = krypto.util.createBuffer();
      // FIXME: generate verify_data
      return rval;
   };
   
   /**
    * Fragments, compresses, encrypts, and queues a record for delivery.
    * 
    * @param c the connection.
    * @param record the record to queue.
    */
   tls.queue = function(c, record)
   {
      console.log('queuing TLS record', record);
      
      // if the record is a handshake record, update handshake hashes
      if(record.type === tls.ContentType.handshake)
      {
         var bytes = record.fragment.bytes();
         c.handshakeState.md5.update(bytes);
         c.handshakeState.sha1.update(bytes);
      }
      
      // handle record fragmentation
      var records;
      if(record.fragment.length() <= tls.MaxFragment)
      {
         records = [record];
      }
      else
      {
         // fragment data as long as it is too long
         records = [];
         var data = record.fragment.bytes();
         while(data.length > tls.MaxFragment)
         {
            records.push(tls.createRecord(
            {
               type: record.type,
               data: krypto.util.createBuffer(data.splice(0, tls.MaxFragment))
            }));
         }
         // add last record
         if(data.length > 0)
         {
            records.push(tls.createRecord(
            {
               type: record.type,
               data: krypto.util.createBuffer(data)
            }));
         }
      }
      
      // compress and encrypt all fragmented records
      for(var i = 0; i < records.length && !c.fail; ++i)
      {
         // update the record using current write state
         var rec = records[i];
         var s = c.state.current.write;
         if(s.update(c, rec))
         {
            console.log('TLS record queued');
            // store record
            c.records.push(rec);
         }
      }
   };
   
   /**
    * Flushes all queued records to the output buffer and calls the
    * tlsDataReady() handler on the given connection.
    * 
    * @param c the connection.
    * 
    * @return true on success, false on failure.
    */
   tls.flush = function(c)
   {
      console.log('flushing TLS records');
      for(var i = 0; i < c.records.length; i++)
      {
         var record = c.records[i];
         console.log('flushing TLS record', record);
         
         // add record header and fragment
         c.tlsData.putByte(record.type);
         c.tlsData.putByte(record.version.major);
         c.tlsData.putByte(record.version.minor);
         c.tlsData.putInt16(record.fragment.length());
         c.tlsData.putBuffer(c.records[i].fragment);
      }
      c.records = [];
      console.log('TLS flush finished');
      return c.tlsDataReady(c);
   };
   
   /**
    * Creates a new TLS connection.
    * 
    * See public createConnection() docs for more details.
    * 
    * @param options the options for this connection.
    * 
    * @return the new TLS connection.
    */
   tls.createConnection = function(options)
   {
      // create TLS connection
      var c =
      {
         sessionId: options.sessionId,
         state:
         {
            pending: null,
            current: tls.createConnectionState(c)
         },
         expect: SHE,
         fragmented: null,
         records: [],
         initHandshake: false,
         handshakeState: null,
         isConnected: false,
         fail: false,
         input: krypto.util.createBuffer(),
         tlsData: krypto.util.createBuffer(),
         data: krypto.util.createBuffer(),
         connected: options.connected,
         tlsDataReady: options.tlsDataReady,
         dataReady: options.dataReady,
         closed: options.closed,
         error: function(c, ex)
         {
            // send TLS alert if origin is client
            if(ex.client)
            {
               var record = tls.createAlert(ex.alert);
               tls.queue(c, record);
               tls.flush(c);
            }
            
            // set fail flag
            c.fail = true;
            options.error(c, ex);
         }
      };
      
      /**
       * Updates the current TLS engine state based on the given record.
       * 
       * @param c the TLS connection.
       * @param record the TLS record to act on.
       */
      var _update = function(c, record)
      {
         console.log('updating TLS engine state');
         // get record handler (align type in table by subtracting lowest)
         var aligned = record.type - tls.ContentType.change_cipher_spec;
         var handler = ctTable[c.expect][aligned];
         if(typeof(handler) === 'undefined')
         {
            // invalid record
            handler = __;
         }
         handler(c, record);
      };
      
      /**
       * Performs a handshake using the TLS Handshake Protocol.
       * 
       * @param sessionId the session ID to use, null to start a new one.
       */
      c.handshake = function(sessionId)
      {
         // FIXME: make sure handshake doesn't take place in the middle of
         // another handshake ... either fail if one is in process or
         // schedule it for later?
         
         // create random
         var random = tls.createRandom();
         
         // FIXME: remove try/catch
         try{
         console.log('doing TLS handshake');
         var record = tls.createRecord(
         {
            type: tls.ContentType.handshake,
            data: tls.createClientHello(sessionId || '', random)
         });
         
         // create handshake state
         c.handshakeState =
         {
            certificateRequest: null,
            clientRandom: random,
            serverRandom: null,
            md5: krypto.md.md5.create(),
            sha1: krypto.md.sha1.create()
         };
         
         console.log('TLS client hello record created');
         tls.queue(c, record);
         tls.flush(c);
         }
         catch(ex)
         {
            console.log(ex);
         }
      };
      
      // used while buffering enough data to read an entire record
      var _record = null;
      
      /**
       * Called when TLS protocol data has been received from somewhere
       * and should be processed by the TLS engine.
       * 
       * @param data the TLS protocol data, as a string, to process.
       * 
       * @return 0 if the data could be processed, otherwise the
       *         number of bytes required for data to be processed.
       */
      c.process = function(data)
      {
         console.log('processing TLS data');
         var rval = 0;
         
         // buffer data, get input length
         var b = c.input;
         b.putBytes(data);
         var len = b.length();
         console.log('data len', len);
         
         // keep processing data until more is needed
         while(rval === 0 && b.length() > 0 && !c.fail)
         {
            // if there is no pending record and there are at least 5 bytes
            // for the record, parse the basic record info
            if(_record === null && len >= 5)
            {
               console.log('got record header');
               _record =
               {
                  type: b.getByte(),
                  version:
                  {
                     major: b.getByte(),
                     minor: b.getByte()
                  },
                  length: b.getInt16(),
                  fragment: krypto.util.createBuffer()
               };
               len -= 5;
               console.log('record length', _record.length);
               
               // check record version
               if(_record.version.major != tls.Version.major ||
                  _record.version.minor != tls.Version.minor)
               {
                  c.error(c, {
                     message: 'Incompatible TLS version.',
                     client: true,
                     alert: {
                        level: tls.Alert.Level.fatal,
                        description: tls.Alert.Description.protocol_version
                     }
                  });
                  c.close();
               }
            }
            
            // see if there is enough data to parse the pending record
            if(_record !== null && len >= _record.length)
            {
               console.log('filling record fragment');
               // fill record fragment
               _record.fragment.putBytes(b.getBytes(_record.length));
               
               // update record using current read state
               var s = c.state.current.read;
               if(s.update(c, _record));
               {
                  // if the record type matches a previously fragmented
                  // record, append the record fragment to it
                  if(c.fragmented !== null)
                  {
                     if(c.fragmented.type === _record.type)
                     {
                        console.log('combining fragmented records');
                        // concatenate record fragments
                        c.fragmented.fragment.putBuffer(_record.fragment);
                        _record = c.fragmented;
                     }
                     else
                     {
                        // error, invalid fragmented record
                        c.error(c, {
                           message: 'Invalid fragmented record.',
                           client: true,
                           alert: {
                              level: tls.Alert.Level.fatal,
                              description:
                                 tls.Alert.Description.unexpected_message
                           }
                        });
                     }
                  }
                  
                  // update engine state, clear cached record
                  _update(c, _record);
                  _record = null;
               }
            }
            else if(_record === null)
            {
               // need at least 5 bytes
               rval = 5 - len;
            }
            else
            {
               // need remainder of record
               rval = _record.length - len;
            }
         }
         
         return rval;
      };
      
      /**
       * Requests that application data be packaged into a TLS record.
       * The tlsDataReady handler will be called when the TLS record(s) have
       * been prepared.
       * 
       * @param data the application data, as a string, to be sent.
       */
      c.prepare = function(data)
      {
         var record = tls.createRecord(
         {
            type: tls.ContentType.application_data,
            data: krypto.util.createBuffer(data)
         });
         tls.queue(c, record);
         tls.flush(c);
      };
      
      /**
       * Closes the connection (sends a close_notify message).
       */
      c.close = function()
      {
         if(c.isConnected)
         {
            // send close_notify alert
            var record = tls.createAlert(
            {
               level: tls.Alert.Level.fatal,
               description: tls.Alert.Description.close_notify
            });
            console.log('TLS close_notify alert record created');
            tls.queue(c, record);
            tls.flush(c);
         }
         
         // call handler
         c.closed(c);
         
         // reset TLS engine state
         c = null;
         c = tls.createConnection(options);
      };
      
      return c;
   };
   
   /**
    * The crypto namespace and tls API.
    */
   krypto.tls = {};
   
   /**
    * Creates a new TLS connection. This does not make any assumptions about
    * the transport layer that TLS is working on top of, ie: it does not
    * assume there is a TCP/IP connection or establish one. A TLS connection
    * is totally abstracted away from the layer is runs on top of, it merely
    * establishes a secure channel between a client" and a "server".
    * 
    * A TLS connection contains 4 connection states: pending read and write,
    * and current read and write.
    * 
    * At initialization, the current read and write states will be null.
    * Only once the security parameters have been set and the keys have
    * been generated can the pending states be converted into current
    * states. Current states will be updated for each record processed.
    * 
    * @param options the options for this connection:
    *    sessionId: a session ID to reuse, null for a new connection.
    *    connected: function(conn) called when the first handshake completes.
    *    tlsDataReady: function(conn) called when TLS protocol data has
    *       been prepared and is ready to be used (typically sent over a
    *       socket connection to its destination), read from conn.tlsData
    *       buffer.
    *    dataReady: function(conn) called when application data has
    *       been parsed from a TLS record and should be consumed by the
    *       application, read from conn.data buffer.
    *    closed: function(conn) called when the connection has been closed.
    *    error: function(conn, error) called when there was an error.
    * 
    * @return the new TLS connection.
    */
   krypto.tls.createConnection = function(options)
   {
      return tls.createConnection(options);
   };
   
   /**
    * Wraps a krypto.net socket with a TLS layer.
    * 
    * @param options:
    *    sessionId: a session ID to reuse, null for a new connection.
    *    socket: the socket to wrap.
    * 
    * @return the TLS-wrapped socket.
    */
   krypto.tls.wrapSocket = function(options)
   {
      // get raw socket
      var socket = options.socket;
      
      // create TLS socket
      var tlsSocket =
      {
         id: socket.id,
         // set handlers
         connected: socket.connected || function(e){},
         closed: socket.closed || function(e){},
         data: socket.data || function(e){},
         error: socket.error || function(e){}
      };
      
      // create TLS connection
      var c = krypto.tls.createConnection({
         sessionId: options.sessionId || null,
         connected: function(c)
         {
            console.log('TLS connected');
            // initial handshake complete
            tlsSocket.connected({
               id: socket.id,
               type: 'connect',
               bytesAvailable: c.data.length()
            });
         },
         tlsDataReady: function(c)
         {
            console.log('sending TLS data over socket');
            // send TLS data over socket
            return socket.send(c.tlsData.getBytes());
         },
         dataReady: function(c)
         {
            console.log('TLS application data ready');
            // indicate application data is ready
            tlsSocket.data({
               id: socket.id,
               type: 'socketData',
               bytesAvailable: c.data.length()
            });
         },
         closed: function(c)
         {
            console.log('TLS connection closed');
            // close socket
            socket.close();
         },
         error: function(c, e)
         {
            console.log('TLS error', e);
            // close socket, send error
            socket.close();
            tlsSocket.error({
               id: socket.id,
               type: 'tlsError',
               message: e.message,
               bytesAvailable: 0,
               error: e
            });
         }
      });
      
      // handle doing handshake after connecting
      socket.connected = function(e)
      {
         console.log('connected', e);
         c.handshake(options.sessionId);
      };
      
      // handle closing TLS connection
      socket.closed = function(e)
      {
         c.isConnected = false;
         c.close();
      };
      
      // handle receiving raw TLS data from socket
      var _requiredBytes = 0;
      socket.data = function(e)
      {
         console.log('received TLS data');
         
         // only receive if there are enough bytes available to
         // process a record
         // FIXME: remove try/catch
         try
         {
         if(e.bytesAvailable >= _requiredBytes)
         {
            var count = Math.max(e.bytesAvailable, _requiredBytes);
            var data = socket.receive(count);
            if(data !== null)
            {
               _requiredBytes = c.process(data);
            }
         }
         }
         catch(ex)
         {
            console.log(ex);
         }
      };
      
      /**
       * Destroys this socket.
       */
      tlsSocket.destroy = function()
      {
         socket.destroy();
      };
      
      /**
       * Connects this socket.
       * 
       * @param options:
       *           host: the host to connect to.
       *           port: the port to connect to.
       *           policyPort: the policy port to use (if non-default). 
       */
      tlsSocket.connect = function(options)
      {
         socket.connect(options);
      };
      
      /**
       * Closes this socket.
       */
      tlsSocket.close = function()
      {
         c.close();
      };
      
      /**
       * Determines if the socket is connected or not.
       * 
       * @return true if connected, false if not.
       */
      tlsSocket.isConnected = function()
      {
         socket.isConnected();
      };
      
      /**
       * Writes bytes to this socket.
       * 
       * @param bytes the bytes (as a string) to write.
       * 
       * @return true on success, false on failure.
       */
      tlsSocket.send = function(bytes)
      {
         return c.prepare(bytes);
      };
      
      /**
       * Reads bytes from this socket (non-blocking). Fewer than the number
       * of bytes requested may be read if enough bytes are not available.
       * 
       * This method should be called from the data handler if there are
       * enough bytes available. To see how many bytes are available, check
       * the 'bytesAvailable' property on the event in the data handler or
       * call the bytesAvailable() function on the socket. If the browser is
       * msie, then the bytesAvailable() function should be used to avoid
       * race conditions. Otherwise, using the property on the data handler's
       * event may be quicker.
       * 
       * @param count the maximum number of bytes to read.
       * 
       * @return the bytes read (as a string) or null on error.
       */
      tlsSocket.receive = function(count)
      {
         return c.data.getBytes(count);
      };
      
      /**
       * Gets the number of bytes available for receiving on the socket.
       * 
       * @return the number of bytes available for receiving.
       */
      tlsSocket.bytesAvailable = function()
      {
         return c.data.length();
      };
      
      return tlsSocket;
   };
})();
