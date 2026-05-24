#include "pci.h"
#include "serial.h"

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

pci_dev_t pci_devs[PCI_MAX_DEVS];
int       pci_count = 0;

static void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"(port));
}
static uint32_t inl(uint16_t port) {
    uint32_t v;
    __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    uint32_t addr = 0x80000000u
        | ((uint32_t)bus  << 16)
        | ((uint32_t)dev  << 11)
        | ((uint32_t)func <<  8)
        | (off & 0xFC);
    outl(PCI_ADDR, addr);
    return inl(PCI_DATA);
}

void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t val) {
    uint32_t addr = 0x80000000u
        | ((uint32_t)bus  << 16)
        | ((uint32_t)dev  << 11)
        | ((uint32_t)func <<  8)
        | (off & 0xFC);
    outl(PCI_ADDR, addr);
    outl(PCI_DATA, val);
}

uint32_t pci_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t bar) {
    return pci_read(bus, dev, func, 0x10 + bar * 4);
}


void pci_init(void) {
    pci_count = 0;
    uint8_t bus, dev, func;
    for (bus = 0; bus < 8; bus++) {
        for (dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0xFFFF) continue;

            uint8_t ht = (pci_read(bus, dev, 0, 0x0C) >> 16) & 0xFF;
            uint8_t nf = (ht & 0x80) ? 8 : 1;

            for (func = 0; func < nf; func++) {
                uint32_t vid = pci_read(bus, dev, func, 0x00);
                if ((vid & 0xFFFF) == 0xFFFF) continue;
                if (pci_count >= PCI_MAX_DEVS) goto done;

                uint32_t cls = pci_read(bus, dev, func, 0x08);
                pci_dev_t *d   = &pci_devs[pci_count++];
                d->bus         = bus;
                d->dev         = dev;
                d->func        = func;
                d->vendor      = (uint16_t)(vid & 0xFFFF);
                d->device      = (uint16_t)(vid >> 16);
                d->class       = (uint8_t)(cls >> 24);
                d->subclass    = (uint8_t)(cls >> 16);
                d->progif      = (uint8_t)(cls >> 8);
                d->header_type = ht & 0x7F;
            }
        }
    }
done:
    serial_puts("pci: ");
    serial_putu((uint32_t)pci_count);
    serial_puts(" devices\n");
}
