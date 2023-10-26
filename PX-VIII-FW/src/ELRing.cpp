#include "ELRing.h"

ELRing::ELRing()
{

}

ELRing::ELRing(uint8_t num_el_buttons, uint8_t ring_num)
{
    _n_els = num_el_buttons;
    _n_ring = ring_num;
    _n_last_activated_el = -1;
    _n_el_quad = _n_els / NUM_QUADRANTS;

    _els = new uint8_t[_n_els];
    for(uint8_t i = 0; i < _n_els; i++){
        set_el(i, INACT);
    }
}

void ELRing::init(uint8_t num_el_buttons, uint8_t ring_num)
{
    _n_els = num_el_buttons;
    _n_ring = ring_num;
    _n_last_activated_el = -1;
    _n_el_quad = _n_els / NUM_QUADRANTS;

    _els = new uint8_t[_n_els];
    for(uint8_t i = 0; i < _n_els; i++){
        set_el(i, INACT);
    }
}

void ELRing::turn_off_ring()
{
    for(uint8_t i = 0; i < _n_els; i++){
        set_el(i, INACT);
    }
}

el_status_t ELRing::set_el(uint8_t idx, el_status_t status)
{
    if(_n_ring == 1){
        if(idx == 7){
            idx = 6; //work around for ring 1 ending up with 7 instead if 8 el-buttons
        }
    }
    if(idx < _n_els){
        *(_els + idx) = status;
        return (el_status_t) *(_els + idx);
    }

    return IDX_ERR;
}

el_status_t ELRing::set_el(uint8_t quadrant, uint8_t q_idx, el_status_t status)
{
    if(_n_ring == 1){
        if(quadrant == 7){
            quadrant = 6; //work around for ring 1 ending up with 7 instead if 8 el-buttons
        }
    }
    uint8_t idx = (quadrant * _n_el_quad) + q_idx;
    return set_el(idx, status);
}

el_status_t ELRing::get_el_status(uint8_t idx){
    if(idx < _n_els){
        return (el_status_t) *(_els + idx);
    }
    
    return IDX_ERR;
}

el_status_t ELRing::get_el_status(uint8_t quadrant, uint8_t q_idx)
{
    uint8_t idx = (quadrant * _n_el_quad) + q_idx;
    return get_el_status(idx);
}


uint8_t ELRing::get_el_num(void)
{
    return _n_els;
}

uint8_t ELRing::els_per_quadrant(void)
{
    return _n_el_quad;
}

uint8_t ELRing::get_el_quadrant(uint8_t idx)
{   if(_n_ring == 1){
        if(idx == 7){
            idx = 6; // ring 1 has 7 quadrants...
        }
    }
    return idx / _n_el_quad;
}

