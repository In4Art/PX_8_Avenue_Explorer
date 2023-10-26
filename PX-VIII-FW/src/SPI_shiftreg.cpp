#include "SPI_shiftreg.h"


SPI_shiftreg::SPI_shiftreg(uint8_t oe_pin, uint8_t l_pin, uint8_t clr_pin, uint8_t n_regs)
{
    _oe_pin = oe_pin;
    _l_pin = l_pin;
    _clr_pin  = clr_pin;
    _n_regs = n_regs;
    pinMode(_oe_pin, OUTPUT);
    pinMode(_l_pin, OUTPUT);
    pinMode(_clr_pin, OUTPUT);

    digitalWrite(_oe_pin, HIGH); //output disable shift reg
	digitalWrite(_clr_pin, HIGH); //don't clear data!!

    _regs = new uint8_t[_n_regs ];

    for(uint8_t i = 0; i < _n_regs; i++){
        *(_regs + i) = 0x00;
    }

    SPI.begin();
    SPI.setHwCs(false);

    shift_data();

}

SPI_shiftreg::~SPI_shiftreg()
{
    delete[] _regs;
}

void SPI_shiftreg::shift_data(void)
{
    uint8_t nBytes = _n_regs;
    while(nBytes > 0){
		SPI.transfer(*(_regs + (nBytes - 1)));
		nBytes--;
	}
	digitalWrite(_l_pin, HIGH);
	delayMicroseconds(10);
	digitalWrite(_l_pin, LOW);
}

void SPI_shiftreg::enable_output(void)
{
    digitalWrite(_oe_pin, LOW);
}

void SPI_shiftreg::disable_output(void)
{
    digitalWrite(_oe_pin, HIGH);
}

reg_res_t SPI_shiftreg::set_data_byte(uint8_t data, uint8_t idx)
{
    if(idx < _n_regs){
        *(_regs + idx) = data;
        return SHIFT_OK;
    }else{
        return IDX_INV;
    }
}


reg_res_t SPI_shiftreg::set_data_bytes(uint8_t *data, uint8_t len)
{
    if(len <= _n_regs){
        for(uint8_t i = 0; i < len; i++){
            *(_regs + i) = *(data + i);
        }
        return SHIFT_OK;
    }else{
        return DATA_OVERFLOW;
    }
}
        
reg_res_t SPI_shiftreg::set_data_bit(uint8_t bit_n, uint8_t val)
{
    uint16_t max_bit_idx = (_n_regs * 8 ) - 1;

    if(bit_n > max_bit_idx){

        return BITN_INV;

    }else{

        uint8_t shift_val = bit_n % 8;
        uint8_t byte_idx = bit_n / 8;

        if(val > 0){
            *(_regs + byte_idx) |= (1 << shift_val);
        }else{
            *(_regs + byte_idx) &= ~(1 << shift_val);
        } 
        return SHIFT_OK;
    }
}



void SPI_shiftreg::clear_all(void)
{
    digitalWrite(_oe_pin, LOW);
    digitalWrite(_clr_pin, LOW);
	digitalWrite(_l_pin, HIGH);
	delayMicroseconds(10);
	digitalWrite(_l_pin, LOW);

    digitalWrite(_oe_pin, HIGH);
    digitalWrite(_clr_pin, HIGH);
    digitalWrite(_oe_pin, LOW);
    digitalWrite(_l_pin, HIGH);
	delayMicroseconds(10);
	digitalWrite(_l_pin, LOW);


    for(uint8_t i = 0; i < _n_regs; i++){
        *(_regs + i) = 0x00;
    }
}