#ifndef __ELRING_H__
#define __ELRING_H__



#include "SPI_shiftreg.h"

#define NUM_QUADRANTS 8

typedef enum{
    INACT,
    ACT,
    NO_CHANGE,
    IDX_ERR
}el_status_t;

class ELRing
{
    public:

        ELRing();

        ELRing(uint8_t num_el_buttons, uint8_t ring_num);

        void init(uint8_t num_el_buttons, uint8_t ring_num);
        
        void turn_off_ring();
        
        el_status_t set_el(uint8_t idx, el_status_t status);
        
        el_status_t set_el(uint8_t quadrant, uint8_t q_idx, el_status_t status);

        el_status_t get_el_status(uint8_t idx);

        el_status_t get_el_status(uint8_t quadrant, uint8_t q_idx);

        uint8_t get_el_quadrant(uint8_t idx);
        
        uint8_t els_per_quadrant(void);
        
        uint8_t get_el_num(void); // get the number of buttons in this ring

    private:
        //number of el buttons in the ring
        uint8_t _n_els;
        //ring number of this ring
        uint8_t _n_ring;
        //number of el buttons per quadrant
        uint8_t _n_el_quad;
        //last activated el in this ring -> should be -1 when no el is active in this ring
        int8_t _n_last_activated_el;
        ///pointer to buffer with el buttons in ring
        uint8_t *_els; 
};




#endif
