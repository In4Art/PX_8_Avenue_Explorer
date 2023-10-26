#ifndef _SPI_SHIFTREG_H_
#define _SPI_SHIFTREG_H_

#include <SPI.h>

typedef enum{
    SHIFT_OK,
    IDX_INV,
    DATA_OVERFLOW,
    BITN_INV

}reg_res_t;

class SPI_shiftreg{
    public:
        SPI_shiftreg(uint8_t oe_pin, uint8_t l_pin, uint8_t clr_pin, uint8_t n_regs);
        ~SPI_shiftreg();
        void shift_data(void);
        void enable_output(void);
        void disable_output(void);
        reg_res_t set_data_byte(uint8_t data, uint8_t idx); //set byte at idx to data
        reg_res_t set_data_bytes(uint8_t *data, uint8_t len);
        reg_res_t set_data_bit(uint8_t bit_n, uint8_t val); //value should be 0 or 1, bit_n is number of bit (must fit within n_regs)
        void clear_all(void);

    private:
        uint8_t _oe_pin; //output enable pin number on mcu
        uint8_t _l_pin; //latch pin on mcu
        uint8_t _clr_pin; //register clear pin on mcu
        uint8_t _n_regs; //number of data registers

        uint8_t *_regs; //pointer to registers memory

    

};


#endif

