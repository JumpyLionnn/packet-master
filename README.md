# packet-master
An over engineered serialization and deserialization library made in c++

It can serialize uint8_t uint16_t, uint32_t, and bool to their smallest representable way.

## How is serialization done?
* boolean value is 1 bit
* uint8_t 8 bits unless specified otherwise
* uint16_t can be stored as 1 or 2 bytes
* uint32_t can be stored as 1 to 4 bytes