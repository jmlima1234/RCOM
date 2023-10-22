// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);
unsigned char getControl_packet(int control, const char *filename, long int fileSize, int *cpsize);
unsigned char *getData(unsigned char *data, int dataSize, int *packetSize);
void printPacket(const unsigned char *packet, int packetSize);
#endif // _APPLICATION_LAYER_H_
