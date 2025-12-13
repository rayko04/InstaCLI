#ifndef ENCRYPTION_HPP
#define ENCRYPTION_HPP

#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

class Encryption {
private:
    static const std::string SECRET_KEY;
    
public:
    // Encrypt
    static std::string encrypt(const std::string& plaintext) {
        if (plaintext.empty()) return plaintext;
        
        std::string encrypted;
        size_t keyLen = SECRET_KEY.length();
        
        for (size_t i = 0; i < plaintext.length(); i++) {
            char encryptedChar = plaintext[i] ^ SECRET_KEY[i % keyLen];
            encrypted += encryptedChar;
        }
        
        return encrypted;
    }
    
    // Decrypt
    static std::string decrypt(const std::string& ciphertext) {
        // XOR encryption is symmetric
        return encrypt(ciphertext);
    }
    
    // Helper: Convert to hex for readable display
    static std::string toHex(const std::string& data) {
        const char hex_chars[] = "0123456789ABCDEF";
        std::string hex;
        
        for (unsigned char c : data) {
            hex += hex_chars[(c >> 4) & 0x0F];
            hex += hex_chars[c & 0x0F];
        }
        
        return hex;
    }
    
    // Helper: Convert from hex
    static std::string fromHex(const std::string& hex) {
        std::string result;
        
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            char byte = (char)strtol(byteString.c_str(), nullptr, 16);
            result += byte;
        }
        
        return result;
    }
};


const std::string Encryption::SECRET_KEY = "MySecretKey12345!@#$%";

#endif
