/*Copyright (C) 2021 Dennis M. Heine
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <EEPROM.h>
#include "TOTP.h"
#include "AES.h"
#include "SoftwareReset.h"

AES aes;
byte AESkey[] = {0x7e, 0x4e, 0x42, 0x38, 0x43, 0x63, 0x4f, 0x4c, 0x23, 0x4a, 0x21, 0x48, 0x3f, 0x7c, 0x59, 0x72};
byte iv[N_BLOCK] = {3, 1, 7, 6, 9, 3, 7, 2, 0, 9, 9, 1, 5, 2, 8, 7};
byte iv_enc[100] = {};
bool locked = false;

uint8_t generate_random_unit8()//!!
{
  uint8_t really_random = *(volatile uint8_t *)0x3FF20E44;
  return really_random;
}

void generate_iv(byte *vector)
{
  for (int i = 0; i < N_BLOCK; i++)
  {
    vector[i] = (byte)generate_random_unit8();
  }
}


int getPass(char* password)
{
  int passLen = EEPROM.read(0);
  int i;
  for (i = 1; i <= passLen + 1; i++)
  {
    password[i - 1] = EEPROM.read(i);
    if (password[i - 1] == '\0')
      break;
  }

  byte data_decoded[100];
  byte out[200];
  aes.do_aes_decrypt((byte *)password, passLen, data_decoded, AESkey, 128, (byte *)iv);
  password = data_decoded;
}

String getKey(int keyNum)
{
  int passLen = EEPROM.read(0);
  int offset = passLen + 1;

  for (int i = 0; i < keyNum; i++)
  {
    int lenCurrentKey = EEPROM.read(offset);
    if (i == keyNum - 1)
    {
      return readStringFromEEPROM(offset);
    }
    offset = offset + lenCurrentKey + 1;
  }
  return "";
}

int addKey(int keyNum, String newkey)
{
  int passLen = EEPROM.read(0);
  int offset = passLen + 1;

  for (int i = 0; i < keyNum; i++)
  {
    int lenCurrentKey = EEPROM.read(offset);
    if (i == keyNum - 1)
    {
      writeStringToEEPROM(offset, newkey);
    }
    offset = offset + lenCurrentKey + 1;
  }
  return -1;
}

void setup()
{

  setup_aes();
  Serial.begin(9600);
  Serial.setTimeout(10000);
  Serial.write("START");
}

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  uint8_t data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  String sData = String((char*)data);
  return dodecrypt(sData);
}
void setup_aes()
{
  aes.set_key(AESkey, sizeof(AESkey)); // Get the globally defined key
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  String enc = doencrypt(strToWrite);

  EEPROM.write(addrOffset, enc.length());
  for (int i = 0; i < enc.length(); i++)
  {
    EEPROM.write(addrOffset + 1 + i, enc.charAt(i));
  }
}

void loop() {
  if (Serial.available() > 0)
  {
    String pass = Serial.readStringUntil('\n');
    String befehl = Serial.readStringUntil('\n');

    if (befehl == "INIT" && EEPROM.read(0) == 0)
    {
      writeStringToEEPROM(0, pass);
      Serial.println("Initialized.");
    }
    else
    {
      String passBuff = readStringFromEEPROM(0);

      if (pass == passBuff)
      {
        if (befehl == "GETKEY" && !locked)
        {
          String keyNum = Serial.readStringUntil('\n');
          String stime = Serial.readStringUntil('\n');
          long lTime = stime.toInt(); //!!
          String sKey = getKey(keyNum.toInt());
          uint8_t buf1[sKey.length()];

          for (int i = 0; i < sKey.length(); i++)
          {
            buf1[i] = uint8_t(sKey.charAt(i));
          }

          TOTP* t = new TOTP( buf1, 10, 30);
          char* code = t->getCode(lTime);
          Serial.println(code);

          locked = true;

        }
        else if (befehl == "ADDKEY")
        {
          int keyLen = Serial.readStringUntil('\n').toInt();
          int keyNum = Serial.readStringUntil('\n').toInt();
          String newKey = Serial.readStringUntil('\n');

          addKey(keyNum, newKey);

          locked = true;

        }
        else if (befehl == "NEWPASS")
        {
          String newPass = Serial.readStringUntil('\n');
          writeStringToEEPROM(0, newPass);
          locked = true;
        }
      }
      else
      {
        Serial.write("WRONGPASS");
      }
    }
  }
}

//Following: Base64 library by Adam Rudd

/*Copyright (C) 2013 Adam Rudd
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

String IV_base64;

String dodecrypt(String b64data)
{
  return _decrypt(b64data, IV_base64, b64data.length());
}

String doencrypt(String msg)
{
  char b64data[200];
  byte cipher[100];

  base64_encode(b64data, (char *)iv, N_BLOCK);
  IV_base64 = String(b64data);

  int b64len = base64_encode(b64data, (char *)msg.c_str(), msg.length());



  // Encrypt! With AES128, our key and IV, CBC and pkcs7 padding
  aes.do_aes_encrypt((byte *)b64data, b64len, cipher, AESkey, 128, iv);



  int len = base64_encode(b64data, (char *)cipher, aes.get_size());
  //b64data[len]='\0';
  //Serial.println(b64data);
  return String(b64data);

}

String _decrypt(String b64data, String IV_base64, int isize)
{
  char data_decoded[200];
  char iv_decoded[200];
  char out[200];
  char temp[200];
  b64data.toCharArray(temp, 200);
  base64_decode(data_decoded, temp, b64data.length());
  //IV_base64.toCharArray(temp, 200);
  //base64_decode(iv_decoded, temp, IV_base64.length());
  aes.do_aes_decrypt((byte *)data_decoded, isize, out, AESkey, 128, iv);
  char message[100];
  int len = base64_decode(message, (char *)out, aes.get_size());

  message[len] = '\0';

  return String(message);
}

const char PROGMEM base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz"
                                       "0123456789+/";

/* 'Private' declarations */
inline void a3_to_a4(unsigned char * a4, unsigned char * a3);
inline void a4_to_a3(unsigned char * a3, unsigned char * a4);
inline unsigned char base64_lookup(char c);

int base64_encode(char *output, char *input, int inputLen) {
  int i = 0, j = 0;
  int encLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];

  while (inputLen--) {
    a3[i++] = *(input++);
    if (i == 3) {
      a3_to_a4(a4, a3);

      for (i = 0; i < 4; i++) {
        output[encLen++] = pgm_read_byte(&base64_alphabet[a4[i]]);
      }

      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) {
      a3[j] = '\0';
    }

    a3_to_a4(a4, a3);

    for (j = 0; j < i + 1; j++) {
      output[encLen++] = pgm_read_byte(&base64_alphabet[a4[j]]);
    }

    while ((i++ < 3)) {
      output[encLen++] = '=';
    }
  }
  output[encLen] = '\0';
  return encLen;
}

int base64_decode(char * output, char * input, int inputLen) {
  int i = 0, j = 0;
  int decLen = 0;
  unsigned char a3[3];
  unsigned char a4[4];


  while (inputLen--) {
    if (*input == '=') {
      break;
    }

    a4[i++] = *(input++);
    if (i == 4) {
      for (i = 0; i < 4; i++) {
        a4[i] = base64_lookup(a4[i]);
      }

      a4_to_a3(a3, a4);

      for (i = 0; i < 3; i++) {
        output[decLen++] = a3[i];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) {
      a4[j] = '\0';
    }

    for (j = 0; j < 4; j++) {
      a4[j] = base64_lookup(a4[j]);
    }

    a4_to_a3(a3, a4);

    for (j = 0; j < i - 1; j++) {
      output[decLen++] = a3[j];
    }
  }
  output[decLen] = '\0';
  return decLen;
}

int base64_enc_len(int plainLen) {
  int n = plainLen;
  return (n + 2 - ((n + 2) % 3)) / 3 * 4;
}

int base64_dec_len(char * input, int inputLen) {
  int i = 0;
  int numEq = 0;
  for (i = inputLen - 1; input[i] == '='; i--) {
    numEq++;
  }

  return ((6 * inputLen) / 8) - numEq;
}

inline void a3_to_a4(unsigned char * a4, unsigned char * a3) {
  a4[0] = (a3[0] & 0xfc) >> 2;
  a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
  a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
  a4[3] = (a3[2] & 0x3f);
}

inline void a4_to_a3(unsigned char * a3, unsigned char * a4) {
  a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
  a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
  a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline unsigned char base64_lookup(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 71;
  if (c >= '0' && c <= '9') return c + 4;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}
