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
      
      // append sha-1 hash to fragment
      record.fragment.putBytes(hmac_sha1(record.fragment.bytes()));
      
      // generate a new IV
      var iv = window.krypto.random.getBytes(16);
      
      // restart cipher
      var cipher = state.cipherState.cipher;
      cipher.restart(iv);
      
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
   var decrypt_aes_128_cbc_sha1_padding: function(blockSize, output, decrypt)
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
         
         // restart cipher
         var cipher = state.cipherState.cipher;
         cipher.restart(iv);
         
         // do decryption
         if(cipher.update(record.fragment) &&
            cipher.finish(decrypt_aes_128_cbc_sha1_padding))
         {
            // decrypted data:
            // first (len - 20) bytes = application data
            // last 20 bytes          = MAC
            
            // create a zero'd out mac
            var mac = '';
            for(var i = 0; i < 20; i++)
            {
               mac[i] = String.fromCharCode(0);
            }
            
            // get fragment and mac
            var len = cipher.output.length();
            if(len >= 20)
            {
               record.fragment = cipher.output.getBytes(len - 20);
               mac = cipher.output.getBytes(20);
            }
            // bad data, but get bytes anyway to try to keep timing consistent
            else
            {
               record.fragment = cipher.output.getBytes();
            }
            
            // see if data integrity checks out
            if(hmac_sha1(record.fragment) == mac)
            {
               rval = record;
            }
         }
      }
      
      return rval;
   };
   
   /**
    * Possible states for the client TLS engine to be in (also actions to take
    * to move to the next state).
    */
   var CH: 0;  // send client hello
   var SH: 1;  // rcv server hello
   var SC: 2;  // rcv server certificate
   var SK: 3;  // rcv server key exchange
   var SR: 4;  // rcv certificate request
   var SD: 5;  // rcv server hello done
   var CC: 6;  // send client certificate
   var CK: 7;  // send client key exchange
   var CV: 8;  // send certificate verify
   var CS: 9;  // send change cipher spec
   var CF: 10; // send finished
   var SS: 11; // rcv change cipher spec
   var SF: 12; // rcv finished
   var SM: 13; // rcv resume server messages
   var CM: 14; // send resume client messages
   var AD: 15; // send/rcv application data
   var SA: 16; // rcv alert
   var CA: 17; // send alert
   var HR: 18; // rcv HelloRequest
   var IG: 19; // ignore, keep current state
   var __: 20; // raise an error
   
   /**
    * The transistional state table for receiving TLS records. It maps
    * the current TLS engine state to the next state based on the action
    * taken. The states and actions that can be taken are in the same
    * namespace.
    * 
    * For instance, if the current state is CH, then the TLS engine needs
    * to send a ClientHello. Once that is done, to get the next state, this
    * table is queried using CH for the current state and CH for the action
    * taken. The result is SH which indicates that a ServerHello needs to be
    * received next. Once it is, then the current state SH and the action SH
    * are looked up in this table which indicate that SC (a server Certificate
    * message needs to be received) is the next state, and so forth.
    * 
    * There are three actions that are never states: SE, HR, and IG. The
    * client is never specifically waiting to receive a HelloRequest or an
    * alert from the server. These messages may occur at any time. The IG
    * action may occur when a HelloRequest message is received in the middle
    * of a handshake. In that case, the IG action is taken, which causes the
    * current state to remain unchanged an no action to be taken at all. This
    * also means it does not have an entry in the state table as an action.
    * 
    * There is no need to list the __ state as an action or an error in the
    * state table. If this state is ever detected, the TLS engine quits.
    * 
    * No state where the client is to send a message can raise an alert,
    * it can only raise an error. Alerts are only issued when a client has
    * received a message that has caused a problem that it must notify
    * the server about. (States beginning with a C should have no CE entries).
    * 
    * Current states are listed on the Y axis and actions taken on the X axis.
    * Next states/actions are the entries in the table.
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
   var _stateTable = [
   //     CH SH SC SK SR SD CC CK CV CS CF SS SF SM CM AD SA CA HR
   /*CH*/[SH,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,IG],
   /*SH*/[__,SC,CA,CA,CA,CA,__,__,__,__,__,CA,CA,CA,__,CA,__,__,IG],
   /*SC*/[__,__,SK,SR,SD,CC,__,__,__,__,__,SR,CA,CA,__,CA,__,__,IG],
   /*SK*/[__,__,__,SR,SD,CC,__,__,__,__,__,CA,CA,CA,__,CA,__,__,IG],
   /*SR*/[__,__,__,__,SD,CC,__,__,__,__,__,CA,CA,CA,__,CA,__,__,IG],
   /*SD*/[__,__,__,__,__,CC,__,__,__,__,__,CA,CA,CA,__,CA,__,__,IG],
   /*CC*/[__,__,__,__,__,__,CK,__,__,__,__,__,__,__,__,__,__,__,IG],
   /*CK*/[__,__,__,__,__,__,__,CV,__,__,__,__,__,__,__,__,__,__,IG],
   /*CV*/[__,__,__,__,__,__,__,__,CS,__,__,__,__,__,__,__,__,__,IG],
   /*CS*/[__,__,__,__,__,__,__,__,__,CF,__,__,__,__,__,__,__,__,IG],
   /*CF*/[__,__,__,__,__,__,__,__,__,__,SS,__,__,__,__,__,__,__,IG],
   /*SS*/[__,__,__,__,__,__,__,__,__,__,__,SF,CA,CA,__,CA,__,__,IG],
   /*SF*/[__,__,__,__,__,__,__,__,__,__,__,CA,AD,CA,__,CA,__,__,IG],
   /*SM*/[__,__,__,__,__,__,__,__,__,__,__,CA,CA,SR,__,CA,__,__,IG],
   /*CM*/[__,__,__,__,__,__,__,__,__,__,__,__,__,__,AD,__,__,__,IG],
   /*AD*/[__,__,__,__,__,__,__,__,__,__,__,CA,CA,CA,__,AD,__,__,CH],
   /*CA*/[__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,IG]
   ];
   
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
      return window.krypto.utils.createBuffer(b.getBytes(len));
   };
   
   /**
    * Writes a TLS variable-length vector to a byte buffer.
    * 
    * @param b the byte buffer.
    * @param lenBytes the number of bytes required to store the length.
    * @param bits the size of each element in bits.
    * @param v the byte buffer vector.
    */
   var writeVector = function(b, lenBytes, bits, v)
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
      },
      
      // handshake table
      var hsTable = [];
      
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
      hsTable[tls.HandshakeType.server_hello] = function(c, record)
      {
         // get the length of the message
         var b = record.fragment;
         var len = b.getInt24();
         
         // minimum of 38 bytes in message
         if(len < 38)
         {
            // FIXME: error
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
      hsTable[tls.HandshakeType.certificate] = function(c, record)
      {
         // get the length of the message
         var b = record.fragment;
         var len = b.getInt24();
         
         // minimum of 3 bytes in message
         if(len < 3)
         {
            // FIXME: error
         }
         else
         {
            var msg =
            {
               certificate_list: readVector(b, 3)
            };
            
            // FIXME: check server certificate
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
      hsTable[tls.HandshakeType.server_key_exchange] = function(c, record)
      {
         // get the length of the message
         var b = record.fragment;
         var len = b.getInt24();
         
         // this implementation only supports RSA, no Diffie-Hellman support
         // so any length > 0 is invalid
         if(len > 0)
         {
            // FIXME: error
         }
         else
         {
            // FIXME: nothing to do?
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
      hsTable[tls.HandshakeType.certificate_request] = function(c, record)
      {
         // get the length of the message
         var b = record.fragment;
         var len = b.getInt24();
         
         // minimum of 5 bytes in message
         if(len < 5)
         {
            // FIXME: error
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
         }
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
      hsTable[tls.HandshakeType.finished] = function(c, record)
      {
         // get the length of the message
         var b = record.fragment;
         var len = b.getInt24();
         
         // FIXME: check length against expected verify_data_length
         var vdl = 12;
         if(len == 12)
         {
            var msg =
            {
               verify_data: b.getBytes(len);
            };
            
            // FIXME: ensure verify data is correct
            // FIXME: create client change cipher spec record and
            // finished record
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
      hsTable[tls.HandshakeType.server_hello_done] = function(c, record)
      {
         // get the length of the message
         var b = record.fragment;
         var len = b.getInt24();
         
         // len must be 0 bytes
         if(len != 0)
         {
            // FIXME: error
         }
         else
         {
            // FIXME: create all of the client response records
            // client certificate
            // client key exchange
            // certificate verify
            // change cipher spec
            // finished
         }
      };
      
      /**
       * Called when the client receives a HelloRequest record.
       * 
       * @param c the connection.
       * @param record the record.
       */
      hsTable[tls.HandshakeType.hello_request] = function(c, record)
      {
         // FIXME: determine whether or not to ignore request based on state
      };
      
      // content type table
      var ctTable = [];
      
      /**
       * Called when the client receives a ChangeCipherSpec record.
       * 
       * @param c the connection.
       * @param record the record.
       */
      ctTable[tls.ContentType.change_cipher_spec] = function(c, record)
      {
         // FIXME: handle server has changed their cipher spec, change read
         // pending state to current, get ready for finished record
      };
      
      /**
       * Called when the client receives an Alert record.
       * 
       * @param c the connection.
       * @param record the record.
       */
      ctTable[tls.ContentType.alert] = function(c, record)
      {
         // FIXME: handle alert, determine if connection must end
      };
      
      /**
       * Called when the client receives a Handshake record.
       * 
       * @param c the connection.
       * @param record the record.
       */
      ctTable[tls.ContentType.handshake] = function(c, record)
      {
         // get the handshake type
         var type = record.fragment.getByte();
         var f = hsTable[type];
         if(f)
         {
            // handle specific handshake type record
            f(c, record);
         }
         else
         {
            // invalid handshake type
            // FIXME: handle
         }
      };
      
      /**
       * Called when the client receives an ApplicationData record.
       * 
       * @param c the connection.
       * @param record the record.
       */
      ctTable[tls.ContentType.application_data] = function(c, record)
      {
         // notify that data is ready
         c.dataReady(record.fragment.bytes());
      };
      
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
      createSecurityParameters: function()
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
         // FIXME: consider adding encrypt/decrypt functions (actual javascript
         // functions) to the params that can be called...
         // FIXME: looks like we only need 1 security params per pending/current
         // state, not 1 for each read/write state (MAYBE...)
         return securityParams;
      },
      
      /**
       * FIXME: Generates keys for the current state
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
       * FIXME: I would have thought you needed this for aes CBC as well...
       *
       * Implementation note: The currently defined cipher suite which
       * requires the most material is AES_256_CBC_SHA256. It requires 2 x 32
       * byte keys and 2 x 32 byte MAC keys, for a total 128 bytes of key
       * material.
       * 
       * FIXME:
       * 
       * @param sp the security parameters to use
       * 
       * @return the security keys and IVs.
       */
      generateKeys: function(sp)
      {
         // TLS_RSA_WITH_AES_128_CBC_SHA (this one is mandatory)
         
         // FIXME: change security parameters so they aren't stored
         // in the state, only the keys are and any encryption function
         // decryption functions necessary ... etc.
         
         // determine the PRF
         var prf;
         switch(sp.prf_algorithm)
         {
            case tls.PRFAlgorithm.tls_prf_sha256:
               prf = prf_sha256;
               break;
            default:
               // FIXME: raise some kind of record alert?
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
      },

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
       * @param keys the encryption key and the mac key for this state.
       * 
       * @return the new initialized TLS connection state.
       */
      createConnectionState: function(sp, keys)
      {
         var state =
         {
            compressionState: null,
            cipherState: null,
            macKey: null,
            sequenceNumber: 0,
            encrypt: null,
            decrypt: null,
            compress: null,
            decompress: null
         };
         
         // FIXME: determine how this should be done
         if(sp)
         {
            // FIXME: start encrypting/decrypting
            
            switch(sp.bulk_cipher_algorithm)
            {
               case tls.BulkCipherAlgorithm.aes:
                  cipherState =
                  {
                     cipher: null,
                     key: keys.encryptionKey;
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
            
            switch(sp.mac_algorithm)
            {
               case tls.MACAlgorithm.hmac_sha1:
                  state.macKey = keys.macKey;
                  break;
                default:
                   throw 'Unsupported MAC algorithm';
            }
            
            switch(sp.compression_algorithm)
            {
               case tls.CompressionMethod.none:
                  break;
               default:
                  throw 'Unsupported compression algorithm';
            }
            
            /*
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
         }
         
         return state;
      },
      
      /**
       * Creates an empty TLS record.
       * 
       * @param options:
       *    type: the record type
       *    data: the plain text data
       * 
       * @return the created record.
       */
      createRecord: function(options)
      {
         var record =
         {
            type: options.type,
            version:
            {
               // FIXME: TLS 1.2 = 3.3, TLS 1.0 = 3.1, send whichever ends
               // up being implemented
               major: 3,
               minor: 3
            },
            length: options.data.length,
            fragment: options.data
         };
         return record;
      },
      
      /**
       * Creates a TLS handshake message and stores it in a byte array.
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
       * 
       * @param type the type of handshake message.
       * @param body the message body.
       */
      createHandshakeMessage: function(type, body)
      {
         var data = [];
         data[0] = type & 0xFF;
         // encode length of body using 3 bytes, big endian
         var len = body.length & 0xFFFFFF;
         data[1] = (len >> 16) & 0xFF;
         data[2] = (len >> 8) & 0xFF;
         data[3] = len & 0xFF;
         data = data.concat(body);
         return data;
      },
      
      /**
       * Parses a HelloRequest body.
       * 
       * @param body the body to parse.
       */
      parseHelloRequest: function(body)
      {
         // FIXME:
      },
      
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
      createRandom: function()
      {
         // get UTC milliseconds
         var d = new Date();
         var utc = +d + d.getTimezoneOffset() * 60000;
         
         return
         {
            gmt_unix_time: utc,
            random_bytes: window.krypto.random.getRandomBytes(28)
         };
      },
      
      /**
       * Gets the cipher-suites supported by the client.
       * 
       * uint8 CipherSuite[2];
       * 
       * @return the cipher suites supported by the client.
       */
      getSupportedCipherSuites: function()
      {
         // FIXME: create array of 2-byte arrays
      },
      
      /**
       * Creates a ClientHello body.
       * 
       * opaque SessionID<0..32>;
       * enum { null(0), (255) } CompressionMethod;
       * 
       * struct {
       *    ProtocolVersion client_version;
       *    Random random;
       *    SessionID session_id;
       *    CipherSuite cipher_suites<2..2^16-2>;
       *    CompressionMethod compression_methods<1..2^8-1>;
       *    select (extensions_present) {
       *        case false:
       *            struct {};
       *        case true:
       *            Extension extensions<0..2^16-1>;
       *    };
       * } ClientHello;
       * 
       * @param sessionId the session ID to use.
       * 
       * @return the ClientHello body bytes.
       */
      createClientHello: function(sessionId)
      {
         var body =
         {
            // no extensions... see session ID cache for session ID
            version:
            {
               // FIXME: TLS 1.2 = 3.3, TLS 1.0 = 3.1, send whichever ends
               // up being implemented
               major: 3,
               minor: 3
            },
            // create client random bytes
            random: createRandom(),
            // use given session ID or blank
            session_id: sessionId || [],
            // use all supported cipher suites
            cipher_suites: getSupportedCipherSuites(),
            // no compression
            compression_methods: [0]
            // no extensions
            extensions: []
         };
         
         // convert body into TLS-encoded bytes
         var array = tls.createArray();
         array.pushInt(body.version.major, 8);
         array.pushInt(body.version.minor, 8);
         array.pushInt(body.random.gmt_unix_time, 32);
         array.pushBytes(body.random.random_bytes);
         array.pushVector(body.session_id, 32, array.pushByte);
         array.pushVector(body.cipher_suites, 2, array.pushBytes);
         array.pushVector(body.compression_methods, 1, array.pushByte);
         return array.bytes;
      },
      
      /**
       * Parses a change_cipher_spec record.
       * 
       * @param c the current connection.
       * @param record the record to update.
       * @param b the byte buffer with the record.
       * 
       * @return true on success, false on failure.
       */
      parseRecordChangeCipherSpec: function(c, record, b)
      {
      },
      
      /**
       * Parses an alert record.
       * 
       * @param c the current connection.
       * @param record the record to update.
       * @param b the byte buffer with the record.
       * 
       * @return true on success, false on failure.
       */
      parseRecordAlert: function(c, record, b)
      {
      },
      
      /**
       * Parses a handshake record.
       * 
       * @param c the current connection.
       * @param record the record to update.
       * @param b the byte buffer with the record.
       * 
       * @return true on success, false on failure.
       */
      parseRecordHandshake: function(c, record, b)
      {
      },
      
      /**
       * Parses an application data record.
       * 
       * @param c the current connection.
       * @param record the record to update.
       * @param b the byte buffer with the record.
       * 
       * @return true on success, false on failure.
       */
      parseRecordApplicationData: function(c, record, b)
      {
      },
      
      /**
       * Parses an incoming record.
       * 
       * @param c the current connection.
       * @param b the byte buffer with the record.
       * 
       * @return the parsed record, null on failure.
       */
      parseRecord: function(c, b)
      {
         // parse basic record
         var record =
         {
            type: b.getByte(),
            version:
            {
               major: b.getByte(),
               minor: b.getByte()
            },
            length: b.getInt16(),
            fragment: null
         };
         
         // FIXME: use current state to decrypt/decompress fragment
         var out = window.krypto.utils.createBuffer();
         c.state
         
         // FIXME: parse plain text fragment
         var success = false;
         switch(record.type)
         {
            case tls.ContentType.change_cipher_spec:
               success = tls.parseRecordChangeCipherSpec(b);
               break;
            case tls.ContentType.alert:
               success = tls.parseRecordAlert(b);
               break;
            case tls.ContentType.handshake:
               success = tls.parseRecordHandshake(b);
               break;
            case tls.ContentType.application_data:
               success = tls.parseRecordApplicationData(b);
               break;
            default:
               // FIXME: handle error case
               break;
         }
         
         return success ? record : null;
      },
      
      /**
       * Creates a new TLS connection.
       * 
       * See public createConnection() docs for more details.
       * 
       * @param options the options for this connection.
       * 
       * @return the new TLS connection.
       */
      createConnection: function(options)
      {
         // private TLS engine state for a TLS connection
         var _state = CH;
         
         /**
          * Gets the action to take based on the given record.
          * 
          * @param record the record.
          * @param client true if the record is from client, false server.
          * 
          * @return the action to take.
          */
         var _getAction = function(record, client)
         {
            var action;
            switch(record.type)
            {
               case tls.ContentType.change_cipher_spec:
                  action = client ? CS : SS;
                  break;
               case tls.ContentType.alert:
                  action = client ? CA : SA;
                  break;
               case tls.ContentType.handshake:
               {
                  var type = record.fragment.at(0);
                  switch(type)
                  {
                     case tls.HandshakeType.hello_request:
                        action = client ? __ : HR;
                        break;
                     case tls.HandshakeType.client_hello:
                        action = client ? CH : __;
                        break;
                     case tls.HandshakeType.server_hello:
                        action = client ? __ : SH;
                        break;
                     case tls.HandshakeType.certificate:
                        action = client ? CC : SC;
                        break;
                     case tls.HandshakeType.server_key_exchange:
                        action = client ? __ : SK;
                        break;
                     case tls.HandshakeType.certificate_request:
                        action = client ? CR : SR;
                        break;
                     case tls.HandshakeType.server_hello_done:
                        action = client ? CD : SD;
                        break;
                     case tls.HandshakeType.certificate_verify:
                        action = client ? CV : SV;
                        break;
                     case tls.HandshakeType.client_key_exchange:
                        action = client ? CK : __;
                        break;
                     case tls.HandshakeType.finished:
                        action = client ? CF : SF;
                        break;
                     default:
                        action = __;
                        break;
                  }
                  break;
               }
               case tls.ContentType.application_data:
                  action = AD;
                  break;
               default:
                  action = __;
                  break;
            }
            return action;
         };
         
         /**
          * Handles a client record.
          * 
          * @param action the action to perform.
          */
         var _handleClientRecord = function(action)
         {
            // compress and encrypt the record
            if(c.current.compress(record, c.state.current) &&
               c.current.encrypt(record, c.state.current))
            {
               switch(action)
               {
                  // client hello
                  case CH:
                     // FIXME: make appropriate state changes... start
                     // new pending state?
                     break;
                  // client certificate
                  case CC:
                     break;
                  // client key exchange
                  case CK:
                     break;
                  // client certificate verify
                  case CV:
                     break;
                  // client change cipher spec
                  case CS:
                     // FIXME: switch pending state to current state?
                     break;
                  // client finished
                  case CF:
                     break;
                  // client resume messages
                  case CM:
                     break;
                  // application data
                  case AD:
                     break;
                  // client alert
                  case CA:
                     break;
               }
               
               // store record
               c.records.push(record);
            }
            // fatal error
            else
            {
               // FIXME: better errors
               c.error(c, 'Could not prepare record.');
            }            
         };
         
         /**
          * Handles a server record.
          * 
          * @param action the action to perform.
          */
         var _handleServerRecord = function(action)
         {
            switch(action)
            {
               // server hello
               case SH:
                  break;
               // server certificate
               case SC:
                  // FIXME: do cert things
                  break;
               // server key exchange
               case SK:
                  break;
               // server certificate request
               case SR:
                  // FIXME: take note server wants a client cert
                  break;
               // server hello done
               case SD:
                  // FIXME: determine what records to send to server
                  break;
               // server change cipher spec
               case SS:
                  break;
               // server finished
               case SF:
                  break;
               // server resume messages
               case SR:
                  break;
               // application data
               case AD:
                  // FIXME: do app data ready or whatever
                  break;
               // server alert
               case SA:
                  break;
               // server HelloRequest
               case HR:
                  break;
            }            
         };
         
         /**
          * Updates the current TLS engine state based on the given record.
          * 
          * @param c the TLS connection.
          * @param record the TLS record to act on.
          * @param client true if the record is from client, false server.
          */
         var _update = function(c, record, client)
         {
            // get the next state based on the action for the record
            var action = _getAction(record, client);
            var next = _stateTable[_state][action];
            
            if(next === __)
            {
               // error case
               c.error(c, 'Invalid TLS state.');
            }
            // only update state if not ignoring action
            else if(next !== IG)
            {
               // do action
               if(_doAction(action))
               {
                  // handle output
                  if((next === CF || next === AD) && c.records.length > 0)
                  {
                     // FIXME: write pending records into bytes
                     // c.output...
                     c.tlsDataReady(c, data);
                  }
                  
                  // update state
                  _state = next;
               }
            }
         };
         
         // pending input record
         var _record = null;
         
         // create TLS connection
         var connection =
         {
            sessionId: options.sessionId,
            state:
            {
               pending:
               {
                  read: tls.createConnectionState(),
                  write: tls.createConnectionState(),
               },
               current:
               {
                  read: null,
                  write: null
               }
            },
            records: [],
            input: window.krypto.utils.createBuffer(),
            output: window.krypto.utils.createBuffer(),
            tlsDataReady: options.tlsDataReady,
            dataReady: options.dataReady,
            closed: options.closed,
            error: options.error,
            
            /**
             * Performs a handshake using the TLS Handshake Protocol.
             * 
             * @param sessionId the session ID to use, null to start a new one.
             */
            handshake: function(sessionId)
            {
               // FIXME: after sending a client hello message, any message
               // other than a server hello (or ignored hello request) is
               // considered a fatal error
               var record = tls.createRecord(
               {
                  type: tls.ContentType.handshake,
                  data: createClientHello(sessionId)
               };
               _update(connection, record, true);
            },
            
            /**
             * Called when a TLS protocol data has been received from somewhere
             * and should be processed by the TLS engine.
             * 
             * @param data the TLS protocol data, as a string, to process.
             */
            process: function(data)
            {
               // buffer data, get input length
               var b = connection.input;
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
                     fragment: null
                  };
                  len -= 5;
               }
               
               // see if there is enough data to parse the pending record
               if(_record !== null && len >= _record.length)
               {
                  tls.parseRecord(connection, record);
                  _update(connection, record, false);
               }
            },
            
            /**
             * Requests that application data be packaged into a TLS record.
             * The recordReady handler will be called when a TLS record has
             * been prepared.
             * 
             * @param data the application data, as a string, to be sent.
             */
            prepare: function(data)
            {
               var record = tls.createRecord(
               {
                  type: tls.ContentType.application_data,
                  data: data
               };
               _update(connection, record, true);
            }
         };
         return connection;
      }
   };
   
   /**
    * The crypto namespace and tls API.
    */
   window.krypto = window.krypto || {};
   window.krypto.tls = 
   {
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
       *    tlsDataReady: function(conn, data) called when TLS protocol data
       *       has been prepared and is ready to be used (typically sent over a
       *       socket connection to its destination).
       *    dataReady: function(conn, data) called when application data has
       *       been parsed from a TLS record and should be consumed by the
       *       application.
       *    closed: function(conn) called when the connection has been closed.
       *    error: function(conn) called when there was an error.
       * 
       * @return the new TLS connection.
       */
      createConnection: function(options)
      {
         return tls.createConnection(options);
      }
   };
   
   /**
    * A TlsArray contains contents that have been encoded according to
    * the TLS specification. Its raw bytes can be accessed via the bytes
    * property.
    */
//   createArray: function()
//   {
//      var array =
//      {
//         bytes: [],
//         
//         /**
//          * Pushes a byte.
//          * 
//          * @param b the byte to push.
//          */
//         pushByte: function(b)
//         {
//            bytes.push(b & 0xFF);
//         },
//         
//         /**
//          * Pushes bytes.
//          * 
//          * @param b the bytes to push.
//          */
//         pushBytes: function(b)
//         {
//            bytes = bytes.concat(b);
//         },
//         
//         /**
//          * Pushes an integer value.
//          * 
//          * @param i the integer value.
//          * @param bits the size of the integer in bits.
//          */
//         pushInt: function(i, bits)
//         {
//            do
//            {
//               bits -= 8;
//               bytes.push((i >> bits) & 0xFF);
//            }
//            while(bits > 0);
//         }
//      
//         /**
//          * Pushes a vector value.
//          * 
//          * @param v the vector.
//          * @param sizeBytes the number of bytes for the size.
//          * @param func the function to call to encode each element.
//          */
//         pushVector: function(v, sizeBytes, func)
//         {
//            /* Variable-length vectors are defined by specifying a subrange
//               of legal lengths, inclusively, using the notation
//               <floor..ceiling>. When these are encoded, the actual length
//               precedes the vector's contents in the byte stream. The length
//               will be in the form of a number consuming as many bytes as
//               required to hold the vector's specified maximum (ceiling)
//               length. A variable-length vector with an actual length field
//               of zero is referred to as an empty vector. */
//            
//            // encode length at the start of the vector, where the number
//            // of bytes for the length is the maximum number of bytes it
//            // would take to encode the vector's ceiling (= sizeBytes)
//            array.pushInt(v.length, sizeBytes * 8);
//            
//            // encode the vector
//            for(var i = 0; i < v.length; i++)
//            {
//               func(v[i]);
//            }
//         }
//      };
//      return array;
//   }
})();
