/**
 * API for Transport Layer Security (TLS)
 *
 * @author Dave Longley
 *
 * Copyright (c) 2009-2010 Digital Bazaar, Inc. All rights reserved.
 * 
 * An API for using TLS.
 * 
 * FIXME: more docs here
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
 * Note: Client ignores HelloRequest if in the middle of a
 * handshake.
 * 
 * FIXME: title this doc section
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
 * FIXME: Might to need to stuff received TLS records into a queue
 * according to sequence number ... and process them accordingly, without
 * doing evil, slow, gross setTimeout() business.
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
 * FIXME: title encryption section
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
 * FIXME: grab that text and put it here
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
 * 
 * FIXME: title data types
 * 
 * Variable-length vectors are defined by specifying a subrange of legal
 * lengths, inclusively, using the notation <floor..ceiling>. When these are
 * encoded, the actual length precedes the vector's contents in the byte
 * stream. The length will be in the form of a number consuming as many bytes
 * as required to hold the vector's specified maximum (ceiling) length. A
 * variable-length vector with an actual length field of zero is referred to
 * as an empty vector.
 */
(function()
{
   // local alias for krypto stuff
   var krypto = window.krypto;
   
   /**
    * Generates pseudo random bytes using a SHA256 algorithm.
    * 
    * @param ms the master secret.
    * @param ke the key expansion.
    * @param r the server random concatenated with the client random.
    * @param cr the client random bytes.
    */
   var prf_sha256 = function(ms, ke, sr, cr)
   {
      // FIXME: implement me
   };
   
   /**
    * Gets the SHA-1 hash for the given data.
    * 
    * @param data the data.
    * 
    * @return the sha-1 hash (20 bytes) for the given data.
    */
   var hmac_sha1 = function(data)
   {
      // FIXME:
   };
   
   /**
    * Encrypts the TLSCompressed record into a TLSCipherText record using
    * AES-128 in CBC mode.
    * 
    * @param record the TLSCompressed record to encrypt.
    * @param state the ConnectionState to use.
    * 
    * @return the TLSCipherText record on success, null on failure.
    */
   var encrypt_aes_128_cbc_sha1 = function(record, state)
   {
      var rval = null;
      
      // append MAC to fragment
      var mac = state.macFunction(
         state.clientMacKey, record.fragment.getBytes());
      record.fragment.putBytes(mac);
      
      // generate a new IV
      var iv = window.krypto.random.getBytes(16);
      
      // start cipher
      var cipher = state.cipherState.cipher;
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
    * @param state the ConnectionState to use.
    * 
    * @return the TLSCompressed record on success, null on failure.
    */
   var decrypt_aes_128_cbc_sha1 = function(record, state)
   {
      var rval = null;
      
      // get IV from beginning of fragment
      if(record.fragment.length() >= 16)
      {
         var iv = record.fragment.getBytes(16);
         
         // start cipher
         var cipher = state.cipherState.cipher;
         cipher.start(iv);
         
         // do decryption
         if(cipher.update(record.fragment) &&
            cipher.finish(decrypt_aes_128_cbc_sha1_padding))
         {
            // decrypted data:
            // first (len - 20) bytes = application data
            // last 20 bytes          = MAC
            var macLen = state.macLength;
            
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
            
            // see if data integrity checks out
            var mac2 = state.macFunction(state.serverMacKey, record.fragment);
            if(mac2 === mac)
            {
               rval = record;
            }
         }
      }
      
      return rval;
   };
   
   /**
    * Reads a TLS variable-length vector from a byte buffer.
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
   var tls =
   {
      /**
       * Version: TLS 1.2 = 3.3, TLS 1.0 = 3.1
       */
      Version:
      {
         major: 3,
         minor: 3
      },
      
      /**
       * Maximum fragment size.
       */
      MaxFragment: 16384,
      
      /**
       * Whether this entity is considered the "client" or "server".
       * enum { server, client } ConnectionEnd;
       */
      ConnectionEnd:
      {
         server: 0,
         client: 1
      };
      
      /**
       * Pseudo-random function algorithm used to generate keys from the
       * master secret.
       * enum { tls_prf_sha256 } PRFAlgorithm;
       */
      PRFAlgorithm:
      {
         tls_prf_sha256: 0
      },
      
      /**
       * Bulk encryption algorithms.
       * enum { null, rc4, 3des, aes } BulkCipherAlgorithm;
       */
      BulkCipherAlgorithm:
      {
         none: null,
         rc4: 0,
         3des: 1,
         aes: 2
      },
      
      /**
       * Cipher types.
       * enum { stream, block, aead } CipherType;
       */
      CipherType:
      {
         stream: 0,
         block: 1,
         aead: 2
      },
      
      /**
       * MAC (Message Authentication Code) algorithms.
       * enum { null, hmac_md5, hmac_sha1, hmac_sha256,
       *        hmac_sha384, hmac_sha512} MACAlgorithm;
       */
      MACAlgorithm:
      {
         none: null,
         hmac_md5: 0,
         hmac_sha1: 1,
         hmac_sha256: 2,
         hmac_sha384: 3,
         hmac_sha512: 4
      },
      
      /**
       * Compression algorithms.
       * enum { null(0), (255) } CompressionMethod;
       */
      CompressionMethod:
      {
         none: 0
      },
      
      /**
       * TLS record content types.
       * enum {
       *    change_cipher_spec(20), alert(21), handshake(22),
       *    application_data(23), (255)
       * } ContentType;
       */
      ContentType:
      {
         change_cipher_spec: 20,
         alert: 21,
         handshake: 22,
         application_data: 23
      },
      
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
      HandshakeType:
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
      }
   };
   
   /**
    * Called when an invalid record is encountered.
    * 
    * @param c the connection.
    * @param record the record.
    */
   var tls.handleInvalid = function(c, record)
   {
      // FIXME: send alert and close connection?
      
      // error case
      c.error(c, {
         message: 'Received TLS record out of order.'
      });
   };
   
   /**
    * Called when the client receives a HelloRequest record.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleHelloRequest = function(c, record)
   {
      // ignore renegotiation requests from the server
   };
   
   /**
    * Called when the client receives a ServerHello record.
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
    */
   var tls.handleServerHello = function(c, record)
   {
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // minimum of 38 bytes in message
      if(len < 38)
      {
         // FIXME: send TLS alert
         
         c.error(c, {
            message: 'Invalid ServerHello message. Message too short.'
         });
      }
      else
      {
         var msg =
         {
            version:
            {
               major: b.getByte(),
               minor: b.getByte()
            },
            random:
            {
               gmt_unix_time: b.getInt32(),
               random_bytes: b.getBytes(28)
            },
            session_id: readVector(b, 1),
            cipher_suite: b.getBytes(2),
            compression_methods: b.getByte(),
            extensions: []
         };
         
         // read extensions if there are any
         if(b.length() > 0)
         {
            msg.extensions = readVector(b, 2); 
         }
         
         // FIXME: do stuff
         
         // expect a server Certificate message next
         c.expect = SCE;
      }
   };
   
   /**
    * Called when the client receives a Certificate record.
    * 
    * opaque ASN.1Cert<1..2^24-1>;
    * struct {
    *    ASN.1Cert certificate_list<0..2^24-1>;
    * } Certificate;
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleCertificate = function(c, record)
   {
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // minimum of 3 bytes in message
      if(len < 3)
      {
        // FIXME: send TLS alert
         
         c.error(c, {
            message: 'Invalid Certificate message. Message too short.'
         });
      }
      else
      {
         var msg =
         {
            certificate_list: readVector(b, 3)
         };
         
         // FIXME: check server certificate
         
         // expect a ServerKeyExchange message next
         c.expect = SKE;
      }
   };
   
   /**
    * Called when the client receives a ServerKeyExchange record.
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
    */
   tls.handleServerKeyExchange = function(c, record)
   {
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // this implementation only supports RSA, no Diffie-Hellman support
      // so any length > 0 is invalid
      if(len > 0)
      {
         // FIXME: send TLS alert
         
         c.error(c, {
            message: 'Invalid key parameters. Only RSA is supported.'
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
    */
   tls.handleCertificateRequest = function(c, record)
   {
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // minimum of 5 bytes in message
      if(len < 5)
      {
         // FIXME: send TLS alert
         
         c.error(c, {
            message: 'Invalid CertificateRequest. Message too short.'
         });
      }
      else
      {
         var msg =
         {
            certificate_types: readVector(b, 1),
            supported_signature_algorithms: readVector(b, 2),
            certificate_authorities: readVector(b, 2)
         };
         
         // FIXME: this implementation doesn't support client certs
         // FIXME: prepare a client certificate/indicate there isn't one
         
         // expect a ServerHelloDone message next
         c.expect = SHD;
      }
   };
   
   /**
    * Called when the client receives a ServerHelloDone record.
    * 
    * struct {} ServerHelloDone;
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleServerHelloDone = function(c, record)
   {
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // len must be 0 bytes
      if(len != 0)
      {
         // FIXME: send TLS alert
         
         c.error(c, {
            message: 'Invalid ServerHelloDone message. Invalid length.'
         });
      }
      else
      {
         // FIXME: create all of the client response records
         // client certificate
         // client key exchange
         // certificate verify
         // change cipher spec
         // finished
         
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
      // FIXME: handle server has changed their cipher spec, change read
      // pending state to current, get ready for finished record
      
      // expect a Finished record next
      c.expect = SFI;
   };
   
   /**
    * Called when the client receives a Finished record.
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
    */
   tls.handleFinished = function(c, record)
   {
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // FIXME: check length against expected verify_data_length
      var vdl = 12;
      if(len != vdl)
      {
         // FIXME: send TLS alert
         
         c.error(c, {
            message: 'Invalid Finished message. Invalid length.'
         });
      }
      else
      {
         var msg =
         {
            verify_data: b.getBytes(len);
         };
         
         // FIXME: ensure verify data is correct
         // FIXME: create client change cipher spec record and
         // finished record
         
         // expect server application data next
         c.expect = SAD;
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
      // FIXME: handle alert, determine if connection must end
   };
   
   /**
    * Called when the client receives a Handshake record.
    * 
    * @param c the connection.
    * @param record the record.
    */
   tls.handleHandshake = function(c, record)
   {
      // get the handshake type
      var type = record.fragment.byte();
      
      // get the length of the message
      var b = record.fragment;
      var len = b.getInt24();
      
      // see if the record fragment doesn't yet contain the full message
      if(len > b.length())
      {
         // cache the record and reset the buffer read pointer
         c.fragmented = record;
         b.read -= 4;
      }
      else
      {
         // full message now available, clear cache, reset read pointer for len
         c.fragmented = null;
         b.read -= 3;
         
         // handle message
         var f = hsTable[c.expect][type];
         if(f)
         {
            // handle specific handshake type record
            f(c, record);
         }
         else
         {
            // invalid handshake type
            c.error(c, {
               message: 'Invalid handshake type.'
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
    * Creates new initialized SecurityParameters. No compression, encryption,
    * or MAC algorithms will be specified.
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
    * @param options:
    *    
    * @return FIXME:
    */
   tls.createSecurityParameters = function()
   {
      // new parameters have no compression, encryption, or MAC algorithms
      var securityParams =
      {
         entity: tls.ConnectionEnd.client,
         prf_algorithm: tls.PRFAlgorithm.tls_prf_sha256,
         bulk_cipher_algorithm: tls.BulkCipherAlgorithm.none,
         cipher_type: tls.CipherType.block,
         enc_key_length: 0,
         block_length: 0,
         fixed_iv_length: 0,
         record_iv_length: 0,
         mac_algorithm: tls.MACAlgorithm.none,
         mac_length: 0,
         mac_key_length: 0,
         compression_algorithm: tls.CompressionMethod.none,
         master_secret: null,
         client_random: null,
         server_random: null
      };
      return securityParams;
   };
   
   /**
    * Generates keys using the given security parameters.
    * 
    * The Record Protocol requires an algorithm to generate keys required
    * by the current connection state.
    * 
    * The master secret is expanded into a sequence of secure bytes, which
    * is then split to a client write MAC key, a server write MAC key, a
    * client write encryption key, and a server write encryption key. Each
    * of these is generated from the byte sequence in that order. Unused
    * values are empty. Some AEAD ciphers may additionally require a
    * client write IV and a server write IV (see Section 6.2.3.3).
    *
    * When keys and MAC keys are generated, the master secret is used as an
    * entropy source.
    *
    * To generate the key material, compute:
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
    * Currently, the client_write_IV and server_write_IV are only generated
    * for implicit nonce techniques as described in Section 3.2.1 of
    * [AEAD]. This implementation does not use AEAD, so these are not
    * generated.
    *
    * Implementation note: The currently defined cipher suite which
    * requires the most material is AES_256_CBC_SHA256. It requires 2 x 32
    * byte keys and 2 x 32 byte MAC keys, for a total 128 bytes of key
    * material.
    * 
    * @param sp the security parameters to use.
    * 
    * @return the security keys.
    */
   tls.generateKeys = function(sp)
   {
      // TLS_RSA_WITH_AES_128_CBC_SHA (this one is mandatory)
      
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
      
      // generate the amount of key material needed
      var len = 2 * sp.mac_key_length + 2 * sp.enc_key_length;
      var key_block;
      var km = [];
      while(km.length < len)
      {
         key_block = prf(
            sp.master_secret,
            'key expansion',
            sp.server_random.concat(sp.client_random));
         km = km.concat(key_block);
      }
      
      // split the key material into the MAC and encryption keys
      return
      {
         client_write_MAC_key: km.splice(0, sp.mac_key_length),
         server_write_MAC_key: km.splice(0, sp.mac_key_length),
         client_write_key: km.splice(0, sp.enc_key_length),
         server_write_key: km.splice(0, sp.enc_key_length)
      };
   };

   /**
    * Creates a new initialized TLS connection state.
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
    * @param sp the security parameters used to initialize the state.
    * 
    * @return the new initialized TLS connection state.
    */
   tls.createConnectionState = function(sp)
   {
      var state =
      {
         sequenceNumber: 0,
         clientMacKey: null,
         serverMacKey: null,
         macLength: 0,
         macFunction: null,
         cipherState: null,
         encrypt: function(record){return record;},
         decrypt: function(record){return record;},
         compressionState: null,
         compress: function(record){return record;},
         decompress: function(record){return record;}
      };
      
      /* security parameters:
         
         entity: tls.ConnectionEnd.client,
         prf_algorithm: tls.PRFAlgorithm.tls_prf_sha256,
         bulk_cipher_algorithm: tls.BulkCipherAlgorithm.none,
         cipher_type: tls.CipherType.block,
         enc_key_length: 0,
         block_length: 0,
         fixed_iv_length: 0,
         record_iv_length: 0,
         mac_algorithm: tls.MACAlgorithm.none,
         mac_length: 0,
         mac_key_length: 0,
         compression_algorithm: tls.CompressionMethod.none,
         master_secret: null,
         client_random: null,
         server_random: null
      */
      if(sp)
      {
         // generate keys
         state.keys = tls.generateKeys(sp);
         
         // mac setup
         state.serverMacKey = keys.server_write_MAC_key;
         state.clientMacKey = keys.client_write_MAC_key;
         state.macLength = sp.mac_length;
         switch(sp.mac_algorithm)
         {
            case tls.MACAlgorithm.hmac_sha1:
               state.macFunction = hmac_sha1;
               break;
             default:
                throw 'Unsupported MAC algorithm';
         }
         
         // cipher setup
         switch(sp.bulk_cipher_algorithm)
         {
            case tls.BulkCipherAlgorithm.aes:
               cipherState =
               {
                  eCipher: krypto.aes.createEncryptionCipher(
                     keys.client_write_key),
                  dCipher: krypto.aes.createDecryptionCipher(
                     keys.server_write_key)
               };
               state.encrypt = encrypt_aes_128_cbc_sha1;
               state.decrypt = decrypt_aes_128_cbc_sha1;
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
    * @return the Random structure.
    */
   tls.createRandom = function()
   {
      // get UTC milliseconds
      var d = new Date();
      var utc = +d + d.getTimezoneOffset() * 60000;
      return {
         gmt_unix_time: utc,
         random_bytes: krypto.random.getRandomBytes(28)
      };
   };
   
   /**
    * Creates an empty TLS record.
    * 
    * @param options:
    *    type: the record type.
    *    data: the plain text data in a byte buffer.
    * 
    * @return the created record.
    */
   tls.createRecord = function(options)
   {
      var record =
      {
         type: options.type,
         version:
         {
            major: tls.Version.major,
            minor: tls.Version.minor
         },
         length: options.data.length,
         fragment: options.data
      };
      return record;
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
    * 
    * @return the ClientHello byte buffer.
    */
   tls.createClientHello = function(sessionId)
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
      
      // create random
      var random = createRandom();
      
      // create supported cipher suites, only 1 at present
      // TLS_RSA_WITH_AES_128_CBC_SHA = { 0x00,0x2F }
      var cipherSuites = util.createBuffer();
      cipherSuites.putByte(0x00);
      cipherSuites.putByte(0x2F);
      
      // create supported compression methods, only null supported
      var compressionMethods = util.createBuffer();
      compressionMethods.putByte(0x00);

      // build record fragment
      var rval = util.createBuffer();
      rval.putInt24(length);               // handshake length
      rval.putByte(tls.Version.major);     // major version
      rval.putByte(tls.Version.minor);     // minor version
      rval.putInt32(random.gmt_unix_time); // random time
      rval.putBytes(random.random_bytes);  // random bytes
      writeVector(rval, 1, util.createBuffer(sessionId));
      writeVector(rval, 2, cipherSuites);
      writeVector(rval, 1, compressionMethods);
      return rval;
   };
   
   /**
    * Compresses, encrypts, and queues a record for delivery.
    * 
    * @param c the connection.
    * @param record the record to queue.
    */
   tls.queue = function(c, record)
   {
      // compress and encrypt the record using current state
      var s = c.current;
      if(s === null || (s.compress(record, s) && s.encrypt(record, s)))
      {
         // store record
         c.records.push(record);
      }
      // fatal error
      else
      {
         // FIXME: pass on errors from encryption/compression?
         c.error(c, {
            message: 'Could not compress and encrypt record.'
         });
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
      for(var i = 0; i < c.records.length; i++)
      {
         c.tlsData.putBuffer(c.records[i].fragment);
      }
      c.records = [];
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
            pending: tls.createConnectionState(),
            current: null
         },
         expect: SHE,
         fragmented: null,
         records: [],
         input: krypto.util.createBuffer(),
         tlsData: krypto.util.createBuffer(),
         data: krypto.util.createBuffer(),
         tlsDataReady: options.tlsDataReady,
         dataReady: options.dataReady,
         closed: options.closed,
         error: options.error
      };
      
      /**
       * Updates the current TLS engine state based on the given record.
       * 
       * @param c the TLS connection.
       * @param record the TLS record to act on.
       */
      var _update = function(c, record)
      {
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
         var record = tls.createRecord(
         {
            type: tls.ContentType.handshake,
            data: createClientHello(sessionId)
         };
         tls.queue(c, record);
         tls.flush(c);
      };
      
      // used while buffering enough data to read an entire record
      var _record = null;
      
      /**
       * Called when TLS protocol data has been received from somewhere
       * and should be processed by the TLS engine.
       * 
       * @param data the TLS protocol data, as a string, to process.
       * 
       * @return 0 if the data could be processed, otherwise the number of
       *         bytes required for data to be processed.
       */
      c.process = function(data)
      {
         var rval = 0;
         
         // buffer data, get input length
         var b = c.input;
         b.putBytes(data);
         var len = b.length();
         
         // if there is no pending record and there are at least 5 bytes
         // for the record, parse the basic record info
         if(_record === null && len >= 5)
         {
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
         }
         
         // see if there is enough data to parse the pending record
         if(_record !== null && len >= _record.length)
         {
            // fill record fragment
            _record.fragment.putBytes(b.getBytes(_record.length));
            
            // decrypt and decompress record using current state
            var s = c.state.current;
            if(s === null ||
               (s.decrypt(_record, s) && s.decompress(_record, s)))
            {
               // if the record type matches a previously fragmented
               // record, append the record fragment to it
               if(c.fragmented !== null)
               {
                  if(c.fragmented.type === _record.type)
                  {
                     // concatenate record fragments
                     c.fragmented.fragment.putBuffer(_record.fragment);
                     _record = c.fragmented;
                  }
                  else
                  {
                     // error, invalid fragmented record
                     c.error(c, {
                        message: 'Invalid fragmented record.'
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
         // fragment data as necessary
         if(data.length <= tls.MaxFragment)
         {
            var record = tls.createRecord(
            {
               type: tls.ContentType.application_data,
               data: util.createBuffer(data)
            };
            tls.queue(c, record);
         }
         else
         {
            var tmp;
            do
            {
               tmp = data.slice(0, tls.MaxFragment);
               data = data.slice(tls.MaxFragment);
               var record = tls.createRecord(
               {
                  type: tls.ContentType.application_data,
                  data: util.createBuffer(tmp)
               };
               tls.queue(c, record);
            }
            while(data.length > tls.MaxFragment);
         }
         
         // flush queued records
         tls.flush(c);
      };
      
      /**
       * Closes the connection (sends a close_notify message).
       */
      c.close = function()
      {
         // FIXME: implement me
         // FIXME: call closed() handler
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
   krypto.tls.createConnection: function(options)
   {
      return tls.createConnection(options);
   },
   
   /**
    * Wraps a krypto.net socket with a TLS layer. Any existing handlers
    * on the socket will be replaced.
    * 
    * @param options:
    *    sessionId: a session ID to reuse, null for a new connection.
    *    socket: the socket to wrap.
    *    connected: function(event) called when the socket connects.
    *    closed: function(event) called when the socket closes.
    *    data: function(event) called when socket data has arrived,
    *       it can be read from the socket using receive().
    *    error: function(event) called when a socket error occurs.
    * 
    * @return the TLS-wrapped socket.
    */
   krypto.tls.wrapSocket: function(options)
   {
      // get raw socket
      var socket = options.socket;
      
      // create TLS socket
      var tlsSocket =
      {
         id: socket.id,
         // set handlers
         connected: options.connected || function(e){},
         closed: options.closed || function(e){},
         data: options.data || function(e){},
         error: options.error || function(e){}
      };
      
      // create TLS connection
      var c = krypto.tls.createConnection({
         sessionId: options.sessionId || null,
         tlsDataReady: function(c)
         {
            // send TLS data over socket
            return socket.send(c.tlsData.getBytes());
         },
         dataReady: function(c)
         {
            // indicate application data is ready
            tlsSocket.data({
               id: socket.id,
               type: 'socketData',
               bytesAvailable: c.data.length()
            });
         },
         closed: function(c)
         {
            // close socket
            socket.close();
            tlsSocket.closed({
               id: socket.id,
               type: 'close',
               bytesAvailable: 0
            });
         },
         error: function(c, e)
         {
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
      
      // handle receiving raw TLS data from socket
      var _requiredBytes = 0;
      socket.data = function(e)
      {
         // only receive if there are enough bytes available to
         // process a record
         if(e.bytesAvailable >= _requiredBytes)
         {
            var count = Math.max(e.bytesAvailable, _requiredBytes);
            var data = socket.receive(count);
            if(data !== null)
            {
               _requiredBytes = c.process(data);
            }
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
       *           port: the port to connec to.
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
