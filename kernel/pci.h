#ifndef PCI_H
#define PCI_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

typedef struct {
    uint8_t  bus, dev, func;
    uint16_t vendor, device;
    uint8_t  class, subclass, progif;
    uint8_t  header_type;
} pci_dev_t;

#define PCI_MAX_DEVS 64

extern pci_dev_t pci_devs[PCI_MAX_DEVS];
extern int       pci_count;

void     pci_init(void);
uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
void     pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t val);
uint32_t pci_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t bar);

#endif
