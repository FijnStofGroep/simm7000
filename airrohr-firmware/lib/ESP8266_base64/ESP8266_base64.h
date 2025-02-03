/*
 * @file ESP8266_base64.h
 *
 * Written by R.Dieperink, Rolenco Leusden
 * Date: 2024-04-13
 * 
 * ESP8266-base64.h encoding and decoding with C++.
 * 
 * Copyright (C) 2024
 *
 * This source code is provided 'as-is', without any express or implied
 * warranty. In no event will the author be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 */

#ifndef _ESP8266_BASE64_H
#define _ESP8266_BASE64_H

/* b64_alphabet:
 *     Description: Base64 alphabet table, a mapping between integers
 *           and base64 digits
 *    Notes: This is an extern here but is defined in Base64.c
 */
extern const char b64_alphabet[];

/* b64_encode:
 *    Description:
 *      Encode a string of characters as base64
 *    Parameters:
 *      output: the output buffer for the encoding, stores the encoded string
 *      input: the input buffer for the encoding, stores the binary to be encoded
 *      inputLen: the length of the input buffer, in bytes
 *    Return value:
 *      Returns the length of the encoded string
 *    Requirements:
 *      1. output must not be null or empty
 *      2. input must not be null
 *      3. inputLen must be greater than or equal to 0
 */
int b64_encode(char *output, const char *input, int inputLen);

/* b64_decode:
 *    Description:
 *      Decode a base64 encoded string into bytes
 *    Parameters:
 *      output: the output buffer for the decoding,
 *          stores the decoded binary
 *      input: the input buffer for the decoding,
 *           stores the base64 string to be decoded
 *      inputLen: the length of the input buffer, in bytes
 *    Return value:
 *      Returns the length of the decoded string
 *    Requirements:
 *      1. output must not be null or empty
 *      2. input must not be null
 *      3. inputLen must be greater than or equal to 0
 */
int b64_decode(char *output, const char *input, int inputLen);

#endif // _ESP8266_BASE64_H 
