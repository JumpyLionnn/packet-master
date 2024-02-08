# packet-master
 a serialization and deserialization library made in c

This is has been a learning experience. It can serialize uint8_t and bool to their smallest representable way.

for bool 1 bit
for uint8_t 8 bits unless specified otherwise
for uint16_t 9 or 17 bits unless specified otherwise 1 bit for the byte count stored
planned to serialize and deserialize uint32_t