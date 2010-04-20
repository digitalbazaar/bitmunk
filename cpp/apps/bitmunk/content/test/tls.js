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
 */
(function($)
{
   /**
    * Private Pseudo Random Functions (PRF).
    */
   
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
    * Private encryption functions.
    */
   // FIXME: put encrypt/decrypt functions here, point to them using states
   /**
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
    * @param record the TLSCompressed record to encrypt.
    * @param state the ConnectionState to use.
    * 
    * @return the TLSCipherText record.
    */
   var encrypt_aes_128_cbc_sha1 = function(record, state)
   {
      // FIXME: implement me
      
      // FIXME: padding byte is the padding length
      // ie padding data: 0x030303
      // padding length: 3
      
      /* The encrypted data length (TLSCiphertext.length) is one more than
         the sum of SecurityParameters.block_length, TLSCompressed.length,
         SecurityParameters.mac_length, and padding_length. */
      
      var genericBlockCipher =
      {
         IV: [],
         block:
         {
            content: [],
            MAC: [],
            padding: [],
            padding_length: 0
         }
      };
   };
   
   /**
    * Decrypts a TLSCipherText record into a TLSCompressed record.
    * 
    * @param record the TLSCipherText record to decrypt.
    * @param state the ConnectionState to use.
    * 
    * @return the TLSCompressed record.
    */
   var decrypt_aes_128_cbc_sha1 = function(record, state)
   {
      // FIXME: implement me
   };
   
   /**
    * Private MAC functions.
    */
   // FIXME: put MAC functions here, point to them using states
   var hmac_sha1 = function()
   {
      // FIXME:
   };
   
   /**
    * Possible actions and states for the client TLS engine to be in.
    * FIXME: mix actions and states together? We'll need to map received
    * record types (ints) anyway ... since 20 and 20 mean different things
    * finished vs. change cipher spec... etc.
    * 
    * An underscore following the state/action abbreviation indicates that the
    * client is sending/has sent a message. An underscore afterwards indicates
    * the client is to receive/has received a message.
    */
   var CH_: 0;  // send client hello
   var _SH: 1;  // rcv server hello
   var _SC: 2;  // rcv server certificate
   var _SK: 3;  // rcv server key exchange
   var _CR: 4;  // rcv certificate request
   var _HD: 5;  // rcv server hello done
   var CC_: 6;  // send client certificate
   var CK_: 7;  // send client key exchange
   var CV_: 8;  // send certificate verify
   var CS_: 9;  // send change cipher spec
   var FN_: 10; // send finished
   var _CS: 11; // rcv change cipher spec
   var _FN: 12; // rcv finished
   var _RS: 13; // rcv resume server messages
   var RC_: 14; // send resume client messages
   var _A_: 15; // send/rcv application data
   var _AT: 16; // rcv alert
   var AT_: 17; // send alert
   var _HR: 18; // rcv HelloRequest
   var IGR: 19; // ignore, keep current state
   var ERR: 20; // raise an error
   
   /**
    * The transistional state table for receiving TLS records. It maps
    * the current TLS engine state to the next state based on the action
    * taken. The states and actions that can be taken are in the same
    * namespace.
    * 
    * For instance, if the current state is CH_, then the TLS engine needs
    * to send a ClientHello. Once that is done, to get the next state, this
    * table is queried using CH_ for the current state and CH_ for the action
    * taken. The result is _SH which indicates that a ServerHello needs to be
    * received next. Once it is, then the current state _SH and the action _SH
    * are looked up in this table which indicate that _SC (a server Certificate
    * message needs to be received) is the next state, and so forth.
    * 
    * There are three actions that are never states: _AT, _HR, and IGR. The
    * client is never specifically waiting to receive a HelloRequest or an
    * alert from the server. These messages may occur at any time. The IGR
    * action may occur when a HelloRequest message is received in the middle
    * of a handshake. In that case, the IGR action is taken, which causes the
    * current state to remain unchanged an no action to be taken at all. This
    * also means it does not have an entry in the state table as an action.
    * 
    * There is no need to list the ERR state as an action or an error in the
    * state table. If this state is ever detected, the TLS engine quits.
    * 
    * No state where the client is to send a message can raise an alert,
    * it can only raise an error. Alerts are only issued when a client has
    * received a message that has caused a problem that it must notify
    * the server about. (States ending with an underscore should have no
    * AT_ entries).
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
   var gStateTable = [
   /*      CH_ _SH _SC _SK _CR _HD CC_ CK_ CV_ CS_ FN_ _CS _FN _RS RC_ _A_ _AT AT_ _HR*/
   /*CH_*/[_SH,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   /*_SH*/[ERR,_SC,AT_,AT_,AT_,AT_,ERR,ERR,ERR,ERR,ERR,AT_,AT_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*_SC*/[ERR,ERR,_SK,_CR,_HD,CC_,ERR,ERR,ERR,ERR,ERR,_RS,AT_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*_SK*/[ERR,ERR,ERR,_CR,_HD,CC_,ERR,ERR,ERR,ERR,ERR,AT_,AT_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*_CR*/[ERR,ERR,ERR,ERR,_HD,CC_,ERR,ERR,ERR,ERR,ERR,AT_,AT_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*_HD*/[ERR,ERR,ERR,ERR,ERR,CC_,ERR,ERR,ERR,ERR,ERR,AT_,AT_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*CC_*/[ERR,ERR,ERR,ERR,ERR,ERR,CK_,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   /*CK_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,CV_,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   /*CV_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,CS_,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   /*CS_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,FN_,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   /*FN_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,_CS,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   /*_CS*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,_FN,AT_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*_FN*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,AT_,_A_,AT_,ERR,AT_,ERR,ERR,IGR],
   /*_RS*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,AT_,AT_,RC_,ERR,AT_,ERR,ERR,IGR],
   /*RC_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,_A_,ERR,ERR,ERR,IGR],
   /*_A_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,AT_,AT_,AT_,ERR,_A_,ERR,ERR,CH_],
   /*AT_*/[ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,IGR],
   ];

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
      
      /**
       * A TlsArray contains contents that have been encoded according to
       * the TLS specification. Its raw bytes can be accessed via the bytes
       * property.
       */
      createArray: function()
      {
         var array =
         {
            bytes: [],
            
            /**
             * Pushes a byte.
             * 
             * @param b the byte to push.
             */
            pushByte: function(b)
            {
               bytes.push(b & 0xFF);
            },
            
            /**
             * Pushes bytes.
             * 
             * @param b the bytes to push.
             */
            pushBytes: function(b)
            {
               bytes = bytes.concat(b);
            },
            
            /**
             * Pushes an integer value.
             * 
             * @param i the integer value.
             * @param bits the size of the integer in bits.
             */
            pushInt: function(i, bits)
            {
               do
               {
                  bits -= 8;
                  bytes.push((i >> bits) & 0xFF);
               }
               while(bits > 0);
            }
         
            /**
             * Pushes a vector value.
             * 
             * @param v the vector.
             * @param sizeBytes the number of bytes for the size.
             * @param func the function to call to encode each element.
             */
            pushVector: function(v, sizeBytes, func)
            {
               /* Variable-length vectors are defined by specifying a subrange
                  of legal lengths, inclusively, using the notation
                  <floor..ceiling>. When these are encoded, the actual length
                  precedes the vector's contents in the byte stream. The length
                  will be in the form of a number consuming as many bytes as
                  required to hold the vector's specified maximum (ceiling)
                  length. A variable-length vector with an actual length field
                  of zero is referred to as an empty vector. */
               
               // encode length at the start of the vector, where the number
               // of bytes for the length is the maximum number of bytes it
               // would take to encode the vector's ceiling (= sizeBytes)
               var length = v.length;
               var bits = sizeBytes * 8;
               do
               {
                  bits -= 8;
                  bytes.push((length >> bits) & 0xFF);
               }
               while(bits > 0);
               
               // encode the vector
               for(var i = 0; i < length; i++)
               {
                  func(v[i]);
               }
            }
         };
         return array;
      },
      
      // FIXME: add state flag to current connection state that is used
      // to do look ups for what to do next in the transistional state
      // table above
      
      // FIXME: move this function to an appropriate location ... call from
      // send/received record/whatever else
      /**
       * Updates the given state based on the given action taken.
       * 
       * @param state the state to update.
       * @param action the action that was taken.
       */
      // FIXME: make private
      updateState: function(state, action)
      {
         // get the next state from the table
         var next = gStateTable[state.current][action];
         
         // only process next state if not ignoring
         if(next != IGR)
         {
            // update current state
            state.current = next;
         }
      },
      
      /**
       * Performs the next action based on the current state.
       * 
       * @param state the state.
       */
      // FIXME: make private
      doAction: function(state)
      {
         // FIXME: need stacks/queues for popping off previous errors and
         // TLS records
         
         switch(state.current)
         {
            // send client hello
            case CH_:
               break;
            // rcv server hello
            case _SH:
               break;
            // rcv server certificate
            case _SC:
               break;
            // rcv server key exchange
            case _SK:
               break;
            // rcv certificate request
            case _CR:
               break;
            // rcv server hello done
            case _HD:
               break;
            // send client certificate
            case CC_:
               break;
            // send client key exchange
            case CK_:
               break;
            // send certificate verify
            case CV_:
               break;
            // send change cipher spec
            case CS_:
               break;
            // send finished
            case FN_:
               break;
            // rcv change cipher spec
            case _CS:
               break;
            // rcv finished
            case _FN:
               break;
            // rcv resume server messages
            case _RS:
               break;
            // send resume client messages
            case RC_:
               break;
            // send/rcv application data
            case _A_:
               break;
            // rcv alert
            case _AT:
               break;
            // send alert
            case AT_:
               break;
            // rcv HelloRequest
            case _HR:
               break;
            // ignore, keep current state
            case IGR:
               break;
            // raise an error
            case ERR:
               break;
         }
      },
      
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
            switch(sp.bulk_cipher_algorithm)
            {
               case tls.BulkCipherAlgorithm.aes:
                  cipherState =
                  {
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
       * // FIXME: write a comment about making a call to compress the record
       * // before sending it out
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
       *    select (HandshakeType) {
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
       * Parses a ServerHello body.
       * 
       * struct {
       *    ProtocolVersion server_version;
       *    Random random;
       *    SessionID session_id;
       *    CipherSuite cipher_suite;
       *    CompressionMethod compression_method;
       *    select (extensions_present) {
       *        case false:
       *            struct {};
       *        case true:
       *            Extension extensions<0..2^16-1>;
       *    };
       * } ServerHello;
       * 
       * @param bytes the ServerHello bytes.
       * 
       * @return the ServerHello body.
       */
      parseServerHello: function(bytes)
      {
         // FIXME: might not even need this method ... just something
         // to handle the byte array
      }
      
      // FIXME: next up, process server certificate message
   };
   
   /**
    * The crypto namespace and tls API.
    */
   window.krypto = window.krypto || {};
   window.krypto.tls = 
   {
      /**
       * Creates a new TLS connection. This does not make any assumptions about
       * the transport layer that TLS is working on top of, ie it does not
       * assume there is a TCP/IP connection or establish one. A TLS connection
       * is totally abstracted away from the layer is runs on top of, it merely
       * establishes a secure channel between two "peers".
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
       *    recordReady: function(conn, record) called when a TLS record has
       *       been prepared and is ready to be used (typically sent over a
       *       socket connection to its destination).
       *    dataReady: function(conn, data) called when application data has
       *       been parsed from a TLS record and should be consumed by the
       *       application.
       *    closed: function(conn) called when the connection has been closed.
       * 
       * @return the new TLS connection.
       */
      createConnection: function(options)
      {
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
            recordReady: options.recordReady,
            dataReady: options.dataReady,
            closed: options.closed,
            
            /**
             * Performs a handshake using the TLS Handshake Protocol.
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
             * The recordReady handler will be called when a TLS record has
             * been prepared.
             * 
             * @param sessionId the session ID to use, null to start a new one.
             */
            handshake: function(sessionId)
            {
               // create ClientHello message
               recordReady(tls.createRecord(
               {
                  type: tls.ContentType.handshake,
                  data: createClientHello(sessionId)
               }));
            },
            
            /**
             * Called when a TLS record has been received from somewhere and
             * should be processed by the TLS engine.
             * 
             * @param record the TLS record to process.
             */
            process: function(record)
            {
               // FIXME:
            },
            
            /**
             * Requests that application data be packaged into a TLS record.
             * The recordReady handler will be called when a TLS record has
             * been prepared.
             * 
             * @param data the application data to be sent.
             */
            prepare: function(data)
            {
               // FIXME:
            }
         };
         return connection;
      }
   };
})(jQuery);
