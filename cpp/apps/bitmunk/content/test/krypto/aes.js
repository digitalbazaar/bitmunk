/**
 * Advanced Encryption Standard (AES) Cipher-Block Chaining implementation.
 * 
 * This implementation is based on the public domain library 'jscrypto' which
 * was written by:
 * 
 * Emily Stark (estark@stanford.edu)
 * Mike Hamburg (mhamburg@stanford.edu)
 * Dan Boneh (dabo@cs.stanford.edu)
 * 
 * Parts of this code are based on the OpenSSL implementation of AES:
 * http://www.openssl.org
 * 
 * @author Dave Longley
 *
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
(function($)
{
   var init = false; // not yet initialized
   var Nb = 4;       // number of words comprising the state (AES = 4)
   var sbox;         // non-linear substitution table used in key expansion
   var isbox;        // inversion of sbox
   var rcon;         // round constant word array
   var mix;          // mix-columns table
   var imix;         // inverse mix-columns table
   
   /**
    * Performs initialization, ie: precomputes tables to optimize for speed.
    * 
    * One way to understand how AES works is to imagine that 'addition' and
    * 'multiplication' are interfaces that require certain mathematical
    * properties to hold true (ie: they are associative) but they might have
    * different implementations and produce different kinds of results ...
    * provided that their mathematical properties remain true. AES defines
    * its own methods of addition and multiplication but keeps some important
    * properties the same, ie: associativity and distributivity. The
    * explanation below tries to shed some light on how AES defines addition
    * and multiplication of bytes and 32-bit words in order to perform its
    * encryption and decryption algorithms.
    * 
    * The basics:
    * 
    * The AES algorithm views bytes as binary representations of polynomials
    * that have either 1 or 0 as the coefficients. It defines the addition
    * or subtraction of two bytes as the XOR operation. It also defines the
    * multiplication of two bytes as a finite field referred to as GF(2^8)
    * (Note: 'GF' means "Galois Field" which is a field that contains a finite
    * number of elements so GF(2^8) has 256 elements).
    * 
    * This means that any two bytes can be represented as binary polynomials;
    * when they multiplied together and modularly reduced by an irreducible
    * polynomial of the 8th degree, the results are the field GF(2^8). The
    * specific irreducible polynomial that AES uses in hexadecimal is 0x11b.
    * This multiplication is associative with 0x01 as the identity:
    * 
    * (b * 0x01 = GF(b, 0x01) = b).
    * 
    * The operation GF(b, 0x02) can be performed at the byte level by left
    * shifting b once and then XOR'ing it (to perform the modular reduction)
    * with 0x11b if b is >= 128. Repeated application of the multiplication
    * of 0x02 can be used to implement the multiplication of any two bytes.
    * 
    * For instance, multiplying 0x57 and 0x13, denoted as GF(0x57, 0x13), can
    * be performed by factoring 0x13 into 0x01, 0x02, and 0x10. Then these
    * factors can each be multiplied by 0x57 and then added together. To do
    * the multiplication, values for 0x57 multiplied by each of these 3 factors
    * can be precomputed and stored in a table. To add them, the values from
    * the table are XOR'd together.
    * 
    * AES also defines addition and multiplication of words, that is 4-byte
    * numbers represented as polynomials of 3 degrees where the coefficients
    * are the values of the bytes.
    * 
    * The word [a0, a1, a2, a3] is a polynomial a3x^3 + a2x^2 + a1x + a0.
    * 
    * Addition is performed by XOR'ing like powers of x. Multiplication
    * is performed in two steps, the first is an algebriac expansion as
    * you would do normally (where addition is XOR). But the result is
    * a polynomial larger than 3 degrees and thus it cannot fit in a word. So
    * next the result is modularly reduced by an AES-specific polynomial of
    * degree 4 which will always produce a polynomial of less than 4 degrees
    * such that it will fit in a word. In AES, this polynomial is x^4 + 1.
    * 
    * The modular product of two polynomials 'a' and 'b' is thus:
    * 
    * d(x) = d3x^3 + d2x^2 + d1x + d0
    * with
    * d0 = GF(a0, b0) ^ GF(a3, b1) ^ GF(a2, b2) ^ GF(a1, b3)
    * d1 = GF(a1, b0) ^ GF(a0, b1) ^ GF(a3, b2) ^ GF(a2, b3)
    * d2 = GF(a2, b0) ^ GF(a1, b1) ^ GF(a0, b2) ^ GF(a3, b3)
    * d3 = GF(a3, b0) ^ GF(a2, b1) ^ GF(a1, b2) ^ GF(a0, b3)
    * 
    * As a matrix:
    * 
    * [d0] = [a0 a3 a2 a1][b0]
    * [d1]   [a1 a0 a3 a2][b1]
    * [d2]   [a2 a1 a0 a3][b2]
    * [d3]   [a3 a2 a1 a0][b3]
    * 
    * Special polynomials defined by AES (0x02 == {02}):
    * a(x)    = {03}x^3 + {01}x^2 + {01}x + {02}
    * a^-1(x) = {0b}x^3 + {0d}x^2 + {09}x + {0e}.
    * 
    * These polynomials are used in the MixColumns() and InverseMixColumns()
    * operations, respectively, to cause each element in the state to affect
    * the output (referred to as diffusing).
    * 
    * RotWord() uses: a0 = a1 = a2 = {00} and a3 = {01}, which is the
    * polynomial x3.
    * 
    * The ShiftRows() method modifies the last 3 rows in the state (where 
    * the state is 4 words with 4 bytes per word) by shifting bytes cyclically.
    * The 1st byte in the second row is moved to the end of the row. The 1st
    * and 2nd bytes in the third row are moved to the end of the row. The 1st,
    * 2nd, and 3rd bytes are moved in the fourth row.
    * 
    * More details on how AES arithmetic works:
    * 
    * In the polynomial representation of binary numbers, XOR performs addition
    * and subtraction and multiplication in GF(2^8) denoted as GF(a, b)
    * corresponds with the multiplication of polynomials modulo an irreducible
    * polynomial of degree 8. In other words, for AES, GF(a, b) will multiply
    * polynomial 'a' with polynomial 'b' and then do a modular reduction by
    * an AES-specific irreducible polynomial of degree 8.
    * 
    * A polynomial is irreducible if its only divisors are one and itself. For
    * the AES algorithm, this irreducible polynomial is:
    * 
    * m(x) = x^8 + x^4 + x^3 + x + 1,
    * 
    * or {01}{1b} in hexadecimal notation, where each coefficient is a bit:
    * 100011011 = 283 = 0x11b.
    * 
    * For example, GF(0x57, 0x83) = 0xc1 because
    * 
    * 0x57 = 87  = 01010111 = x^6 + x^4 + x^2 + x + 1
    * 0x85 = 131 = 10000101 = x^7 + x + 1
    * 
    * (x^6 + x^4 + x^2 + x + 1) * (x^7 + x + 1)
    * =  x^13 + x^11 + x^9 + x^8 + x^7 +
    *    x^7 + x^5 + x^3 + x^2 + x +
    *    x^6 + x^4 + x^2 + x + 1
    * =  x^13 + x^11 + x^9 + x^8 + x^6 + x^5 + x^4 + x^3 + 1 = y
    *    y modulo (x^8 + x^4 + x^3 + x + 1)
    * =  x^7 + x^6 + 1.
    * 
    * The modular reduction by m(x) guarantees the result will be a binary
    * polynomial of less than degree 8, so that it can fit in a byte.
    * 
    * The operation to multiply a binary polynomial b with x (the polynomial
    * x in binary representation is 00000010) is:
    * 
    * b_7x^8 + b_6x^7 + b_5x^6 + b_4x^5 + b_3x^4 + b_2x^3 + b_1x^2 + b_0x^1
    * 
    * To get GF(b, x) we must reduce that by m(x). If b_7 is 0 (that is the
    * most significant bit is 0 in b) then the result is already reduced. If
    * it is 1, then we can reduce it by subtracting m(x) via an XOR.
    * 
    * It follows that multiplication by x (00000010 or 0x02) can be implemented
    * by performing a left shift followed by a conditional bitwise XOR with
    * 0x1b. This operation on bytes is denoted by xtime(). Multiplication by
    * higher powers of x can be implemented by repeated application of xtime(). 
    * 
    * By adding intermediate results, multiplication by any constant can be
    * implemented. For instance:
    * 
    * GF(0x57, 0x13) = 0xfe because:
    * 
    * xtime(b) = (b & 128) ? (b << 1 ^ 0x11b) : (b << 1)
    * 
    * Note: We XOR with 0x11b instead of 0x1b because in javascript our
    * datatype for b can be larger than 1 byte, so a left shift will not
    * automatically eliminate bits that overflow a byte ... by XOR'ing the
    * overflow bit with 1 (the extra one from 0x11b) we zero it out. 
    * 
    * GF(0x57, 0x02) = xtime(0x57) = 0xae
    * GF(0x57, 0x04) = xtime(0xae) = 0x47
    * GF(0x57, 0x08) = xtime(0x47) = 0x8e
    * GF(0x57, 0x10) = xtime(0x8e) = 0x07
    * 
    * GF(0x57, 0x13) = GF(0x57, (0x01 ^ 0x02 ^ 0x10))
    * 
    * And by the distributive property (since XOR is addition and GF() is
    * multiplication):
    * 
    * = GF(0x57, 0x01) ^ GF(0x57, 0x02) ^ GF(0x57, 0x10)
    * = 0x57 ^ 0xae ^ 0x07
    * = 0xfe.
    */
   var initialize = function()
   {
      init = true;
      
      /* Populate the Rcon table. These are the values given by
         [x^(i-1),{00},{00},{00}] where x^(i-1) are powers of x (and x = 0x02)
         in the field of GF(2^8), where i starts at 1.
         
         rcon[0] = [0x00, 0x00, 0x00, 0x00]
         rcon[1] = [0x01, 0x00, 0x00, 0x00] 2^(1-1) = 2^0 = 1
         rcon[2] = [0x02, 0x00, 0x00, 0x00] 2^(2-1) = 2^1 = 2
         ...
         rcon[9]  = [0x1B, 0x00, 0x00, 0x00] 2^(9-1)  = 2^8 = 0x1B
         rcon[10] = [0x36, 0x00, 0x00, 0x00] 2^(10-1) = 2^9 = 0x36
         
         We only store the first byte because it is the only one used.
       */
      rcon = [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36];
      
      // compute xtime table which maps i onto GF(i, 0x02)
      var xtime = new Array(256);
      for(var i = 0; i < 128; i++)
      {
         xtime[i] = i << 1;
         xtime[i + 128] = (i + 128) << 1 ^ 0x11B;
      }
      
      // compute all other tables
      sbox = new Array(256);
      isbox = new Array(256);
      mix = new Array(4);
      imix = new Array(4);
      for(var i = 0; i < 4; i++)
      {
         mix[i] = new Array(256);
         imix[i] = new Array(256);
      }
      var e = 0, ei = 0, e2, e4, e8, sx, sx2, me, ime;
      for(var i = 0; i < 256; i++)
      {
         /* We need to generate the SubBytes() sbox and isbox tables so that
            we can perform byte substitutions. This requires us to traverse
            all of the elements in GF, find their multiplicative inverses,
            and apply to each the following affine transformation:
            
            bi' = bi ^ b(i + 4) mod 8 ^ b(i + 5) mod 8 ^ b(i + 6) mod 8 ^
                  b(i + 7) mod 8 ^ ci
            for 0 <= i < 8, where bi is the ith bit of the byte, and ci is the
            ith bit of a byte c with the value {63} or {01100011}.
            
            It is possible to traverse every possible value in a Galois field
            using what is referred to as a 'generator'. There are many
            generators (128 out of 256): 3,5,6,9,11,82 to name a few. To fully
            traverse GF we iterate 255 times, multiplying by our generator
            each time.
            
            On each iteration we can determine the multiplicative inverse for
            the current element.
            
            Suppose there is an element in GF 'e'. For a given generator 'g',
            e = g^x. The multiplicative inverse of e is g^(255 - x). It turns
            out that if use the inverse of a generator as another generator
            it will produce all of the corresponding multiplicative inverses
            at the same time. For this reason, we choose 5 as our inverse
            generator because it only requires 2 multiplies and 1 add and its
            inverse, 82, requires relatively few operations as well.
            
            In order to apply the affine transformation, the multiplicative
            inverse 'ei' of 'e' can be repeatedly XOR'd (4 times) with a
            bit-cycling of 'ei'. To do this 'ei' is first stored in 's' and
            'x'. Then 's' is left shifted and the high bit of 's' is made the
            low bit. The resulting value is stored in 's'. Then 'x' is XOR'd
            with 's' and stored in 'x'. On each subsequent iteration the same
            operation is performed. When 4 iterations are complete, 'x' is
            XOR'd with 'c' (0x63) and the transformed value is stored in 'x'.
            For example:
            
            s = 01000001
            x = 01000001
            
            iteration 1: s = 10000010, x ^= s
            iteration 2: s = 00000101, x ^= s
            iteration 3: s = 00001010, x ^= s
            iteration 4: s = 00010100, x ^= s
            x ^= 0x63
            
            This can be done with a loop where s = (s << 1) | (s >> 7). However,
            it can also be done by using a single 16-bit (in this case 32-bit)
            number 'sx'. Since XOR is an associative operation, we can set 'sx'
            to 'ei' and then XOR it with 'sx' left-shifted 1,2,3, and 4 times.
            The most significant bits will flow into the high 8 bit positions
            and be correctly XOR'd with one another. All that remains will be
            to cycle the high 8 bits by XOR'ing them all with the lower 8 bits
            afterwards.
            
            At the same time we're populating sbox and isbox we can precompute
            the multiplication we'll need to do to do MixColumns() later.
         */
         
         // apply affine transformation
         sx = ei ^ (ei << 1) ^ (ei << 2) ^ (ei << 3) ^ (ei << 4);
         sx = (sx >> 8) ^ (sx & 0xFF) ^ 0x63;
         
         // update tables
         sbox[e] = sx;
         isbox[sx] = e;
         
         /* Mixing columns is done using matrix multiplication. The columns
            that are to be mixed are each a single word in the current state.
            The state has Nb columns (4 columns). Therefore each column and
            each row is a 4 byte word. So to mix the columns in a single
            column 'c' we use the following matrix multiplication:
            
            [2 3 1 1]*[c0]=[c'0]
            [1 2 3 1] [c1] [c'1]
            [1 1 2 3] [c2] [c'2]
            [3 1 1 2] [c3] [c'3]
            
            c0, c1, c2, and c3 are the bytes of one of the words in the state.
            To do matrix multiplication, for each mixed column c' we multiply
            the corresponding row from the left matrix with the corresponding
            column from the right matrix. In total, we get 4 equations: 
            
            c'0 = 2*c0 + 3*c1 + 1*c2 + 1*c3
            c'1 = 1*c0 + 2*c1 + 3*c2 + 1*c3
            c'2 = 1*c0 + 1*c1 + 2*c2 + 3*c3
            c'3 = 3*c0 + 1*c1 + 1*c2 + 2*c3
            
            As usual, the multiplication is as previously defined and the
            addition is XOR. In order to optimize mixing columns we can store
            the multiplication results in tables. If you think of the whole
            column as a word (it might help to visualize by mentally rotating
            the equations above by counterclockwise 90 degrees) then you can
            see that it would be useful to map the multiplications performed on
            each byte (c0, c1, c2, c3) onto a word as well. For instance,
            we could map 2*c0,1*c0,1*c0,3*c0 onto a word by storing 2*c0
            in the highest 8 bits and 3*c0 in the lowest 8 bits (with the
            other two respectively in the middle). This means that a table
            can be constructed that uses c0 as an index to the word. We can
            do something similar with c1, c2, and c3, creating a total of
            4 tables.
            
            To construct a full c', we can just look up each byte of c in
            their respective tables and XOR the results together.
            
            Also, to build each table we only have to calculate the word
            for 2,1,1,3 for every byte ... which we can do on each iteration
            of this loop since we will iterate over every byte. After we have
            calculated 2,1,1,3 we can get the results for the other tables
            by cycling the byte at the end to the beginning. For instance
            we can take the result of table 2,1,1,3 and produce table 3,2,1,1
            by moving the right most byte to the left most position just like
            how you can imagine the 3 moved out of 2,1,1,3 and to the front
            to produce 3,2,1,1.
            
            There is another optimization in that the same multiples of
            the current element we need in order to advance our generator
            to the next iteration can be reused in performing the 2,1,1,3
            calculation. We also calculate the inverse mix column tables,
            with e,9,d,b being the inverse of 2,1,1,3.
            
            When we're done, and we need to actually mix columns, the first
            byte of each state word should be put through mix[0] (2,1,1,3),
            the second through mix[1] (3,2,1,1) and so forth. Then they should
            be XOR'd together to produce the fully mixed column.
         */
         
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
         for(var n = 0; n < 4; n++)
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
            e = e2 ^ xtime[xtime[xtime[e2 ^ e8]]];
            ei ^= xtime[xtime[ei]];
         }
      }
   };
   
   /**
    * Generates a key schedule using the AES key expansion algorithm.
    * 
    * The AES algorithm takes the Cipher Key, K, and performs a Key Expansion
    * routine to generate a key schedule. The Key Expansion generates a total
    * of Nb*(Nr + 1) words: the algorithm requires an initial set of Nb words,
    * and each of the Nr rounds requires Nb words of key data. The resulting
    * key schedule consists of a linear array of 4-byte words, denoted [wi ],
    * with i in the range 0 ≤ i < Nb(Nr + 1).
    * 
    * KeyExpansion(byte key[4*Nk], word w[Nb*(Nr+1)], Nk)
    * AES-128 (Nb=4, Nk=4, Nr=10)
    * AES-192 (Nb=4, Nk=6, Nr=12)
    * AES-256 (Nb=4, Nk=8, Nr=14)
    * Note: Nr=Nk+6.
    * 
    * Nb is the number of columns (32-bit words) comprising the State (or
    * number of bytes in a block). For AES, Nb=4.
    * 
    * @param key the key to schedule (as an array of 32-bit words).
    * @param decrypt true to modify the key schedule to decrypt, false not to.
    * 
    * @return the generated key schedule.
    */
   var expandKey = function(key, decrypt)
   {
      // copy the key's words to initialize the key schedule
      var w = key.slice(0);
      
      /* RotWord() will rotate a word, moving the first byte to the last
         byte's position (shifting the other bytes left).
         
         We will be getting the value of Rcon at i / Nk. 'i' will iterate
         from Nk to (Nb * Nr+1). Nk = 4 (4 byte key), Nb = 4 (4 words in
         a block), Nr = Nk + 6 (10). Therefore 'i' will iterate from
         4 to 44 (exclusive). Each time we iterate 4 times, i / Nk will
         increase by 1. We use a counter iNk to keep track of this.
      */
      
      // go through the rounds expanding the key
      var temp, iNk = 1;
      var Nk = w.length;
      var Nr1 = Nk + 6 + 1;
      for(var i = Nk; i < Nb * Nr1; i++)
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
      
      /* When we are updating a cipher block we always use the code path for
         encryption whether we are decrypting or not (to shorten code and
         simplify the generation of look up tables). However, because there
         are differences in the decryption algorithm, other than just swapping
         in different look up tables, we must transform our key schedule to
         account for these changes:
         
         1. The decryption algorithm gets its key rounds in reverse order.
         2. The decryption algorithm adds the round key before mixing columns
            instead of afterwards.
         
         We don't need to modify our key schedule to handle the first case,
         we can just traverse the key schedule in reverse order when decrypting.
         
         The second case requires a little work.
         
         The tables we built for performing rounds will take an input and then
         perform SubBytes() and MixColumns() or, for the decrypt version,
         InvSubBytes() and InvMixColumns(). But the decrypt algorithm requires
         us to AddRoundKey() before InvMixColumns(). This means we'll need to
         apply some transformations to the round key to inverse-mix its columns
         so they'll be correct for moving AddRoundKey() to after the state has
         had its columns inverse-mixed.
         
         To inverse-mix the columns of the state when we're decrypting we use a
         lookup table that will apply InvSubBytes() and InvMixColumns() at the
         same time. However, the round key's bytes are not inverse-substituted
         in the decryption algorithm. To get around this problem, we can first
         substitute the bytes in the round key so that when we apply the
         transformation via the InvSubBytes()+InvMixColumns() table, it will
         undo our substitution leaving us with the original value that we
         want -- and then inverse-mix that value.
         
         This change will correctly alter our key schedule so that we can XOR
         each round key with our already transformed decryption state. This
         allows us to use the same code path as the encryption algorithm.
      */
      if(decrypt)
      {
         var imx0 = imix[0];
         var imx1 = imix[1];
         var imx2 = imix[2];
         var imx3 = imix[3];
         // do not modify the first or last round key (round keys are Nb words)
         // as no column mixing is performed before they are added 
         for(var i = Nb; i < w.length - Nb; i++)
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
   };
   
   /**
    * Updates a single block (16 bytes) using AES. The update will either
    * encrypt or decrypt the block.
    * 
    * @param w the key schedule.
    * @param input the input block (an array of 32-bit words).
    * @param output the updated output block.
    * @param decrypt true to decrypt the block, false to encrypt it.
    */
   var updateBlock = function(w, input, output, decrypt)
   {
      /*
      Cipher(byte in[4*Nb], byte out[4*Nb], word w[Nb*(Nr+1)])
      begin
          byte state[4,Nb]
          state = in
          AddRoundKey(state, w[0, Nb-1])
          for round = 1 step 1 to Nr–1
             SubBytes(state)
             ShiftRows(state)
             MixColumns(state)
             AddRoundKey(state, w[round*Nb, (round+1)*Nb-1])
          end for
          SubBytes(state)
          ShiftRows(state)
          AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
          out = state
      end
      
      InvCipher(byte in[4*Nb], byte out[4*Nb], word w[Nb*(Nr+1)])
      begin
          byte state[4,Nb]
          state = in
          AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
          for round = Nr-1 step -1 downto 1
             InvShiftRows(state)
             InvSubBytes(state)
             AddRoundKey(state, w[round*Nb, (round+1)*Nb-1])
             InvMixColumns(state)
          end for
          InvShiftRows(state)
          InvSubBytes(state)
          AddRoundKey(state, w[0, Nb-1])
          out = state
      end
      */
      
      // Encrypt: AddRoundKey(state, w[0, Nb-1])
      // Decrypt: AddRoundKey(state, w[Nr*Nb, (Nr+1)*Nb-1])
      // Note: To understand the purpose of the 'order' var read on below.
      var Nr = w.length / 4 - 1;
      var i, delta, order;
      var mx0, mx1, mx2, mx3, sub;
      if(decrypt)
      {
         i = Nr * Nb;
         delta = -4;
         order = [3, 0, 1, 2];
         mx0 = imix[0];
         mx1 = imix[1];
         mx2 = imix[2];
         mx3 = imix[3];
         sub = isbox;
      }
      else
      {
         i = 0;
         delta = 4;
         order = [1, 2, 3, 0];
         mx0 = mix[0];
         mx1 = mix[1];
         mx2 = mix[2];
         mx3 = mix[3];
         sub = sbox;
      }
      output[0] = input[0] ^ w[i];
      output[1] = input[1] ^ w[i + 1];
      output[2] = input[2] ^ w[i + 2];
      output[3] = input[3] ^ w[i + 3];
      
      /* In order to share code we follow the encryption algorithm when both
         encrypting and decrypting. To account for the changes required in the
         decryption algorithm, we use different lookup tables when decrypting
         and use a modified key schedule to account for the difference in the
         order of transformations applied when performing rounds. We also get
         key rounds in reverse order (relative to encryption).
      */
      var s0, s1, s2, s3;
      for(var round = 1; round < Nr; round++)
      {
         /* As described above, we'll be using table lookups to perform the
            column mixing. In order to mix columns we perform these
            transformations on each column (s0, s1, s2, s3 are columns):
            
            c'0 = 2*c0 + 3*c1 + 1*c2 + 1*c3
            c'1 = 1*c0 + 2*c1 + 3*c2 + 1*c3
            c'2 = 1*c0 + 1*c1 + 2*c2 + 3*c3
            c'3 = 3*c0 + 1*c1 + 1*c2 + 2*c3
            
            So using mix tables,
            mx0[s0 >> 24] will yield this word: [2*c0,1*c0,1*c0,3*c0]
            ...
            mx3[s0 & 0xFF] will yield this word: [1*c3,1*c3,3*c3,2*c3]
            
            Therefore (& 0xFF omitted for brevity):
            s0 = mx0[s0 >> 24] ^ mx1[s0 >> 16] ^ mx2[s0 >> 8] ^ mx3[s0]
            s1 = mx0[s1 >> 24] ^ mx1[s1 >> 16] ^ mx2[s1 >> 8] ^ mx3[s1]
            s2 = mx0[s2 >> 24] ^ mx1[s2 >> 16] ^ mx2[s2 >> 8] ^ mx3[s2]
            s3 = mx0[s3 >> 24] ^ mx1[s3 >> 16] ^ mx2[s3 >> 8] ^ mx3[s3]
            
            However, first we need to perform ShiftRows(). The ShiftRows()
            transformation cyclically shifts the last 3 rows of the state
            over different offsets. The first row (r = 0) is not shifted.
            
            s'_r,c = s_r,(c + shift(r, Nb) mod Nb
            for 0 < r < 4 and 0 <= c < Nb and
            shift(1, 4) = 1
            shift(2, 4) = 2
            shift(3, 4) = 3.
            
            This causes the first byte in r = 1 to be moved to the end of
            the row, the first 2 bytes in r = 2 to be moved to the end of
            the row, the first 3 bytes in r = 3 to be moved to the end of
            the row:
            
            [s1,0 s1,1 s1,2 s1,3] => [s1,1 s1,2 s1,3 s1,0]
            [s2,0 s2,1 s2,2 s2,3]    [s2,2 s2,3 s2,0 s2,1]
            [s3,0 s3,1 s3,2 s3,3]    [s3,2 s3,3 s3,0 s3,1]
            
            We can make these substitutions inline with our column mixing to
            generate an updated set of equations:
            
            c0 c1 c2 c3 => c0 c1 c2 c3
            c0 c1 c2 c3    c1 c2 c3 c0  (cycled 1 byte)
            c0 c1 c2 c3    c2 c3 c0 c1  (cycled 2 bytes)
            c0 c1 c2 c3    c3 c0 c1 c2  (cycled 3 bytes)
            
            c'0 = 2*c0 + 3*c1 + 1*c2 + 1*c3
            c'1 = 1*c1 + 2*c2 + 3*c3 + 1*c0
            c'2 = 1*c2 + 1*c3 + 2*c0 + 3*c1
            c'3 = 3*c3 + 1*c0 + 1*c1 + 2*c2
            
            When performing the inverse we transform the mirror image and
            skip the bottom row, instead of the top one, and move upwards:
            
            c3 c2 c1 c0 => c0 c3 c2 c1  (cycled 3 bytes)
            c3 c2 c1 c0    c1 c0 c3 c2  (cycled 2 bytes)
            c3 c2 c1 c0    c2 c1 c0 c3  (cycled 1 byte)
            c3 c2 c1 c0    c3 c2 c1 c0
            
            If you compare the resulting matrixes for ShiftRows()+MixColumns()
            and for InvShiftRows()+InvMixColumns() only the 2nd and 4th columns
            are different. So the code below uses an array to specify the order
            for the 2nd and 4th columns based on encryption vs. decryption mode.
            
            encrypt: c1,c2,c3,c0
            decrypt: c3,c0,c1,c2
         */
         i += delta;
         s0 =
            mx0[output[0] >>> 24] ^
            mx1[output[order[0]] >>> 16 & 0xFF] ^
            mx2[output[2] >>> 8 & 0xFF] ^
            mx3[output[order[2]] & 0xFF] ^ w[i];
         s1 =
            mx0[output[1] >>> 24] ^
            mx1[output[order[1]] >>> 16 & 0xFF] ^
            mx2[output[3] >>> 8 & 0xFF] ^
            mx3[output[order[3]] & 0xFF] ^ w[i + 1];
         s2 =
            mx0[output[2] >>> 24] ^
            mx1[output[order[2]] >>> 16 & 0xFF] ^
            mx2[output[0] >>> 8 & 0xFF] ^
            mx3[output[order[0]] & 0xFF] ^ w[i + 2];
         s3 =
            mx0[output[3] >>> 24] ^
            mx1[output[order[3]] >>> 16 & 0xFF] ^
            mx2[output[1] >>> 8 & 0xFF] ^
            mx3[output[order[1]] & 0xFF] ^ w[i + 3];
         output[0] = s0;
         output[1] = s1;
         output[2] = s2;
         output[3] = s3;
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
      i += delta;
      s0 =
         (sub[output[0] >>> 24] << 24) ^
         (sub[output[order[0]] >>> 16 & 0xFF] << 16) ^
         (sub[output[2] >>> 8 & 0xFF] << 8) ^
         (sub[output[order[2]] & 0xFF]) ^ w[i];
      s1 =
         (sub[output[1] >>> 24] << 24) ^
         (sub[output[order[1]] >>> 16 & 0xFF] << 16) ^
         (sub[output[3] >>> 8 & 0xFF] << 8) ^
         (sub[output[order[3]] & 0xFF]) ^ w[i + 1];
      s2 =
         (sub[output[2] >>> 24] << 24) ^
         (sub[output[order[2]] >>> 16 & 0xFF] << 16) ^
         (sub[output[0] >>> 8 & 0xFF] << 8) ^
         (sub[output[order[0]] & 0xFF]) ^ w[i + 2];
      s3 =
         (sub[output[3] >>> 24] << 24) ^
         (sub[output[order[3]] >>> 16 & 0xFF] << 16) ^
         (sub[output[1] >>> 8 & 0xFF] << 8) ^
         (sub[output[order[1]] & 0xFF]) ^ w[i + 3];
      output[0] = s0;
      output[1] = s1;
      output[2] = s2;
      output[3] = s3;
   };
   
   /**
    * Creates an AES cipher object. CBC (cipher-block-chaining) mode will be
    * used.
    * 
    * The key and iv may be given as a string of bytes, an array of bytes, a
    * byte buffer, or an array of 32-bit words.
    * 
    * @param key the symmetric key to use.
    * @param iv the initialization vector to use.
    * @param output the buffer to write to.
    * @param decrypt true for decryption, false for encryption.
    * 
    * @return the cipher.
    */
   var createCipher = function(key, iv, output, decrypt)
   {
      var cipher = null;
      
      if(!init)
      {
         initialize();
      }
      
      /* Note: The key may be a string of bytes, an array of bytes, a byte
         buffer, or an array of 32-bit integers. If the key is in bytes, then
         it must be 16, 24, or 32 bytes in length. If it is in 32-bit
         integers, it must be 4, 6, or 8 integers long.
      */
      
      // convert iv string into byte buffer
      if(key.constructor == String &&
         (key.length == 16 || key.length == 24 || key.length == 32))
      {
         var tmp = key;
         var key = window.krypto.utils.createBuffer();
         for(var i = 0; i < tmp.length; i++)
         {
            key.putByte(tmp[i]);
         }
      }
      // convert key byte array into byte buffer
      else if(key.constructor == Array &&
         (key.length == 16 || key.length == 24 || key.length == 32))
      {
         key = window.krypto.utils.createBuffer(key);
      }
      
      // convert key byte buffer into 32-bit integer array
      if(key.constructor != Array)
      {
         var tmp = key;
         key = [];
         
         // key lengths of 16, 24, 32 bytes allowed
         var len = tmp.length();
         if(len == 16 || len == 24 || len == 32)
         {
            while(tmp.length() > 0)
            {
               key.push(tmp.getInt32());
            }
         }
      }
      
      // key must be an array of 32-bit integers by now
      if(key.constructor == Array &&
         (key.length == 4 || key.length == 6 || key.length == 8))
      {
         // private vars for state
         var _w = expandKey(key, decrypt);
         var _blockSize = Nb << 2;
         var _input;
         var _output;
         var _inBlock;
         var _outBlock;
         var _prev;
         var _finish;
         cipher =
         {
            /**
             * Output from AES (either encrypted or decrypted bytes).
             */
            output: null
         };
         
         /**
          * Updates the next block using CBC mode.
          * 
          * @param input the buffer to read from.
          */
         cipher.update = function(input)
         {
            if(!_finish)
            {
               // not finishing, so fill the input buffer with more input
               _input.putBuffer(input);
            }
            
            /* In encrypt mode, the threshold for updating a block is the
               block size. As soon as enough input is available to update
               a block, encryption may occur. In decrypt mode, we wait for
               2 blocks to be available or for the finish flag to be set
               with only 1 block available. This is done so that the output
               buffer will not be populated with padding bytes at the end
               of the decryption -- they can be truncated before returning
               from finish().
             */
            var threshold = decrypt && !_finish ?
               _blockSize << 1 : _blockSize;
            while(_input.length() >= threshold)
            {
               // get next block
               if(decrypt)
               {
                  for(var i = 0; i < Nb; i++)
                  {
                     _inBlock[i] = _input.getInt32();
                  }
               }
               else
               {
                  // CBC mode XOR's IV (or previous block) with plaintext
                  for(var i = 0; i < Nb; i++)
                  {
                     _inBlock[i] = _prev[i] ^ _input.getInt32();
                  }
               }
               
               // update block
               updateBlock(_w, _inBlock, _outBlock, decrypt);
               
               // write output, save previous ciphered block
               if(decrypt)
               {
                  // CBC mode XOR's IV (or previous block) with plaintext
                  for(var i = 0; i < Nb; i++)
                  {
                     _output.putInt32(_prev[i] ^ _outBlock[i]);
                  }
                  _prev = _inBlock.slice(0);
               }
               else
               {
                  for(var i = 0; i < Nb; i++)
                  {
                     _output.putInt32(_outBlock[i]);
                  }
                  _prev = _outBlock;
               }
            }
         };
         
         /**
          * Finishes encrypting or decrypting.
          * 
          * @param pad a padding function to use, null for default,
          *           signature(blockSize, buffer, decrypt).
          * 
          * @return true if successful, false on error.
          */
         cipher.finish = function(pad)
         {
            var rval = true;
            
            if(!decrypt)
            {
               if(pad)
               {
                  rval = pad(_blockSize, _input, decrypt);
               }
               else
               {
                  // add PKCS#7 padding to block (each pad byte is the
                  // value of the number of pad bytes)
                  var padding = Math.max(
                     _blockSize, _blockSize - _input.length());
                  for(var i = 0; i < padding; i++)
                  {
                     _input.putByte(padding);
                  }
               }
            }
            
            if(rval)
            {
               // do final update
               _finish = true;
               cipher.update();
            }
            
            if(decrypt)
            {
               // check for error: input data not a multiple of blockSize
               rval = (_input.length() == 0);
               if(rval)
               {
                  if(pad)
                  {
                     rval = pad(_blockSize, _output, decrypt);
                  }
                  else
                  {
                     // ensure padding bytes are valid
                     var len = _output.length();
                     var count = _output.at(len - 1);
                     for(var i = len - count; rval && i < len; i++)
                     {
                        rval = (_output.at(i) == count);
                     }
                     if(rval)
                     {
                        // trim off padding bytes
                        _output.truncate(count);
                     }
                  }
               }
            }
            
            return rval;
         };
         
         /**
          * Restarts the encryption or decryption process, whichever was
          * previously configured.
          * 
          * The iv may be given as a string of bytes, an array of bytes, a
          * byte buffer, or an array of 32-bit words.
          * 
          * @param iv the initialization vector to use.
          * @param output the output the buffer to write to.
          */
         cipher.restart = function(iv, output)
         {
            /* Note: The IV may be a string of bytes, an array of bytes, a
               byte buffer, or an array of 32-bit integers. If the IV is in
               bytes, then it must be Nb (16) bytes in length. If it is in
               32-bit integers, then it must be 4 integers long. */
            
            // convert iv string into byte buffer
            if(iv.constructor == String && iv.length == 16)
            {
               iv = window.krypto.utils.createBuffer(iv);
            }
            // convert iv byte array into byte buffer
            else if(iv.constructor == Array && iv.length == 16)
            {
               var tmp = iv;
               var iv = window.krypto.utils.createBuffer();
               for(var i = 0; i < 16; i++)
               {
                  iv.putByte(tmp[i]);
               }
            }
            
            // convert iv byte buffer into 32-bit integer array
            if(iv.constructor != Array)
            {
               var tmp = iv;
               iv = new Array(4);
               iv[0] = tmp.getInt32();
               iv[1] = tmp.getInt32();
               iv[2] = tmp.getInt32();
               iv[3] = tmp.getInt32();
            }
            
            // set private vars
            _input = window.krypto.utils.createBuffer();
            _output = output || window.krypto.utils.createBuffer();
            _prev = iv.slice(0);
            _inBlock = new Array(Nb);
            _outBlock = new Array(Nb);
            _finish = false;
            cipher.output = _output;
         };
         cipher.restart(iv, output);
      }
      return cipher;
   };
   
   /**
    * The crypto namespace and aes API.
    */
   window.krypto = window.krypto || {};
   window.krypto.aes =
   {
      /**
       * Creates an AES cipher object to encrypt data in CBC mode using the
       * given symmetric key. The output will be stored in the 'output' member
       * of the returned cipher.
       * 
       * The key and iv may be given as a string of bytes, an array of bytes,
       * a byte buffer, or an array of 32-bit words.
       * 
       * @param key the symmetric key to use.
       * @param iv the initialization vector to use.
       * @param output the buffer to write to, null to create one.
       * 
       * @return the cipher.
       */
      startEncrypting: function(key, iv, output)
      {
         return createCipher(key, iv, output, false);
      },
      
      /**
       * Creates an AES cipher object to decrypt data in CBC mode using the
       * given symmetric key. The output will be stored in the 'output' member
       * of the returned cipher.
       * 
       * The key and iv may be given as a string of bytes, an array of bytes,
       * a byte buffer, or an array of 32-bit words.
       * 
       * @param key the symmetric key to use.
       * @param iv the initialization vector to use.
       * @param output the buffer to write to, null to create one.
       * 
       * @return the cipher.
       */
      startDecrypting: function(key, iv, output)
      {
         return createCipher(key, iv, output, true);
      },
      
      /**
       * Expands a key. Typically only used for testing.
       * 
       * @param key the symmetric key to expand, as an array of 32-bit words.
       * @param decrypt true to expand for decryption, false for encryption.
       * 
       * @return the expanded key.
       */
      _expandKey: function(key, decrypt)
      {
         if(!init)
         {
            initialize();
         }
         return expandKey(key, decrypt);
      },
      
      /**
       * Updates a single block. Typically only used for testing.
       * 
       * @param w the expanded key to use.
       * @param input an array of block-size 32-bit words.
       * @param output an array of block-size 32-bit words.
       * @param decrypt true to decrypt, false to encrypt.
       */
      _updateBlock: updateBlock
   };
})(jQuery);
